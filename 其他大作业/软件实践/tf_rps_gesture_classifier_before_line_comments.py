import argparse
import json
import os
import random
import shutil
import tempfile
from collections import deque
from datetime import datetime
from pathlib import Path
from time import perf_counter

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "1")

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

tf = None


# 默认目录配置：让数据、模型和输出报告都保存在项目文件夹中，方便演示和提交。
PROJECT_DIR = Path(__file__).resolve().parent
DEFAULT_DATA_DIR = PROJECT_DIR / "gesture_data"
DEFAULT_OUTPUT_DIR = PROJECT_DIR / "outputs" / "rps_gesture"

# 三个手势类别，模型最后一层 softmax 的输出顺序也和这里一致。
CLASSES = ["rock", "paper", "scissors"]
DISPLAY_NAMES = {
    "rock": "石头",
    "paper": "布",
    "scissors": "剪刀",
    "uncertain": "未识别",
    "waiting": "等待",
}
GESTURE_EMOJI = {
    "rock": "✊",
    "paper": "✋",
    "scissors": "✌",
    "uncertain": "❔",
    "waiting": "⏳",
}

# 猜拳胜负规则：识别出用户手势后，电脑选择能赢它的手势。
WINNING_MOVE = {"rock": "paper", "paper": "scissors", "scissors": "rock"}


def require_tf():
    # TensorFlow 启动较慢，所以只在训练或预测真正需要时再导入。
    global tf
    if tf is None:
        import tensorflow as loaded_tf

        tf = loaded_tf
    return tf


def import_cv2():
    # OpenCV 负责摄像头读取、画框、裁剪和基础图像处理。
    try:
        import cv2

        return cv2
    except ImportError as exc:
        raise SystemExit(
            "OpenCV is required for camera collection/prediction.\n"
            "Install it with: python -m pip install opencv-python"
        ) from exc


def ensure_dirs(data_dir: Path):
    # 创建 rock、paper、scissors 三个数据目录。
    for name in CLASSES:
        (data_dir / name).mkdir(parents=True, exist_ok=True)


def class_counts(data_dir: Path) -> dict[str, int]:
    # 统计每个类别已有多少张采集图片。
    counts = {}
    for name in CLASSES:
        folder = data_dir / name
        counts[name] = len(image_files(folder)) if folder.exists() else 0
    return counts


def archive_class_images(data_dir: Path, class_name: str):
    # 重新采集某类手势时，先把旧图片备份，避免直接丢失数据。
    folder = data_dir / class_name
    files = image_files(folder) if folder.exists() else []
    if not files:
        print(f"{class_name} has no images to archive.")
        return None

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = data_dir.parent / "gesture_data_backups" / f"{class_name}_{timestamp}"
    backup_dir.mkdir(parents=True, exist_ok=True)
    for path in files:
        shutil.move(str(path), str(backup_dir / path.name))
    print(f"Archived {len(files)} old {class_name} images to {backup_dir}")
    return backup_dir


def image_files(folder: Path):
    # 只读取常见图片格式，避免把其他文件误当成训练样本。
    suffixes = {".jpg", ".jpeg", ".png", ".bmp"}
    return sorted([path for path in folder.iterdir() if path.suffix.lower() in suffixes])


def center_square(frame, size_ratio: float):
    # 计算画面中心的正方形 ROI，用户把手势放进这个绿色框。
    h, w = frame.shape[:2]
    side = int(min(h, w) * size_ratio)
    x1 = (w - side) // 2
    y1 = (h - side) // 2
    return x1, y1, x1 + side, y1 + side


def square_crop_with_margin(image, x, y, w, h, margin_ratio=0.28):
    # 根据手部轮廓重新裁剪成正方形，并留出少量边缘，避免切掉手指。
    height, width = image.shape[:2]
    cx = x + w / 2
    cy = y + h / 2
    side = int(max(w, h) * (1.0 + margin_ratio))
    side = max(side, 24)
    x1 = int(round(cx - side / 2))
    y1 = int(round(cy - side / 2))
    x2 = x1 + side
    y2 = y1 + side

    pad_left = max(0, -x1)
    pad_top = max(0, -y1)
    pad_right = max(0, x2 - width)
    pad_bottom = max(0, y2 - height)
    if pad_left or pad_top or pad_right or pad_bottom:
        image = cv2_copy_border(image, pad_top, pad_bottom, pad_left, pad_right)
        x1 += pad_left
        y1 += pad_top
        x2 += pad_left
        y2 += pad_top
    return image[y1:y2, x1:x2]


def cv2_copy_border(image, top, bottom, left, right):
    # 裁剪框超出图像边界时，用黑色像素补齐边缘。
    cv2 = import_cv2()
    return cv2.copyMakeBorder(image, top, bottom, left, right, cv2.BORDER_CONSTANT, value=(0, 0, 0))


def hand_mask(roi, cv2):
    # 使用 YCrCb 肤色阈值提取手部候选区域，目的是减少背景对模型的干扰。
    blurred = cv2.GaussianBlur(roi, (5, 5), 0)
    ycrcb = cv2.cvtColor(blurred, cv2.COLOR_BGR2YCrCb)
    ycrcb_mask = cv2.inRange(
        ycrcb,
        np.array([35, 132, 78], dtype=np.uint8),
        np.array([245, 180, 135], dtype=np.uint8),
    )
    kernel = np.ones((5, 5), np.uint8)
    mask = cv2.morphologyEx(ycrcb_mask, cv2.MORPH_OPEN, kernel, iterations=1)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=2)
    mask = cv2.medianBlur(mask, 5)
    return mask


def preprocess_hand_roi(roi, cv2, image_size: int):
    # 训练和预测共用同一套预处理：找手部、去背景、裁成正方形、缩放到模型输入尺寸。
    resized_fallback = cv2.resize(roi, (image_size, image_size))
    mask = hand_mask(roi, cv2)
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    min_area = roi.shape[0] * roi.shape[1] * 0.015
    contours = [contour for contour in contours if cv2.contourArea(contour) >= min_area]
    if not contours:
        return resized_fallback

    height, width = roi.shape[:2]

    def contour_score(contour):
        # 如果脸或衣服也进入绿色框，可能出现多个轮廓；这里优先选择更靠中下方的主轮廓。
        area = cv2.contourArea(contour)
        x, y, w, h = cv2.boundingRect(contour)
        center_y = (y + h / 2) / max(1, height)
        center_x = (x + w / 2) / max(1, width)
        center_bonus = 1.0 - min(1.0, abs(center_x - 0.5) * 1.2)
        lower_bonus = 0.65 + center_y
        return area * lower_bonus * (0.75 + 0.25 * center_bonus)

    contour = max(contours, key=contour_score)
    x, y, w, h = cv2.boundingRect(contour)
    clean_mask = np.zeros(mask.shape, dtype=np.uint8)
    cv2.drawContours(clean_mask, [contour], -1, 255, thickness=cv2.FILLED)
    clean_mask = cv2.dilate(clean_mask, np.ones((7, 7), np.uint8), iterations=1)
    foreground = np.zeros_like(roi)
    foreground[clean_mask > 0] = roi[clean_mask > 0]
    cropped = square_crop_with_margin(foreground, x, y, w, h)
    return cv2.resize(cropped, (image_size, image_size))


def prepare_roi_for_model(roi, cv2, args):
    # 默认启用手部前景提取；如需对比原图训练，可通过 --no-hand-preprocess 关闭。
    if getattr(args, "hand_preprocess", True):
        return preprocess_hand_roi(roi, cv2, args.image_size)
    return cv2.resize(roi, (args.image_size, args.image_size))


def load_image_for_training(path_value, image_size: int, hand_preprocess: bool):
    # TensorFlow 数据管道中调用 OpenCV 读取中文路径图片，并执行和 UI 相同的预处理。
    cv2 = import_cv2()
    if hasattr(path_value, "item"):
        path_value = path_value.item()
    path = path_value.decode("utf-8") if isinstance(path_value, bytes) else str(path_value)
    data = np.fromfile(path, dtype=np.uint8)
    image = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if image is None:
        raise ValueError(f"Could not read image: {path}")
    if hand_preprocess:
        image = preprocess_hand_roi(image, cv2, image_size)
    else:
        image = cv2.resize(image, (image_size, image_size))
    rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    return rgb.astype("float32")


def collect_one_class(args, class_name: str):
    # 采集某一个类别的手势图片，例如 rock、paper 或 scissors。
    cv2 = import_cv2()
    data_dir = Path(args.data_dir)
    ensure_dirs(data_dir)
    save_dir = data_dir / class_name
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        raise SystemExit(f"Could not open camera {args.camera}.")

    existing = len(image_files(save_dir))
    saved = 0
    frame_count = 0
    paused = True
    print(f"Collecting '{class_name}' into {save_dir}")
    print("Put your hand in the green square. Press SPACE to start/pause, q to quit.")

    while saved < args.samples:
        ok, frame = cap.read()
        if not ok:
            break
        frame = cv2.flip(frame, 1)  # 水平翻转后像照镜子，采集时更自然。
        x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
        roi = frame[y1:y2, x1:x2]
        cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 2)
        status = "PAUSED" if paused else "SAVING"
        text = f"{class_name} {saved}/{args.samples} {status}"
        cv2.putText(frame, text, (20, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (30, 240, 80), 2)
        cv2.imshow("gesture collector", frame)

        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break
        if key == 32:
            paused = not paused
        if paused:
            continue

        frame_count += 1
        if frame_count % args.every == 0:
            # 每隔几帧保存一张图片，避免连续样本完全重复。
            img = cv2.resize(roi, (args.image_size, args.image_size))
            out = save_dir / f"{class_name}_{existing + saved:04d}.jpg"
            ok, encoded = cv2.imencode(".jpg", img)
            if not ok:
                raise SystemExit(f"Could not encode image for {out}.")
            encoded.tofile(str(out))
            saved += 1

    cap.release()
    cv2.destroyAllWindows()
    print(f"Saved {saved} images for '{class_name}'.")


def save_jpg(path: Path, image, cv2):
    # 使用 imencode + tofile 保存图片，可以兼容 Windows 中文路径。
    path.parent.mkdir(parents=True, exist_ok=True)
    ok, encoded = cv2.imencode(".jpg", image)
    if ok:
        encoded.tofile(str(path))


def collect_data(args):
    # 数据采集总入口：可以采集全部类别，也可以只重采某一个类别。
    original_samples = args.samples
    if args.replace_existing:
        selected_classes = CLASSES if args.class_name == "all" else [args.class_name]
        for name in selected_classes:
            archive_class_images(Path(args.data_dir), name)

    if args.target_count > 0:
        counts = class_counts(Path(args.data_dir))
        selected_classes = CLASSES if args.class_name == "all" else [args.class_name]
        for name in selected_classes:
            remaining = max(0, args.target_count - counts.get(name, 0))
            if remaining <= 0:
                print(f"{name} already has {counts.get(name, 0)} images, target reached.")
                continue
            input(f"\nPrepare gesture '{name}', need {remaining} more images, then press Enter.")
            args.samples = remaining
            collect_one_class(args, name)
        args.samples = original_samples
        print("Current dataset counts:", class_counts(Path(args.data_dir)))
        return

    if args.class_name == "all":
        for name in CLASSES:
            input(f"\nPrepare gesture '{name}', then press Enter.")
            collect_one_class(args, name)
    else:
        collect_one_class(args, args.class_name)
    print("Current dataset counts:", class_counts(Path(args.data_dir)))


def split_image_paths(data_dir: Path, validation_split: float, seed: int):
    # 分层划分数据集：每个类别都按相同比例拆成训练集和验证集，避免类别不均衡。
    rng = random.Random(seed)
    train_paths, train_labels, val_paths, val_labels = [], [], [], []
    for label, name in enumerate(CLASSES):
        files = image_files(data_dir / name)
        rng.shuffle(files)
        val_count = max(1, int(round(len(files) * validation_split)))
        val_files = files[:val_count]
        train_files = files[val_count:]
        train_paths.extend(str(path) for path in train_files)
        train_labels.extend([label] * len(train_files))
        val_paths.extend(str(path) for path in val_files)
        val_labels.extend([label] * len(val_files))
    return train_paths, train_labels, val_paths, val_labels


def decode_image(path, label, image_size: int, hand_preprocess: bool):
    # 读取图片并转换为 TensorFlow 训练需要的图像张量和 one-hot 标签。
    if hand_preprocess:
        image = tf.numpy_function(
            lambda p: load_image_for_training(p, image_size, True),
            [path],
            tf.float32,
        )
        image.set_shape([image_size, image_size, 3])
    else:
        data = tf.io.read_file(path)
        image = tf.io.decode_image(data, channels=3, expand_animations=False)
        image.set_shape([None, None, 3])
        image = tf.image.resize(image, [image_size, image_size])
    label = tf.one_hot(label, depth=len(CLASSES))
    return image, label


def make_dataset(paths, labels, image_size: int, batch_size: int, seed: int, training: bool, hand_preprocess: bool):
    # 构建 tf.data 数据管道；训练集打乱顺序，验证集保持固定。
    ds = tf.data.Dataset.from_tensor_slices((paths, labels))
    if training:
        ds = ds.shuffle(len(paths), seed=seed, reshuffle_each_iteration=True)
    ds = ds.map(lambda p, y: decode_image(p, y, image_size, hand_preprocess), num_parallel_calls=tf.data.AUTOTUNE)
    return ds.batch(batch_size).prefetch(tf.data.AUTOTUNE)


def make_datasets(data_dir: Path, image_size: int, batch_size: int, seed: int, validation_split: float, hand_preprocess: bool):
    # 同时创建训练集和验证集，并打印各类别样本数量，便于检查数据是否均衡。
    require_tf()
    train_paths, train_labels, val_paths, val_labels = split_image_paths(data_dir, validation_split, seed)
    train_counts = {name: train_labels.count(i) for i, name in enumerate(CLASSES)}
    val_counts = {name: val_labels.count(i) for i, name in enumerate(CLASSES)}
    print(f"Training split: {train_counts}")
    print(f"Validation split: {val_counts}")
    train_ds = make_dataset(train_paths, train_labels, image_size, batch_size, seed, training=True, hand_preprocess=hand_preprocess)
    val_ds = make_dataset(val_paths, val_labels, image_size, batch_size, seed, training=False, hand_preprocess=hand_preprocess)
    return train_ds, val_ds, train_counts, val_counts


def build_model(image_size: int, learning_rate: float):
    # 从零搭建 CNN 卷积神经网络，不使用任何预训练模型。
    require_tf()
    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input(shape=(image_size, image_size, 3)),
            tf.keras.layers.Rescaling(1.0 / 255),  # 将像素值从 0-255 归一化到 0-1。
            # 数据增强：模拟手势的轻微翻转、旋转、移动、缩放和光照变化，提升泛化能力。
            tf.keras.layers.RandomFlip("horizontal"),
            tf.keras.layers.RandomRotation(0.03),
            tf.keras.layers.RandomTranslation(0.04, 0.04),
            tf.keras.layers.RandomZoom(0.06),
            tf.keras.layers.RandomBrightness(0.16, value_range=(0, 1)),
            tf.keras.layers.RandomContrast(0.18),
            # 三组卷积层和池化层用于提取手指边缘、掌心轮廓等局部视觉特征。
            tf.keras.layers.Conv2D(32, 3, padding="same", activation="relu"),
            tf.keras.layers.MaxPooling2D(),
            tf.keras.layers.Conv2D(64, 3, padding="same", activation="relu"),
            tf.keras.layers.MaxPooling2D(),
            tf.keras.layers.Conv2D(128, 3, padding="same", activation="relu"),
            tf.keras.layers.MaxPooling2D(),
            tf.keras.layers.Flatten(),  # 将二维特征图展平成一维向量。
            tf.keras.layers.Dropout(0.35),  # 防止模型只记住训练集，降低过拟合风险。
            tf.keras.layers.Dense(128, activation="relu"),
            tf.keras.layers.Dense(len(CLASSES), activation="softmax"),  # 输出三类手势的概率。
        ]
    )
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate),
        loss="categorical_crossentropy",
        metrics=["accuracy"],
    )
    return model


def plot_history(history, out_path: Path):
    # 保存训练曲线，答辩时可以展示准确率和损失如何变化。
    plt.figure(figsize=(8, 4))
    plt.subplot(1, 2, 1)
    plt.plot(history.history["accuracy"], label="train")
    plt.plot(history.history["val_accuracy"], label="val")
    plt.title("Accuracy")
    plt.xlabel("Epoch")
    plt.legend()
    plt.subplot(1, 2, 2)
    plt.plot(history.history["loss"], label="train")
    plt.plot(history.history["val_loss"], label="val")
    plt.title("Loss")
    plt.xlabel("Epoch")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=160)
    plt.close()


def save_keras_model(model, target_path: Path):
    # 先保存到临时目录再复制到目标路径，减少 Windows 中文路径导致的保存问题。
    target_path = Path(target_path)
    target_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="rps_keras_save_") as tmp_dir:
        tmp_path = Path(tmp_dir) / target_path.name
        model.save(str(tmp_path))
        shutil.copy2(tmp_path, target_path)


def compute_confusion_matrix(model, dataset):
    # 混淆矩阵用于观察每个真实类别被预测成了什么类别。
    matrix = np.zeros((len(CLASSES), len(CLASSES)), dtype=int)
    for images, labels in dataset:
        predictions = model.predict(images, verbose=0)
        true_ids = np.argmax(labels.numpy(), axis=1)
        pred_ids = np.argmax(predictions, axis=1)
        for true_id, pred_id in zip(true_ids, pred_ids):
            matrix[int(true_id), int(pred_id)] += 1
    return matrix


def train_model(args):
    # 完整训练流程：加载数据、构建模型、训练、评估并保存模型和报告。
    require_tf()
    random.seed(args.seed)
    np.random.seed(args.seed)
    tf.random.set_seed(args.seed)
    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    counts = class_counts(data_dir)
    if min(counts.values(), default=0) < 10:
        raise SystemExit(
            f"Not enough images in {data_dir}. Collect at least 30 per class first. Counts: {counts}"
        )

    train_ds, val_ds, train_counts, val_counts = make_datasets(
        data_dir, args.image_size, args.batch_size, args.seed, args.validation_split, args.hand_preprocess
    )
    model = build_model(args.image_size, args.learning_rate)
    callbacks = [
        # 验证准确率长时间不提升时提前停止，避免过拟合。
        tf.keras.callbacks.EarlyStopping(monitor="val_accuracy", mode="max", patience=8, restore_best_weights=True),
        # 验证损失不下降时降低学习率，让后期训练更稳定。
        tf.keras.callbacks.ReduceLROnPlateau(monitor="val_loss", factor=0.5, patience=4),
    ]
    history = model.fit(train_ds, validation_data=val_ds, epochs=args.epochs, callbacks=callbacks)
    loss, acc = model.evaluate(val_ds, verbose=0)
    confusion = compute_confusion_matrix(model, val_ds)
    save_keras_model(model, output_dir / "model.keras")
    shutil.copy2(output_dir / "model.keras", output_dir / "best_model.keras")
    (output_dir / "labels.txt").write_text("\n".join(CLASSES), encoding="utf-8")
    plot_history(history, output_dir / "training_curve.png")

    report = {
        # 训练报告记录数据量、准确率、混淆矩阵和是否使用预训练模型，方便答辩说明。
        "time": datetime.now().isoformat(timespec="seconds"),
        "data_dir": str(data_dir),
        "counts": counts,
        "train_counts": train_counts,
        "validation_counts": val_counts,
        "classes": CLASSES,
        "image_size": args.image_size,
        "epochs_ran": len(history.history["loss"]),
        "validation_loss": float(loss),
        "validation_accuracy": float(acc),
        "confusion_matrix_rows_true_columns_pred": confusion.tolist(),
        "hand_preprocess": bool(args.hand_preprocess),
        "pretrained_model": False,
        "framework": f"TensorFlow {tf.__version__}",
    }
    (output_dir / "run_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Saved model to {output_dir / 'model.keras'}")
    print(f"Validation accuracy: {acc:.3f}")


def load_labels(output_dir: Path):
    # 读取训练时保存的类别标签；如果文件不存在，则使用默认类别顺序。
    label_file = output_dir / "labels.txt"
    if label_file.exists():
        return [line.strip() for line in label_file.read_text(encoding="utf-8").splitlines() if line.strip()]
    return CLASSES


def winning_response(label: str):
    # 根据识别结果返回电脑应该出的获胜手势。
    if label not in WINNING_MOVE:
        return "waiting", "请把清晰手势放在绿色方框内。"
    response = WINNING_MOVE[label]
    return response, f"{DISPLAY_NAMES[response]} 可以赢 {DISPLAY_NAMES[label]}。"


def load_predictor(args):
    # 加载已经训练好的 Keras 模型和标签文件。
    require_tf()
    output_dir = Path(args.output_dir)
    model_path = Path(args.model) if args.model else output_dir / "model.keras"
    if not model_path.exists():
        raise SystemExit(f"Model not found: {model_path}. Train first.")
    labels = load_labels(output_dir)
    model = tf.keras.models.load_model(model_path)
    return model, labels


def skin_fraction(roi, cv2):
    # 估计 ROI 中肤色像素比例，用于辅助处理剪刀和布容易混淆的情况。
    ycrcb = cv2.cvtColor(roi, cv2.COLOR_BGR2YCrCb)
    lower = np.array([0, 133, 77], dtype=np.uint8)
    upper = np.array([255, 173, 127], dtype=np.uint8)
    mask = cv2.inRange(ycrcb, lower, upper)
    mask = cv2.medianBlur(mask, 5)
    return cv2.countNonZero(mask) / float(mask.shape[0] * mask.shape[1])


def predict_roi_probs(model, roi, cv2, args):
    # 对单张 ROI 图像进行预处理，并输出 rock/paper/scissors 三类概率。
    img = prepare_roi_for_model(roi, cv2, args)
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    return model.predict(np.expand_dims(rgb.astype("float32"), axis=0), verbose=0)[0]


def predict_rois_probs(model, rois, cv2, args):
    # 对多帧 ROI 批量预测，UI 最终锁定结果时会对这些概率取平均。
    batch = []
    for roi in rois:
        img = prepare_roi_for_model(roi, cv2, args)
        rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        batch.append(rgb.astype("float32"))
    if not batch:
        return np.zeros((1, len(CLASSES)), dtype=np.float32)
    return model.predict(np.stack(batch, axis=0), verbose=0)


def choose_label(labels, probs, roi, cv2, args):
    # 从概率中选择最终类别；置信度太低时返回 uncertain。
    idx = int(np.argmax(probs))
    confidence = float(probs[idx])
    label = labels[idx]
    corrected = False

    if getattr(args, "scissors_correction", True) and label == "paper" and "scissors" in labels and roi is not None:
        # 剪刀和布都属于张开手指的形态，容易混淆；这里做一个保守的剪刀修正。
        scissors_index = labels.index("scissors")
        scissors_prob = float(probs[scissors_index])
        skin_threshold = getattr(args, "scissors_skin_threshold", 0.04)
        min_prob = getattr(args, "scissors_min_prob", 0.24)
        if skin_fraction(roi, cv2) >= skin_threshold and scissors_prob >= min_prob:
            label = "scissors"
            confidence = scissors_prob
            corrected = True

    if confidence < getattr(args, "min_confidence", 0.55) and not corrected:
        label = "uncertain"
    return label, confidence, probs


def classify_roi(model, labels, roi, cv2, args, recent_probs):
    # 实时预测时使用最近多帧平均概率，让结果更平滑。
    probs = predict_roi_probs(model, roi, cv2, args)
    recent_probs.append(probs)
    smooth_probs = np.mean(np.array(recent_probs), axis=0)
    label, confidence, smooth_probs = choose_label(labels, smooth_probs, roi, cv2, args)
    return label, confidence, smooth_probs


def predict_camera(args):
    # 调试用的实时摄像头预测窗口；最终展示主要使用 run_ui。
    require_tf()
    cv2 = import_cv2()
    model, labels = load_predictor(args)
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        raise SystemExit(f"Could not open camera {args.camera}.")
    print("Running camera prediction. Press q to quit.")
    recent_probs = deque(maxlen=max(1, args.smooth))

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        frame = cv2.flip(frame, 1)
        x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
        roi = frame[y1:y2, x1:x2]
        label, confidence, probs = classify_roi(model, labels, roi, cv2, args, recent_probs)
        response, reason = winning_response(label)

        cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 2)
        text = f"You: {label} {confidence:.2f}"
        cv2.putText(frame, text, (20, 38), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (30, 240, 80), 2)
        cv2.putText(frame, f"Win with: {response}", (20, 72), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (80, 220, 255), 2)
        cv2.putText(frame, reason, (20, 104), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (240, 240, 240), 2)
        y = 138
        for name, p in zip(labels, probs):
            cv2.putText(frame, f"{name:<8} {p:.2f}", (20, y), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (240, 240, 240), 2)
            y += 28
        cv2.imshow("gesture predictor", frame)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


def draw_probability_bars(canvas, labels, probs):
    # 在 Tkinter 侧边栏绘制三类概率条。
    canvas.delete("all")
    width = int(canvas["width"])
    colors = {"rock": "#7dd3fc", "paper": "#86efac", "scissors": "#fca5a5"}
    for index, (name, prob) in enumerate(zip(labels, probs)):
        top = 12 + index * 48
        bar_width = int((width - 130) * float(prob))
        canvas.create_text(10, top + 13, anchor="w", text=DISPLAY_NAMES.get(name, name), fill="#dbeafe", font=("Microsoft YaHei UI", 11, "bold"))
        canvas.create_rectangle(96, top, width - 20, top + 26, fill="#1f2937", outline="#334155")
        canvas.create_rectangle(96, top, 96 + bar_width, top + 26, fill=colors.get(name, "#93c5fd"), outline="")
        canvas.create_text(width - 16, top + 13, anchor="e", text=f"{prob:.0%}", fill="#e5e7eb", font=("Microsoft YaHei UI", 10))


def run_ui(args):
    # 图形化比赛界面：倒计时、采样、锁定识别结果，并显示电脑获胜手势。
    require_tf()
    cv2 = import_cv2()
    try:
        import tkinter as tk
        from PIL import Image, ImageTk
    except ImportError as exc:
        raise SystemExit("Tkinter and Pillow are required for UI mode.") from exc

    model, labels = load_predictor(args)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        raise SystemExit(f"Could not open camera {args.camera}.")

    root = tk.Tk()
    root.title("石头剪刀布手势比赛")
    root.configure(bg="#0f172a")
    root.minsize(980, 620)

    video_panel = tk.Label(root, bg="#020617")
    video_panel.pack(side="left", fill="both", expand=True, padx=(18, 10), pady=18)

    side = tk.Frame(root, bg="#0f172a", width=330)
    side.pack(side="right", fill="y", padx=(8, 18), pady=18)
    side.pack_propagate(False)

    tk.Label(side, text="石头剪刀布比赛", fg="#f8fafc", bg="#0f172a", font=("Microsoft YaHei UI", 24, "bold")).pack(anchor="w")
    tk.Label(
        side,
        text="点击开始，把手势放进绿色方框。出现“出拳！”后保持一下，随后锁定结果。",
        fg="#94a3b8",
        bg="#0f172a",
        wraplength=300,
        justify="left",
        font=("Microsoft YaHei UI", 11),
    ).pack(anchor="w", pady=(4, 22))

    countdown_var = tk.StringVar(value="准备")
    tk.Label(side, textvariable=countdown_var, fg="#fef3c7", bg="#0f172a", font=("Microsoft YaHei UI", 40, "bold")).pack(anchor="center", pady=(0, 12))

    start_button = tk.Button(
        side,
        text="开始比赛",
        command=lambda: start_round(),
        bg="#2563eb",
        fg="#ffffff",
        activebackground="#1d4ed8",
        activeforeground="#ffffff",
        bd=0,
        padx=18,
        pady=10,
        font=("Microsoft YaHei UI", 13, "bold"),
    )
    start_button.pack(fill="x", pady=(0, 22))

    tk.Label(side, text="你的手势", fg="#93c5fd", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w")
    gesture_emoji_var = tk.StringVar(value=GESTURE_EMOJI["waiting"])
    tk.Label(side, textvariable=gesture_emoji_var, fg="#f8fafc", bg="#0f172a", font=("Segoe UI Emoji", 78)).pack(anchor="center")
    gesture_var = tk.StringVar(value="等待")
    tk.Label(side, textvariable=gesture_var, fg="#f8fafc", bg="#0f172a", font=("Microsoft YaHei UI", 18, "bold")).pack(anchor="center", pady=(0, 12))

    tk.Label(side, text="电脑出招", fg="#fbbf24", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w")
    response_emoji_var = tk.StringVar(value=GESTURE_EMOJI["waiting"])
    tk.Label(side, textvariable=response_emoji_var, fg="#fde68a", bg="#0f172a", font=("Segoe UI Emoji", 78)).pack(anchor="center")
    response_var = tk.StringVar(value="等待")
    tk.Label(side, textvariable=response_var, fg="#fde68a", bg="#0f172a", font=("Microsoft YaHei UI", 18, "bold")).pack(anchor="center", pady=(0, 10))

    reason_var = tk.StringVar(value="点击开始比赛。")
    tk.Label(
        side,
        textvariable=reason_var,
        fg="#cbd5e1",
        bg="#0f172a",
        wraplength=300,
        justify="left",
        font=("Microsoft YaHei UI", 12),
    ).pack(anchor="w", pady=(0, 22))

    confidence_var = tk.StringVar(value="置信度：--")
    tk.Label(side, textvariable=confidence_var, fg="#a7f3d0", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w", pady=(0, 8))
    bars = tk.Canvas(side, width=300, height=160, bg="#0f172a", highlightthickness=0)
    bars.pack(anchor="w", pady=(0, 22))

    tk.Label(
        side,
        text="提示：出拳后保持约 1 秒，不要马上收手；尽量保持和采集数据时相同的距离、角度和光照。",
        fg="#64748b",
        bg="#0f172a",
        wraplength=300,
        justify="left",
        font=("Microsoft YaHei UI", 10),
    ).pack(anchor="w", side="bottom")

    recent_probs = deque(maxlen=max(1, args.smooth))
    round_rois = deque(maxlen=max(1, args.final_window))
    running = {"value": True}
    state = {"phase": "ready", "deadline": 0.0, "capture_deadline": 0.0, "last_roi": None}

    def show_frame(frame):
        # OpenCV 图像是 BGR，Tkinter 显示前需要转为 RGB。
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        image = Image.fromarray(rgb_frame)
        image.thumbnail((720, 560), Image.Resampling.LANCZOS)
        photo = ImageTk.PhotoImage(image=image)
        video_panel.configure(image=photo)
        video_panel.image = photo

    def set_waiting_visuals():
        # 空闲状态：不显示预测结果，等待用户点击开始。
        countdown_var.set("准备")
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        gesture_var.set("等待")
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        response_var.set("等待")
        reason_var.set("点击开始比赛。")
        confidence_var.set("置信度：--")
        draw_probability_bars(bars, labels, np.zeros(len(labels)))

    def set_result_visuals(label, confidence, probs, random_result=False):
        # 锁定结果后，更新用户手势、电脑出拳、置信度和概率条。
        response, reason = winning_response(label)
        gesture_emoji_var.set(GESTURE_EMOJI.get(label, GESTURE_EMOJI["uncertain"]))
        if random_result:
            gesture_var.set(f"随机：{DISPLAY_NAMES.get(label, label)}")
        else:
            gesture_var.set(f"{DISPLAY_NAMES.get(label, label)}  {confidence:.0%}")
        response_emoji_var.set(GESTURE_EMOJI.get(response, GESTURE_EMOJI["waiting"]))
        response_var.set(DISPLAY_NAMES.get(response, response))
        reason_var.set("未能稳定识别，已随机选择一个结果。" if random_result else reason)
        confidence_var.set("置信度：随机结果" if random_result else f"置信度：{confidence:.1%}")
        draw_probability_bars(bars, labels, probs)

    def start_round():
        # 开始新一局：清空上一局缓存，进入倒计时阶段。
        recent_probs.clear()
        round_rois.clear()
        state["phase"] = "countdown"
        state["deadline"] = perf_counter() + max(0.5, float(args.countdown))
        state["capture_deadline"] = 0.0
        state["last_roi"] = None
        countdown_var.set(str(int(np.ceil(args.countdown))))
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        gesture_var.set("结果锁定后显示")
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        response_var.set("结果锁定后显示")
        reason_var.set("请把手势保持在绿色方框内，倒计时结束后才显示结果。")
        confidence_var.set("置信度：隐藏")
        draw_probability_bars(bars, labels, np.zeros(len(labels)))
        start_button.configure(text="倒计时中", state="disabled", bg="#475569")

    def lock_round(frame):
        # 倒计时结束后锁定结果：对采样到的多帧预测概率取平均。
        if round_rois:
            predictions = predict_rois_probs(model, list(round_rois), cv2, args)
            final_probs = np.mean(predictions, axis=0)
        else:
            final_probs = np.zeros(len(labels), dtype=np.float32)
        label, confidence, final_probs = choose_label(labels, final_probs, state["last_roi"], cv2, args)
        random_result = label == "uncertain"
        if random_result:
            # 如果没有稳定识别出来，就随机给出一个手势，保证比赛流程有结果。
            label = random.choice(list(labels))
            confidence = 0.0
            final_probs = np.zeros(len(labels), dtype=np.float32)
            final_probs[labels.index(label)] = 1.0
        response, reason = winning_response(label)
        set_result_visuals(label, confidence, final_probs, random_result=random_result)
        result = {
            "time": datetime.now().isoformat(timespec="seconds"),
            "label": label,
            "display_label": DISPLAY_NAMES.get(label, label),
            "response": response,
            "display_response": DISPLAY_NAMES.get(response, response),
            "confidence": float(confidence),
            "random_result": bool(random_result),
            "probabilities": {name: float(prob) for name, prob in zip(labels, final_probs)},
            "sampled_frames": len(round_rois),
            "capture_duration": float(args.capture_duration),
        }
        try:
            (output_dir / "latest_ui_result.json").write_text(
                json.dumps(result, ensure_ascii=False, indent=2),
                encoding="utf-8",
            )
            if state["last_roi"] is not None:
                # 保存调试图：原始 ROI 和模型真正看到的预处理后输入。
                save_jpg(output_dir / "latest_ui_roi.jpg", state["last_roi"], cv2)
                save_jpg(output_dir / "latest_ui_model_input.jpg", prepare_roi_for_model(state["last_roi"], cv2, args), cv2)
            save_jpg(output_dir / "latest_ui_frame.jpg", frame, cv2)
        except OSError as exc:
            print(f"保存本局调试图片失败：{exc}")
        countdown_var.set("已锁定")
        if not random_result:
            reason_var.set(reason)
        start_button.configure(text="下一局", state="normal", bg="#2563eb")
        state["phase"] = "locked"

        show_frame(frame)

    def start_capture():
        # 倒计时结束后短暂采样，避免用户最后一秒出手造成单帧模糊。
        round_rois.clear()
        state["phase"] = "capture"
        state["capture_deadline"] = perf_counter() + max(0.05, float(args.capture_duration))
        countdown_var.set("出拳！")
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        gesture_var.set("正在锁定")
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        response_var.set("正在锁定")
        reason_var.set("请保持刚出的手势不动，正在采集稳定画面。")
        confidence_var.set("置信度：隐藏")
        draw_probability_bars(bars, labels, np.zeros(len(labels)))

    def on_close():
        # 关闭窗口时释放摄像头资源。
        running["value"] = False
        cap.release()
        root.destroy()

    def update_frame():
        # UI 主循环：持续读取摄像头画面，并根据阶段更新界面状态。
        if not running["value"]:
            return
        if state["phase"] == "locked":
            root.after(80, update_frame)
            return
        ok, frame = cap.read()
        if ok:
            frame = cv2.flip(frame, 1)
            x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
            roi = frame[y1:y2, x1:x2]
            cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 3)

            if state["phase"] == "countdown":
                remaining = state["deadline"] - perf_counter()
                if remaining <= 0:
                    start_capture()
                    cv2.putText(frame, "!", (42, 112), cv2.FONT_HERSHEY_SIMPLEX, 3.0, (80, 220, 255), 7)
                    show_frame(frame)
                    root.after(80, update_frame)
                    return
                count = int(np.ceil(remaining))
                countdown_var.set(str(count))
                gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
                gesture_var.set("结果锁定后显示")
                response_emoji_var.set(GESTURE_EMOJI["waiting"])
                response_var.set("结果锁定后显示")
                reason_var.set("请保持手势，锁定前不显示预测结果。")
                confidence_var.set("置信度：隐藏")
                draw_probability_bars(bars, labels, np.zeros(len(labels)))
                cv2.putText(frame, str(count), (42, 112), cv2.FONT_HERSHEY_SIMPLEX, 3.0, (80, 220, 255), 7)
            elif state["phase"] == "capture":
                # 采样阶段只收集图像，不提前把预测结果显示出来。
                round_rois.append(roi.copy())
                state["last_roi"] = roi.copy()
                countdown_var.set("出拳！")
                gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
                gesture_var.set("正在锁定")
                response_emoji_var.set(GESTURE_EMOJI["waiting"])
                response_var.set("正在锁定")
                reason_var.set("正在采集出拳后的稳定画面。")
                confidence_var.set("置信度：隐藏")
                draw_probability_bars(bars, labels, np.zeros(len(labels)))
                cv2.putText(frame, "!", (42, 112), cv2.FONT_HERSHEY_SIMPLEX, 3.0, (80, 220, 255), 7)
                if perf_counter() >= state["capture_deadline"]:
                    lock_round(frame)
                    root.after(80, update_frame)
                    return
            else:
                set_waiting_visuals()

            show_frame(frame)
        root.after(20, update_frame)

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.bind("<space>", lambda _event: start_round())
    set_waiting_visuals()
    update_frame()
    root.mainloop()


def show_status(args):
    # 输出当前项目状态：数据集数量、模型路径和是否存在模型。
    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)
    print("Project: self-trained rock-paper-scissors gesture classifier")
    print(f"Data dir: {data_dir}")
    print(f"Counts: {class_counts(data_dir)}")
    print(f"Output dir: {output_dir}")
    print(f"Model exists: {(output_dir / 'model.keras').exists()}")
    print("No pretrained model is used. The CNN starts from random weights.")


def parse_args():
    # 命令行参数。默认 mode 为 ui，所以 VS Code 直接运行脚本会打开图形界面。
    parser = argparse.ArgumentParser(
        description="Collect your own hand images, train a TensorFlow CNN from scratch, and run live prediction."
    )
    parser.add_argument("mode", nargs="?", default="ui", choices=["collect", "train", "predict", "ui", "status"])
    parser.add_argument("--data-dir", default=DEFAULT_DATA_DIR)
    parser.add_argument("--output-dir", default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--image-size", type=int, default=128)
    parser.add_argument("--camera", type=int, default=0)
    parser.add_argument("--roi-ratio", type=float, default=0.62)
    parser.add_argument("--class-name", choices=CLASSES + ["all"], default="all")
    parser.add_argument("--samples", type=int, default=160)
    parser.add_argument("--target-count", type=int, default=0)
    parser.add_argument("--replace-existing", action=argparse.BooleanOptionalAction, default=False)
    parser.add_argument("--every", type=int, default=2)
    parser.add_argument("--epochs", type=int, default=45)
    parser.add_argument("--batch-size", type=int, default=16)
    parser.add_argument("--learning-rate", type=float, default=7e-4)
    parser.add_argument("--validation-split", type=float, default=0.2)
    parser.add_argument("--smooth", type=int, default=8)
    parser.add_argument("--min-confidence", type=float, default=0.55)
    parser.add_argument("--countdown", type=float, default=3.0)
    parser.add_argument("--capture-duration", type=float, default=0.8)
    parser.add_argument("--final-window", type=int, default=24)
    parser.add_argument("--hand-preprocess", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--scissors-correction", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--scissors-skin-threshold", type=float, default=0.04)
    parser.add_argument("--scissors-min-prob", type=float, default=0.24)
    parser.add_argument("--seed", type=int, default=11)
    parser.add_argument("--model", default="")
    return parser.parse_args()


def main():
    # 程序入口：根据 mode 分发到采集、训练、预测、UI 或状态查看功能。
    args = parse_args()
    if args.mode == "collect":
        collect_data(args)
    elif args.mode == "train":
        train_model(args)
    elif args.mode == "predict":
        predict_camera(args)
    elif args.mode == "ui":
        run_ui(args)
    else:
        show_status(args)


if __name__ == "__main__":
    main()
