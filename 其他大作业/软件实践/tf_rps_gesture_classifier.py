# 导入 argparse，用来解析命令行参数。
import argparse
# 导入 json，用来读写训练报告和 UI 结果文件。
import json
# 导入 os，用来设置环境变量。
import os
# 导入 random，用来在无法识别时随机给出结果。
import random
# 导入 shutil，用来移动旧数据到备份目录。
import shutil
# 导入 tempfile，用来保存模型时创建临时文件。
import tempfile
# 从 collections 导入 deque，用作固定长度的预测缓存。
from collections import deque
# 从 datetime 导入 datetime，用来生成时间戳和记录结果时间。
from datetime import datetime
# 从 pathlib 导入 Path，用面向对象的方式处理文件路径。
from pathlib import Path
# 从 time 导入 perf_counter，用高精度计时控制倒计时。
from time import perf_counter
# 设置 TensorFlow 日志等级，减少无关提示输出。
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "1")

# 导入 matplotlib，用来保存训练曲线图。
import matplotlib

# 切换到 Agg 后端，让程序可以在无窗口环境保存图片。
matplotlib.use("Agg")
# 导入 pyplot，用来绘制准确率和损失曲线。
import matplotlib.pyplot as plt
# 导入 numpy，用来处理图像数组和概率数组。
import numpy as np

# 先把 TensorFlow 占位为空，后面需要时再真正导入。
tf = None


#示当前 Python 文件所在的目录
PROJECT_DIR = Path(__file__).resolve().parent
#默认数据集目录
DEFAULT_DATA_DIR = PROJECT_DIR / "gesture_data"
#默认输出目录
DEFAULT_OUTPUT_DIR = PROJECT_DIR / "outputs" / "rps_gesture"

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

WINNING_MOVE = {"rock": "paper", "paper": "scissors", "scissors": "rock"}

# 定义 require_tf 函数：按需加载 TensorFlow，避免程序启动时立刻变慢。
def require_tf():
    # TensorFlow 启动较慢，所以只在训练或预测真正需要时再导入。
    global tf
    if tf is None:
        import tensorflow as loaded_tf
        tf = loaded_tf
    return tf


# 定义 import_cv2 函数：安全导入 OpenCV，摄像头和图像处理都依赖它。
def import_cv2():
    try:
        import cv2
        return cv2
    except ImportError as exc:
        raise SystemExit(
            "OpenCV is required for camera collection/prediction.\n"
            "Install it with: python -m pip install opencv-python"
        ) from exc


# 定义 ensure_dirs 函数：确保三类手势的数据文件夹都存在。
def ensure_dirs(data_dir: Path):
    # 创建 rock、paper、scissors 三个数据目录。
    for name in CLASSES:
        # 创建目录；如果已经存在就直接复用。
        (data_dir / name).mkdir(parents=True, exist_ok=True)


# 定义 class_counts 函数：统计每类手势目前已有多少张图片。
def class_counts(data_dir: Path) -> dict[str, int]:
    # 统计每个类别已有多少张采集图片。
    # 计算并保存 `counts`，供后续逻辑使用。
    counts = {}
    # 遍历 `name`，逐项执行下面的逻辑。
    for name in CLASSES:
        # 计算并保存 `folder`，供后续逻辑使用。
        folder = data_dir / name
        # 计算并保存 `counts[name]`，供后续逻辑使用。
        counts[name] = len(image_files(folder)) if folder.exists() else 0
    # 返回 `counts`，把结果交给调用者。
    return counts


# 定义 archive_class_images 函数：把旧图片移到备份目录，方便重新采集。
def archive_class_images(data_dir: Path, class_name: str):
    # 重新采集某类手势时，先把旧图片备份，避免直接丢失数据。
    folder = data_dir / class_name
    # 计算并保存 `files`，供后续逻辑使用。
    files = image_files(folder) if folder.exists() else []
    # 判断条件 `not files` 是否成立。
    if not files:
        # 在控制台输出当前进度或状态信息。
        print(f"{class_name} has no images to archive.")
        # 返回 `None`，把结果交给调用者。
        return None

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = data_dir.parent / "gesture_data_backups" / f"{class_name}_{timestamp}"
    # 创建目录；如果已经存在就直接复用。
    backup_dir.mkdir(parents=True, exist_ok=True)
    # 遍历 `path`，逐项执行下面的逻辑。
    for path in files:
        # 把旧图片移动到备份目录。
        shutil.move(str(path), str(backup_dir / path.name))
    # 在控制台输出当前进度或状态信息。
    print(f"Archived {len(files)} old {class_name} images to {backup_dir}")
    return backup_dir

# 定义 image_files 函数：列出目录中的图片样本文件。
def image_files(folder: Path):
    # 只读取常见图片格式，避免把其他文件误当成训练样本。
    # 计算并保存 `suffixes`，供后续逻辑使用。
    suffixes = {".jpg", ".jpeg", ".png", ".bmp"}

    return sorted([path for path in folder.iterdir() if path.suffix.lower() in suffixes])


# 定义 center_square 函数：计算画面中心的正方形采集区域。
def center_square(frame, size_ratio: float):
    # 计算画面中心的正方形 ROI，用户把手势放进这个绿色框。
    h, w = frame.shape[:2]
    # 计算正方形边长
    side = int(min(h, w) * size_ratio)
    #计算左上角坐标
    x1 = (w - side) // 2
    y1 = (h - side) // 2
    return x1, y1, x1 + side, y1 + side


# 定义 square_crop_with_margin 函数：按手部轮廓裁出带边缘的正方形图片。
def square_crop_with_margin(image, x, y, w, h, margin_ratio=0.28):
    # 根据手部轮廓重新裁剪成正方形，并留出少量边缘，避免切掉手指。
    height, width = image.shape[:2]
    #算出手部矩形的中心点。
    cx = x + w / 2
    cy = y + h / 2
    # 决定最终裁剪框的边长。
    side = int(max(w, h) * (1.0 + margin_ratio))
    side = max(side, 24)
    #算出正方形裁剪框的左上角和右下角
    x1 = int(round(cx - side / 2))
    y1 = int(round(cy - side / 2))
    x2 = x1 + side
    y2 = y1 + side

    #处理越界。
    pad_left = max(0, -x1)
    pad_top = max(0, -y1)
    pad_right = max(0, x2 - width)
    pad_bottom = max(0, y2 - height)
    #如果任何方向越界，就给图像补边。
    if pad_left or pad_top or pad_right or pad_bottom:
        image = cv2_copy_border(image, pad_top, pad_bottom, pad_left, pad_right)
        x1 += pad_left
        y1 += pad_top
        x2 += pad_left
        y2 += pad_top
    return image[y1:y2, x1:x2]


# 定义 cv2_copy_border 函数：给超出边界的裁剪区域补黑边。
def cv2_copy_border(image, top, bottom, left, right):
    # 裁剪框超出图像边界时，用黑色像素补齐边缘。
    cv2 = import_cv2()
    #用固定颜色补边。
    return cv2.copyMakeBorder(image, top, bottom, left, right, cv2.BORDER_CONSTANT, value=(0, 0, 0))


# 定义 hand_mask 函数：用肤色阈值提取手部候选区域。
def hand_mask(roi, cv2):
    # 使用 YCrCb 肤色阈值提取手部候选区域，目的是减少背景对模型的干扰。
    # 对图像做高斯模糊，减少噪声。
    blurred = cv2.GaussianBlur(roi, (5, 5), 0)
    # 转换图像颜色空间。肤色在 YCrCb 里通常更容易分离
    ycrcb = cv2.cvtColor(blurred, cv2.COLOR_BGR2YCrCb)
    # 保留颜色值落在指定范围内的像素。
    ycrcb_mask = cv2.inRange(
        ycrcb,
        np.array([35, 132, 78], dtype=np.uint8),
        np.array([245, 180, 135], dtype=np.uint8),
    )
    #创建形态学操作用的卷积核
    kernel = np.ones((5, 5), np.uint8)
    #做开运算，用于去掉小的白色噪点
    mask = cv2.morphologyEx(ycrcb_mask, cv2.MORPH_OPEN, kernel, iterations=1)
    #做闭运算，填补白色区域里的小黑洞
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel, iterations=2)
    #中值滤波
    mask = cv2.medianBlur(mask, 5)

    return mask


# 定义 preprocess_hand_roi 函数：把摄像头 ROI 转成模型可用的手部输入图。
def preprocess_hand_roi(roi, cv2, image_size: int):
    # 训练和预测共用同一套预处理：找手部、去背景、裁成正方形、缩放到模型输入尺寸。
    # 把图像缩放到指定尺寸。
    resized_fallback = cv2.resize(roi, (image_size, image_size))
    mask = hand_mask(roi, cv2)
    # 从掩码中找出候选手部轮廓。
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    # 计算最小区域
    min_area = roi.shape[0] * roi.shape[1] * 0.015
    # 计算轮廓面积，大于最小面积的才算。
    contours = [contour for contour in contours if cv2.contourArea(contour) >= min_area]
    if not contours:
        return resized_fallback
    height, width = roi.shape[:2]

    # 定义 contour_score 函数：给每个候选轮廓打分，帮助选出真正的手。
    def contour_score(contour):
        # 如果脸或衣服也进入绿色框，可能出现多个轮廓；这里优先选择更靠中下方的主轮廓。
        # 计算轮廓面积。
        area = cv2.contourArea(contour)
        # 计算轮廓外接矩形。
        x, y, w, h = cv2.boundingRect(contour)
        center_y = (y + h / 2) / max(1, height)
        center_x = (x + w / 2) / max(1, width)
        center_bonus = 1.0 - min(1.0, abs(center_x - 0.5) * 1.2)
        lower_bonus = 0.65 + center_y
        return area * lower_bonus * (0.75 + 0.25 * center_bonus)

    contour = max(contours, key=contour_score)
    # 计算轮廓外接矩形。
    x, y, w, h = cv2.boundingRect(contour)
    clean_mask = np.zeros(mask.shape, dtype=np.uint8)
    # 把选中的手部轮廓画到干净掩码上。
    cv2.drawContours(clean_mask, [contour], -1, 255, thickness=cv2.FILLED)
    # 膨胀掩码，保留更多手部边缘。
    clean_mask = cv2.dilate(clean_mask, np.ones((7, 7), np.uint8), iterations=1)
    #创建一张和 roi 一样大小、一样通道数的全黑图片
    foreground = np.zeros_like(roi)
    #根据掩码把手部区域从原图复制到黑图上。
    foreground[clean_mask > 0] = roi[clean_mask > 0]
    #从 foreground 里裁剪出手部区域
    cropped = square_crop_with_margin(foreground, x, y, w, h)
    return cv2.resize(cropped, (image_size, image_size))


# 定义 prepare_roi_for_model 函数：根据参数决定是否做手部前景预处理。
def prepare_roi_for_model(roi, cv2, args):
    # 默认启用手部前景提取；如需对比原图训练，可通过 --no-hand-preprocess 关闭。
    if getattr(args, "hand_preprocess", True):
        return preprocess_hand_roi(roi, cv2, args.image_size)
    return cv2.resize(roi, (args.image_size, args.image_size))


# 定义 load_image_for_training 函数：读取训练图片并转换成模型训练数组。
def load_image_for_training(path_value, image_size: int, hand_preprocess: bool):
    # TensorFlow 数据管道中调用 OpenCV 读取中文路径图片，并执行和 UI 相同的预处理。
    cv2 = import_cv2()
    # 通过 .item() 取出其中真正保存的 Python 值。
    if hasattr(path_value, "item"):
        path_value = path_value.item()
    #将路径统一转换为 Python 字符串。
    path = path_value.decode("utf-8") if isinstance(path_value, bytes) else str(path_value)
    #使用 np.fromfile 读取图片原始字节
    data = np.fromfile(path, dtype=np.uint8)
    # 把图片字节解码成 OpenCV 图像。
    image = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if image is None:
        raise ValueError(f"Could not read image: {path}")
    if hand_preprocess:
        image = preprocess_hand_roi(image, cv2, image_size)
    else:
        image = cv2.resize(image, (image_size, image_size))
    # 转换图像颜色空间。
    rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    return rgb.astype("float32")


# 定义 collect_one_class 函数：打开摄像头采集某一个类别的手势图片。
def collect_one_class(args, class_name: str):
    # 采集某一个类别的手势图片，例如 rock、paper 或 scissors。
    cv2 = import_cv2()
    data_dir = Path(args.data_dir)
    ensure_dirs(data_dir)
    save_dir = data_dir / class_name
    #打开摄像头
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        raise SystemExit(f"Could not open camera {args.camera}.")

    # 统计当前类别目录里已经有多少张图片
    existing = len(image_files(save_dir))
    # 计算并保存 `saved`，供后续逻辑使用。
    saved = 0
    # 本次运行已经保存了多少张图片
    frame_count = 0
    paused = True
    print(f"Collecting '{class_name}' into {save_dir}")
    print("Put your hand in the green square. Press SPACE to start/pause, q to quit.")

    # 只要 `saved < args.samples` 成立，就持续循环执行。
    while saved < args.samples:
        # 从摄像头读取一帧画面。
        ok, frame = cap.read()
        if not ok:
            break
        # 水平翻转画面，让摄像头预览像照镜子一样。
        frame = cv2.flip(frame, 1)  
        #根据 roi_ratio 在画面中心计算一个正方形区域
        x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
        #从整帧画面中截取绿色框内部的 ROI。
        roi = frame[y1:y2, x1:x2]
        # 在画面上画出绿色手势采集框。
        cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 2)
        #根据当前是否暂停，显示 PAUSED 或 SAVING。
        status = "PAUSED" if paused else "SAVING"
        text = f"{class_name} {saved}/{args.samples} {status}"
        cv2.putText(frame, text, (20, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (30, 240, 80), 2)
        #显示窗口
        cv2.imshow("gesture collector", frame)

        # 读取键盘按键，用来暂停、继续或退出。
        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break
        if key == 32:
            paused = not paused
        if paused:
            continue
        #累计帧数
        frame_count += 1
        if frame_count % args.every == 0:
            # 每隔几帧保存一张图片，避免连续样本完全重复。
            # 把图像缩放到指定尺寸。
            img = cv2.resize(roi, (args.image_size, args.image_size))
            # 构造输出文件名。
            out = save_dir / f"{class_name}_{existing + saved:04d}.jpg"
            # 把图像编码成 JPG 数据。
            ok, encoded = cv2.imencode(".jpg", img)
            if not ok:
                raise SystemExit(f"Could not encode image for {out}.")
            # 把编码后的图片写入磁盘，兼容中文路径。
            encoded.tofile(str(out))
            saved += 1

    # 释放摄像头资源。
    cap.release()
    # 关闭 OpenCV 创建的所有窗口。
    cv2.destroyAllWindows()
    print(f"Saved {saved} images for '{class_name}'.")


# 定义 save_jpg 函数：用兼容中文路径的方式保存 JPG 图片。
def save_jpg(path: Path, image, cv2):
    # 使用 imencode + tofile 保存图片，可以兼容 Windows 中文路径。
    # 创建目录；如果已经存在就直接复用。
    path.parent.mkdir(parents=True, exist_ok=True)
    # 把图像编码成 JPG 数据。
    ok, encoded = cv2.imencode(".jpg", image)
    # 判断条件 `ok` 是否成立。
    if ok:
        # 把编码后的图片写入磁盘，兼容中文路径。
        encoded.tofile(str(path))


# 定义 collect_data 函数：组织完整的数据采集流程。
def collect_data(args):
    # 数据采集总入口：可以采集全部类别，也可以只重采某一个类别。
    #保存用户最初设置的采样数量
    original_samples = args.samples
    #先把已有图片归档备份，避免新旧数据混在一起。
    if args.replace_existing:
        selected_classes = CLASSES if args.class_name == "all" else [args.class_name]
        for name in selected_classes:
            #备份旧图片
            archive_class_images(Path(args.data_dir), name)

    if args.target_count > 0:
        # 统计当前数据集中每个类别已有多少张图片。
        counts = class_counts(Path(args.data_dir))
        selected_classes = CLASSES if args.class_name == "all" else [args.class_name]
        for name in selected_classes:
            #计算剩余需要的数量
            remaining = max(0, args.target_count - counts.get(name, 0))
            # 判断条件 `remaining <= 0` 是否成立。
            if remaining <= 0:
                print(f"{name} already has {counts.get(name, 0)} images, target reached.")
                continue
            # 提示用户准备好当前类别的手势，再按 Enter 开始采集。
            input(f"\nPrepare gesture '{name}', need {remaining} more images, then press Enter.")
            # 把本轮采集数量改成 remaining
            args.samples = remaining
            #开始采集一轮
            collect_one_class(args, name)
        #恢复原始 samples 参数
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


# 定义 split_image_paths 函数：把图片路径按训练集和验证集拆分。
def split_image_paths(data_dir: Path, validation_split: float, seed: int):
    # 分层划分数据集：每个类别都按相同比例拆成训练集和验证集，避免类别不均衡。
    rng = random.Random(seed)
    train_paths, train_labels, val_paths, val_labels = [], [], [], []
    for label, name in enumerate(CLASSES):
        files = image_files(data_dir / name)
        # 把当前类别下的图片顺序随机打乱
        rng.shuffle(files)
        val_count = max(1, int(round(len(files) * validation_split)))
        # 把打乱后的图片列表切开：
        #前 val_count 张作为验证集
        #剩下的作为训练集
        val_files = files[:val_count]
        train_files = files[val_count:]
        #把训练图片路径加入 train_paths，并为这些图片加入对应的数字标签。
        train_paths.extend(str(path) for path in train_files)
        train_labels.extend([label] * len(train_files))
        val_paths.extend(str(path) for path in val_files)
        val_labels.extend([label] * len(val_files))
   
    return train_paths, train_labels, val_paths, val_labels


# 定义 decode_image 函数：在 TensorFlow 数据管道中解码单张图片。
def decode_image(path, label, image_size: int, hand_preprocess: bool):
    # 读取图片并转换为 TensorFlow 训练需要的图像张量和 one-hot 标签。
    if hand_preprocess:
        #把普通 Python/NumPy 函数接入 TensorFlow 数据管道。
        image = tf.numpy_function(
            lambda p: load_image_for_training(p, image_size, True),
            [path],
            tf.float32,
        )
        #手动指定图片形状，方便后续 batch 和模型输入。
        image.set_shape([image_size, image_size, 3])
    else:
        #使用 TensorFlow 原生接口读取图片文件
        data = tf.io.read_file(path)
        #将图片二进制数据解码成三通道 RGB 图像。
        image = tf.io.decode_image(data, channels=3, expand_animations=False)
        #固定位3通道
        image.set_shape([None, None, 3])
        #统一缩放
        image = tf.image.resize(image, [image_size, image_size])
    #将数字标签转换为 one-hot 标签
    label = tf.one_hot(label, depth=len(CLASSES))
    return image, label


# 定义 make_dataset 函数：把路径和标签封装成可训练的数据集。
def make_dataset(paths, labels, image_size: int, batch_size: int, seed: int, training: bool, hand_preprocess: bool):
    # 构建 tf.data 数据管道；训练集打乱顺序，验证集保持固定。
    ds = tf.data.Dataset.from_tensor_slices((paths, labels))
    #如果是训练集，就打乱样本顺序。
    if training:
        ds = ds.shuffle(len(paths), seed=seed, reshuffle_each_iteration=True)
    #对数据集中的每一项执行 decode_image。
    ds = ds.map(lambda p, y: decode_image(p, y, image_size, hand_preprocess), num_parallel_calls=tf.data.AUTOTUNE)
    return ds.batch(batch_size).prefetch(tf.data.AUTOTUNE)


# 定义 make_datasets 函数：创建训练集、验证集和标签列表。
def make_datasets(data_dir: Path, image_size: int, batch_size: int, seed: int, validation_split: float, hand_preprocess: bool):
    # 同时创建训练集和验证集，并打印各类别样本数量，便于检查数据是否均衡。
    require_tf()
    # 按类别（训练集、验证集）划分图片路径。
    train_paths, train_labels, val_paths, val_labels = split_image_paths(data_dir, validation_split, seed)
    # 统计训练集中每个类别的样本数量。
    train_counts = {name: train_labels.count(i) for i, name in enumerate(CLASSES)}
    # 统计验证集中每个类别的样本数量。
    val_counts = {name: val_labels.count(i) for i, name in enumerate(CLASSES)}
    print(f"Training split: {train_counts}")
    print(f"Validation split: {val_counts}")
    # 根据训练图片路径和标签创建训练数据集。
    train_ds = make_dataset(train_paths, train_labels, image_size, batch_size, seed, training=True, hand_preprocess=hand_preprocess)
    # 根据验证图片路径和标签创建验证数据集。
    val_ds = make_dataset(val_paths, val_labels, image_size, batch_size, seed, training=False, hand_preprocess=hand_preprocess)
    return train_ds, val_ds, train_counts, val_counts


# 定义 build_model 函数：搭建并编译从零训练的 CNN 模型。
def build_model(image_size: int, learning_rate: float):
    # 从零搭建 CNN 卷积神经网络，不使用任何预训练模型。
    # 执行“搭建并编译从零训练的 CNN 模型”中的这一行操作。
    require_tf()
    # 调用 Keras 接口搭建、训练或加载模型。
    model = tf.keras.Sequential(
        # 开始一个多行结构。
        [
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Input(shape=(image_size, image_size, 3)),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Rescaling(1.0 / 255),  # 将像素值从 0-255 归一化到 0-1。
            # 数据增强：模拟手势的轻微翻转、旋转、移动、缩放和光照变化，提升泛化能力。
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.RandomFlip("horizontal"),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.RandomRotation(0.03),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.RandomTranslation(0.04, 0.04),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.RandomZoom(0.06),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.RandomBrightness(0.16, value_range=(0, 1)),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.RandomContrast(0.18),
            # 三组卷积层和池化层用于提取手指边缘、掌心轮廓等局部视觉特征。
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Conv2D(32, 3, padding="same", activation="relu"),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.MaxPooling2D(),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Conv2D(64, 3, padding="same", activation="relu"),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.MaxPooling2D(),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Conv2D(128, 3, padding="same", activation="relu"),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.MaxPooling2D(),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Flatten(),  # 将二维特征图展平成一维向量。
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Dropout(0.35),  # 防止模型只记住训练集，降低过拟合风险。
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Dense(128, activation="relu"),
            # 调用 Keras 接口搭建、训练或加载模型。
            tf.keras.layers.Dense(len(CLASSES), activation="softmax"),  # 输出三类手势的概率。
        # 结束上面的多行结构。
        ]
    # 结束上面的多行结构。
    )
    # 执行“搭建并编译从零训练的 CNN 模型”中的这一行操作。
    model.compile(
        # 调用 Keras 接口搭建、训练或加载模型。
        optimizer=tf.keras.optimizers.Adam(learning_rate),
        # 计算并保存 `loss`，供后续逻辑使用。
        loss="categorical_crossentropy",
        # 计算并保存 `metrics`，供后续逻辑使用。
        metrics=["accuracy"],
    # 结束上面的多行结构。
    )
    # 返回 `model`，把结果交给调用者。
    return model


# 定义 plot_history 函数：保存训练过程中的准确率和损失曲线。
def plot_history(history, out_path: Path):
    # 使用 matplotlib 绘制或保存训练图表。
    #创建一张大小为 8 x 4 的图
    plt.figure(figsize=(8, 4))
    # 把整张图分成 1 行 2 列，当前操作第 1 个子图。
    plt.subplot(1, 2, 1)
    # 训练集准确率曲线
    plt.plot(history.history["accuracy"], label="train")
    # 验证集准确率曲线
    plt.plot(history.history["val_accuracy"], label="val")
    # 标题
    plt.title("Accuracy")
    # 横轴
    plt.xlabel("Epoch")
    # 图例
    plt.legend()
    # 2图
    plt.subplot(1, 2, 2)
    # 训练集损失曲线
    plt.plot(history.history["loss"], label="train")
    #验证集损失曲线
    plt.plot(history.history["val_loss"], label="val")
    plt.title("Loss")
    plt.xlabel("Epoch")
    plt.legend()
    # 自动调整子图间距，避免标题、坐标轴、图例重叠
    plt.tight_layout()
    # 把图保存到 out_path，dpi=160 表示图片清晰度
    plt.savefig(out_path, dpi=160)
    # 关闭图像
    plt.close()


# 定义 save_keras_model 函数：保存 Keras 模型，同时兼容不同版本接口。
def save_keras_model(model, target_path: Path):
    # 先保存到临时目录再复制到目标路径，减少 Windows 中文路径导致的保存问题。
    #确保 target_path 是 Path 类型
    target_path = Path(target_path)
    # 创建目录；如果已经存在就直接复用。
    target_path.parent.mkdir(parents=True, exist_ok=True)
    #创建一个临时目录。with 结束后，这个临时目录会自动删除，解决中文不友好
    with tempfile.TemporaryDirectory(prefix="rps_keras_save_") as tmp_dir:
        tmp_path = Path(tmp_dir) / target_path.name
        # 把模型保存到临时路径。
        model.save(str(tmp_path))
        #把临时保存好的模型文件复制到最终目标路径。
        shutil.copy2(tmp_path, target_path)


# 定义 compute_confusion_matrix 函数：计算验证集上的混淆矩阵。
def compute_confusion_matrix(model, dataset):
    # 混淆矩阵用于观察每个真实类别被预测成了什么类别。
    #创建一个全 0 矩阵
    matrix = np.zeros((len(CLASSES), len(CLASSES)), dtype=int)

    for images, labels in dataset:
        # 让模型输出当前图片属于各类别的概率。
        predictions = model.predict(images, verbose=0)
        #得到真实类别编号
        true_ids = np.argmax(labels.numpy(), axis=1)
        # 模型预测概率中取最大概率对应的类别编号
        pred_ids = np.argmax(predictions, axis=1)
        #计算矩阵
        for true_id, pred_id in zip(true_ids, pred_ids):
            matrix[int(true_id), int(pred_id)] += 1
    return matrix


# 定义 train_model 函数：训练模型并保存模型、标签和报告。
def train_model(args):
    # 完整训练流程：加载数据、构建模型、训练、评估并保存模型和报告。
    # 执行“训练模型并保存模型、标签和报告”中的这一行操作。
    require_tf()
    #自带的随机
    random.seed(args.seed)
    # numpy随机
    np.random.seed(args.seed)
    # tensorflow随机
    tf.random.set_seed(args.seed)
    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)
    # 创建目录；如果已经存在就直接复用。
    output_dir.mkdir(parents=True, exist_ok=True)
    counts = class_counts(data_dir)
    if min(counts.values(), default=0) < 10:
        raise SystemExit(
            f"Not enough images in {data_dir}. Collect at least 30 per class first. Counts: {counts}"
        )
    # 创建训练集和验证集
    train_ds, val_ds, train_counts, val_counts = make_datasets(
        data_dir, args.image_size, 
        args.batch_size, args.seed, 
        args.validation_split, 
        args.hand_preprocess
    )
    #构建 CNN 模型
    model = build_model(args.image_size, args.learning_rate)
  
    callbacks = [
        # 验证准确率长时间不提升时提前停止，避免过拟合。
        tf.keras.callbacks.EarlyStopping(monitor="val_accuracy", mode="max", patience=8, restore_best_weights=True),
        # 验证损失不下降时降低学习率，让后期训练更稳定。
        tf.keras.callbacks.ReduceLROnPlateau(monitor="val_loss", factor=0.5, patience=4),

    ]
    # 开始训练神经网络模型。
    history = model.fit(train_ds, validation_data=val_ds, epochs=args.epochs, callbacks=callbacks)
    # 在验证集上评估模型效果。
    loss, acc = model.evaluate(val_ds, verbose=0)
    #计算混淆矩阵
    confusion = compute_confusion_matrix(model, val_ds)
    # 保存模型
    save_keras_model(model, output_dir / "model.keras")
    # 复制一份最佳模型
    shutil.copy2(output_dir / "model.keras", output_dir / "best_model.keras")
    # 保存标签
    (output_dir / "labels.txt").write_text("\n".join(CLASSES), encoding="utf-8")
    # 画训练曲线
    plot_history(history, output_dir / "training_curve.png")


    report = {
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
    # 把结果字典转换成格式化 JSON 文本。
    (output_dir / "run_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Saved model to {output_dir / 'model.keras'}")
    print(f"Validation accuracy: {acc:.3f}")


# 定义 load_labels 函数：读取训练时保存的类别标签。
def load_labels(output_dir: Path):
    # 读取训练时保存的类别标签；如果文件不存在，则使用默认类别顺序。
    label_file = output_dir / "labels.txt"
    if label_file.exists():
        #从 labels.txt 里读取类别名，去掉空行和首尾空格，最后返回一个类别列表
        return [line.strip() for line in label_file.read_text(encoding="utf-8").splitlines() if line.strip()]
    return CLASSES


# 定义 winning_response 函数：根据猜拳规则生成电脑获胜手势。
def winning_response(label: str):
    # 根据识别结果返回电脑应该出的获胜手势。
    if label not in WINNING_MOVE:
        return "waiting", "请把清晰手势放在绿色方框内。"
    response = WINNING_MOVE[label]
    return response, f"{DISPLAY_NAMES[response]} 可以赢 {DISPLAY_NAMES[label]}。"


# 定义 load_predictor 函数：加载预测阶段需要的模型和标签。
def load_predictor(args):
    # 加载已经训练好的 Keras 模型和标签文件。
    require_tf()
    output_dir = Path(args.output_dir)
    model_path = Path(args.model) if args.model else output_dir / "model.keras"
    if not model_path.exists():
        raise SystemExit(f"Model not found: {model_path}. Train first.")
    labels = load_labels(output_dir)
    # 调用 Keras 接口搭建、训练或加载模型。
    model = tf.keras.models.load_model(model_path)
    return model, labels


# 定义 skin_fraction 函数：估算画面中肤色像素比例。
def skin_fraction(roi, cv2):
    # 估计 ROI 中肤色像素比例，用于辅助处理剪刀和布容易混淆的情况。
    # 转换图像颜色空间。
    ycrcb = cv2.cvtColor(roi, cv2.COLOR_BGR2YCrCb)
    # 计算并保存 `lower`，供后续逻辑使用。
    lower = np.array([0, 133, 77], dtype=np.uint8)
    # 计算并保存 `upper`，供后续逻辑使用。
    upper = np.array([255, 173, 127], dtype=np.uint8)
    # 按阈值生成二值掩码。
    mask = cv2.inRange(ycrcb, lower, upper)
    # 对掩码做中值滤波，进一步平滑边缘。
    mask = cv2.medianBlur(mask, 5)
    # 返回 `cv2.countNonZero(mask) / float(mask.shape[0] * mask.shape[1])`，把结果交给调用者。
    return cv2.countNonZero(mask) / float(mask.shape[0] * mask.shape[1])


# 定义 predict_roi_probs 函数：对单个 ROI 输出模型概率。弃用
def predict_roi_probs(model, roi, cv2, args):
    # 对单张 ROI 图像进行预处理，并输出 rock/paper/scissors 三类概率。

    img = prepare_roi_for_model(roi, cv2, args)
    # 转换图像颜色空间。
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    return model.predict(np.expand_dims(rgb.astype("float32"), axis=0), verbose=0)[0]


# 定义 predict_rois_probs 函数：批量预测多个 ROI 的概率。
def predict_rois_probs(model, rois, cv2, args):
    # 对多帧 ROI 批量预测，UI 最终锁定结果时会对这些概率取平均。
    batch = []
    # 逐个处理 ROI。
    for roi in rois:
        #完成裁剪、缩放、补边、
        img = prepare_roi_for_model(roi, cv2, args)
        #转换图像颜色空间。
        rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        #将图像转换为 float32 类型后加入 batch。
        batch.append(rgb.astype("float32"))

    if not batch:
        #返回一个形状为 (1, 类别数) 的全零概率数组。
        return np.zeros((1, len(CLASSES)), dtype=np.float32)
    # model.predict 会一次性对所有 ROI 做前向推理，返回每个 ROI 对应各个类别的预测概率。
    return model.predict(np.stack(batch, axis=0), verbose=0)


# 定义 choose_label 函数：根据概率、阈值和剪刀修正规则选择最终标签。
def choose_label(labels, probs, roi, cv2, args):
    # 从概率中选择最终类别；置信度太低时返回 uncertain。
    #找概率最大的类别
    idx = int(np.argmax(probs))
    confidence = float(probs[idx])
    label = labels[idx]
    #记录有没有做过修正
    corrected = False

    # 判断是否需要做“剪刀修正”
    if getattr(args, "scissors_correction", True) and label == "paper" and "scissors" in labels and roi is not None:
        # 剪刀和布都属于张开手指的形态，容易混淆；这里做一个保守的剪刀修正。
        # 取出剪刀的概率
        scissors_index = labels.index("scissors")
        scissors_prob = float(probs[scissors_index])
        # 读取两个修正阈值
        skin_threshold = getattr(args, "scissors_skin_threshold", 0.04)
        min_prob = getattr(args, "scissors_min_prob", 0.24)
        # 执行剪刀修正
        if skin_fraction(roi, cv2) >= skin_threshold and scissors_prob >= min_prob:
            label = "scissors"
            confidence = scissors_prob
            corrected = True

    # 如果置信度太低，就标记为未识别
    if confidence < getattr(args, "min_confidence", 0.55) and not corrected:
        label = "uncertain"
    return label, confidence, probs


# 定义 classify_roi 函数：把单帧 ROI 转成平滑后的分类结果。弃用
def classify_roi(model, labels, roi, cv2, args, recent_probs):
    # 实时预测时使用最近多帧平均概率，让结果更平滑。
    # 计算并保存 `probs`，供后续逻辑使用。
    probs = predict_roi_probs(model, roi, cv2, args)
    # 执行“把单帧 ROI 转成平滑后的分类结果”中的这一行操作。
    recent_probs.append(probs)
    # 计算并保存 `smooth_probs`，供后续逻辑使用。
    smooth_probs = np.mean(np.array(recent_probs), axis=0)
    # 同时计算并保存 `label, confidence, smooth_probs` 这些值。
    label, confidence, smooth_probs = choose_label(labels, smooth_probs, roi, cv2, args)
    # 返回 `label, confidence, smooth_probs`，把结果交给调用者。
    return label, confidence, smooth_probs


# 定义 predict_camera 函数：使用 OpenCV 窗口进行实时预测。弃用
def predict_camera(args):
    require_tf()
    cv2 = import_cv2()
    model, labels = load_predictor(args)
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        raise SystemExit(f"Could not open camera {args.camera}.")
    print("Running camera prediction. Press q to quit.")
    #创建一个固定长度的队列，用来保存最近几帧的预测概率
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


# 定义 draw_probability_bars 函数：在 Tk 画布上绘制三类概率条。
def draw_probability_bars(canvas, labels, probs):
    # 在 Tkinter 侧边栏绘制三类概率条。
    #清空画布上原来的内容
    canvas.delete("all")
    #获取画布宽度。
    width = int(canvas["width"])
    # 定义三种手势的颜色。
    colors = {"rock": "#7dd3fc", "paper": "#86efac", "scissors": "#fca5a5"}
    # 遍历类别和概率
    for index, (name, prob) in enumerate(zip(labels, probs)):
        # 计算这一行概率条的顶部 y 坐标。
        top = 12 + index * 48
        # 根据概率计算彩色条长度。
        bar_width = int((width - 130) * float(prob))
        # 画左侧类别文字。
        canvas.create_text(10, top + 13, anchor="w", text=DISPLAY_NAMES.get(name, name), fill="#dbeafe", font=("Microsoft YaHei UI", 11, "bold"))
        #画概率条的背景框。
        canvas.create_rectangle(96, top, width - 20, top + 26, fill="#1f2937", outline="#334155")
        # 画真正的彩色概率条。
        canvas.create_rectangle(96, top, 96 + bar_width, top + 26, fill=colors.get(name, "#93c5fd"), outline="")
        #在右侧画百分比文字。
        canvas.create_text(width - 16, top + 13, anchor="e", text=f"{prob:.0%}", fill="#e5e7eb", font=("Microsoft YaHei UI", 10))

# 定义 run_ui 函数：启动带倒计时和比赛结果的图形界面。
def run_ui(args):
    # 图形化比赛界面：倒计时、采样、锁定识别结果，并显示电脑获胜手势。
    require_tf()
    cv2 = import_cv2()
    try:
        # 导入 tkinter，用来构建桌面图形界面。
        import tkinter as tk
        # 导入 Pillow 的图像桥接工具，把 OpenCV 画面显示到 Tk。
        #Tkinter 负责窗口界面，Pillow 负责把 OpenCV 图像转成 Tkinter 能显示的图片。
        from PIL import Image, ImageTk
    except ImportError as exc:
        raise SystemExit("Tkinter and Pillow are required for UI mode.") from exc
    #加载模型
    model, labels = load_predictor(args)
    output_dir = Path(args.output_dir)
    # 创建目录；如果已经存在就直接复用。
    output_dir.mkdir(parents=True, exist_ok=True)
    # 打开指定编号的摄像头。
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        raise SystemExit(f"Could not open camera {args.camera}.")
    # 创建界面
    root = tk.Tk()
    #设置窗口标题。
    root.title("石头剪刀布手势比赛")
    # 设置窗口背景色。
    root.configure(bg="#0f172a")
    # 限制窗口最小大小，防止控件挤在一起。
    root.minsize(980, 620)
    # 创建左侧摄像头画面区域。它本质是一个 Label，但里面会不断更新图片。
    video_panel = tk.Label(root, bg="#020617")
    # 把摄像头区域放到窗口左边，并让它尽量占据剩余空间
    video_panel.pack(side="left", fill="both", expand=True, padx=(18, 10), pady=18)
    # 创建右侧控制面板，宽度 330。
    side = tk.Frame(root, bg="#0f172a", width=330)
    # 把右侧面板放到窗口右边，竖向填满。
    side.pack(side="right", fill="y", padx=(8, 18), pady=18)
    #禁止右侧面板被内部控件撑大或缩小，保持固定宽度。
    side.pack_propagate(False)

    # 创建右侧 UI 控件
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

    # 倒计时文字变量
    countdown_var = tk.StringVar(value="准备")
    tk.Label(side, textvariable=countdown_var, fg="#fef3c7", bg="#0f172a", font=("Microsoft YaHei UI", 40, "bold")).pack(anchor="center", pady=(0, 12))

    # 创建“开始比赛”按钮。点击按钮后调用内部函数 start_round()
    start_button = tk.Button(
        side,
        text="开始比赛",
        command=lambda: start_round(),
        bg="#2563eb",
        fg="#ffffff",
        activebackground="#414654",
        activeforeground="#ffffff",
        bd=0,
        padx=18,
        pady=10,
        font=("Microsoft YaHei UI", 13, "bold"),
    )
    start_button.pack(fill="x", pady=(0, 22))
    #保存“你的手势”的图标和文字
    tk.Label(side, text="你的手势", fg="#93c5fd", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w")
    gesture_emoji_var = tk.StringVar(value=GESTURE_EMOJI["waiting"])
    tk.Label(side, textvariable=gesture_emoji_var, fg="#f8fafc", bg="#0f172a", font=("Segoe UI Emoji", 78)).pack(anchor="center")
    gesture_var = tk.StringVar(value="等待")
    tk.Label(side, textvariable=gesture_var, fg="#f8fafc", bg="#0f172a", font=("Microsoft YaHei UI", 18, "bold")).pack(anchor="center", pady=(0, 12))

    # 保存“电脑出招”的图标和文字。
    tk.Label(side, text="电脑出招", fg="#fbbf24", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w")
    response_emoji_var = tk.StringVar(value=GESTURE_EMOJI["waiting"])
    tk.Label(side, textvariable=response_emoji_var, fg="#fde68a", bg="#0f172a", font=("Segoe UI Emoji", 78)).pack(anchor="center")
    response_var = tk.StringVar(value="等待")
    tk.Label(side, textvariable=response_var, fg="#fde68a", bg="#0f172a", font=("Microsoft YaHei UI", 18, "bold")).pack(anchor="center", pady=(0, 10))

    # 保存解释文字
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
    #保存置信度文字
    confidence_var = tk.StringVar(value="置信度：--")
    #创建概率条画布
    tk.Label(side, textvariable=confidence_var, fg="#a7f3d0", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w", pady=(0, 8))
    bars = tk.Canvas(side, width=300, height=160, bg="#0f172a", highlightthickness=0)
    bars.pack(anchor="w", pady=(0, 22))
    #右侧面板底部的提示文字。
    tk.Label(
        side,
        text="提示：出拳后保持约 1 秒，不要马上收手；尽量保持和采集数据时相同的距离、角度和光照。",
        fg="#64748b",
        bg="#0f172a",
        wraplength=300,
        justify="left",
        font=("Microsoft YaHei UI", 10),
    ).pack(anchor="w", side="bottom")

    #实时预测平滑缓存
    recent_probs = deque(maxlen=max(1, args.smooth))
    #保存本局倒计时结束后采集到的多帧 ROI
    round_rois = deque(maxlen=max(1, args.final_window))
    # 保存窗口是否还在运行。
    running = {"value": True}
    #核心状态机
    state = {"phase": "ready", "deadline": 0.0, "capture_deadline": 0.0, "last_roi": None}

    # 定义 show_frame 函数：把 OpenCV 图像刷新到 Tk 窗口中。
    def show_frame(frame):
        # OpenCV 图像是 BGR，Tkinter 显示前需要转为 RGB。
        # 转换图像颜色空间。
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        #把 NumPy 图像数组转成 Pillow 图片
        image = Image.fromarray(rgb_frame)
        # 把图片缩放到适合 UI 显示的大小
        image.thumbnail((720, 560), Image.Resampling.LANCZOS)
        #把 Pillow 图片转成 Tkinter 可显示的图片对象
        photo = ImageTk.PhotoImage(image=image)
        #更新左侧摄像头画面。
        video_panel.configure(image=photo)
        #保存引用，防止图片被 Python 垃圾回收
        video_panel.image = photo

    # 定义 set_waiting_visuals 函数：把界面重置为等待开局状态。
    def set_waiting_visuals():
        # 空闲状态：不显示预测结果，等待用户点击开始。
        #把界面恢复成等待状态
        countdown_var.set("准备")
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        gesture_var.set("等待")
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        response_var.set("等待")
        reason_var.set("点击开始比赛。")
        confidence_var.set("置信度：--")
        draw_probability_bars(bars, labels, np.zeros(len(labels)))

    # 定义 set_result_visuals 函数：把最终识别结果显示到界面上
    def set_result_visuals(label, confidence, probs, random_result=False):
        # 锁定结果后，更新用户手势、电脑出拳、置信度和概率条。
        # 根据你的手势计算电脑应该出什么
        response, reason = winning_response(label)
        #设置“你的手势”的图
        gesture_emoji_var.set(GESTURE_EMOJI.get(label, GESTURE_EMOJI["uncertain"]))
        if random_result:
            gesture_var.set(f"随机：{DISPLAY_NAMES.get(label, label)}")
        else:
            gesture_var.set(f"{DISPLAY_NAMES.get(label, label)}  {confidence:.0%}")
        #设置“电脑出招”的图标。
        response_emoji_var.set(GESTURE_EMOJI.get(response, GESTURE_EMOJI["waiting"]))
        #设置“电脑出招”的文字。
        response_var.set(DISPLAY_NAMES.get(response, response))
        #解释文字
        reason_var.set("未能稳定识别，已随机选择一个结果。" if random_result else reason)
        confidence_var.set("置信度：随机结果" if random_result else f"置信度：{confidence:.1%}")
        draw_probability_bars(bars, labels, probs)

    # 定义 start_round 函数：开始一局新的倒计时比赛。
    def start_round():
        # 开始新一局：清空上一局缓存，进入倒计时阶段。
        # 清空实时预测的平滑缓存
        recent_probs.clear()
        # 清空本局采样图片缓存
        round_rois.clear()
        # 把当前阶段改成倒计时
        state["phase"] = "countdown"
        #计算倒计时结束的时间点。
        #perf_counter() 是当前时间，args.countdown 是倒计时秒数，默认大概是 3 秒
        state["deadline"] = perf_counter() + max(0.5, float(args.countdown))
        #剩余时间小于等于 0，就进入采样阶段
        state["capture_deadline"] = 0.0
        #清空上一局最后一张 ROI 图片
        state["last_roi"] = None
        #更新界面上的倒计时数字
        countdown_var.set(str(int(np.ceil(args.countdown))))
        # 把“你的手势”图标设成等待图标
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        # “你的手势”文字
        gesture_var.set("结果锁定后显示")
        # “电脑出招”图标设成等待图标
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        # 电脑出招文字
        response_var.set("结果锁定后显示")
        #更新提示文字
        reason_var.set("请把手势保持在绿色方框内，倒计时结束后才显示结果。")
        #隐藏置信度。
        confidence_var.set("置信度：隐藏")
        # 把概率条清零
        draw_probability_bars(bars, labels, np.zeros(len(labels)))
        #修改开始按钮状态
        start_button.configure(text="倒计时中", state="disabled", bg="#475569")

    # 定义 lock_round 函数：倒计时结束后锁定并保存本局结果。
    def lock_round(frame):
        # 倒计时结束后锁定结果：对采样到的多帧预测概率取平均。
        #判断有没有采集到 ROI
        if round_rois:
            # 对多帧 ROI 做预测
            predictions = predict_rois_probs(model, list(round_rois), cv2, args)
            #多帧概率取平均
            final_probs = np.mean(predictions, axis=0)
        else:
            #构造一个全 0 概率数组
            final_probs = np.zeros(len(labels), dtype=np.float32)
        # 选择最有可能的选项，对剪刀进行优化识别
        label, confidence, final_probs = choose_label(labels, final_probs, state["last_roi"], cv2, args)
        #判断是否识别失败
        random_result = label == "uncertain"
        if random_result:
            # 如果没有稳定识别出来，就随机给出一个手势，保证比赛流程有结果。
            #随机选一个
            label = random.choice(list(labels))
            #新创建一个全 0 概率数组，把随机选中的那个类别概率设为 1
            confidence = 0.0
            final_probs = np.zeros(len(labels), dtype=np.float32)
            final_probs[labels.index(label)] = 1.0
        # 获取获胜的手势
        response, reason = winning_response(label)
        # 显示最终结果UI界面
        set_result_visuals(label, confidence, final_probs, random_result=random_result)
        #组织本局结果数据
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
            # 把文本内容写入文件。
            (output_dir / "latest_ui_result.json").write_text(
                # 把结果字典转换成格式化 JSON 文本。
                json.dumps(result, ensure_ascii=False, indent=2),
                encoding="utf-8",
            )
            #.如果有最后一帧 ROI，就保存 ROI 调试图
            if state["last_roi"] is not None:
                # 保存调试图：原始 ROI 和模型真正看到的预处理后输入。
                save_jpg(output_dir / "latest_ui_roi.jpg", state["last_roi"], cv2)
                #保存模型真正看到的图像。
                save_jpg(output_dir / "latest_ui_model_input.jpg", prepare_roi_for_model(state["last_roi"], cv2, args), cv2)
            #保存整张锁定画面
            save_jpg(output_dir / "latest_ui_frame.jpg", frame, cv2)
        except OSError as exc:
            # 在控制台输出当前进度或状态信息。
            print(f"保存本局调试图片失败：{exc}")
        # 更新倒计时文字
        countdown_var.set("已锁定")
        if not random_result:
            reason_var.set(reason)
        # 恢复按钮
        start_button.configure(text="下一局", state="normal", bg="#2563eb")
        state["phase"] = "locked"

        #显示锁定那一帧画面
        show_frame(frame)

    # 定义 start_capture 函数：进入短暂采样阶段以获得稳定画面。
    def start_capture():
        # 倒计时结束后短暂采样，避免用户最后一秒出手造成单帧模糊。
        # 清空本轮已经采集到的 ROI 图像。
        round_rois.clear()
        state["phase"] = "capture"
        #设置采集截止时间
        state["capture_deadline"] = perf_counter() + max(0.05, float(args.capture_duration))
        # 更新界面文字
        countdown_var.set("出拳！")
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        gesture_var.set("正在锁定")
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        response_var.set("正在锁定")
        reason_var.set("请保持刚出的手势不动，正在采集稳定画面。")
        confidence_var.set("置信度：隐藏")
        #把概率条清零。
        draw_probability_bars(bars, labels, np.zeros(len(labels)))

    # 定义 on_close 函数：关闭窗口并释放摄像头资源。
    def on_close():
        running["value"] = False
        # 释放摄像头资源。
        cap.release()
        root.destroy()

    # 定义 update_frame 函数：持续读取摄像头并驱动 UI 状态更新。
    def update_frame():
        # UI 主循环：持续读取摄像头画面，并根据阶段更新界面状态。
        # 如果程序已经停止运行，就直接退出，不再刷新画面
        if not running["value"]:
            return
        # 不继续读取和更新摄像头预测，只是每隔 80ms 再检查一次
        if state["phase"] == "locked":
            root.after(80, update_frame)
            return
        # 从摄像头读取一帧画面。
        ok, frame = cap.read()
        if ok:
            # 水平翻转画面，让摄像头预览像照镜子一样。
            frame = cv2.flip(frame, 1)
            #后计算中央正方形 ROI
            x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
            roi = frame[y1:y2, x1:x2]
            # 在画面上画出绿色手势采集框。
            cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 3)

            if state["phase"] == "countdown":
                #计算距离倒计时结束还剩多少秒
                remaining = state["deadline"] - perf_counter()
                #进入采集阶段
                if remaining <= 0:
                    #锁定画面，开始识别
                    start_capture()
                    # 把提示文字绘制到摄像头画面上。
                    cv2.putText(frame, "!", (42, 112), cv2.FONT_HERSHEY_SIMPLEX, 3.0, (80, 220, 255), 7)
                    #显示当前帧
                    show_frame(frame)
                    # 80ms 后继续更新
                    root.after(80, update_frame)
                    return
                #计算并显示剩余秒数和其余UI
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
                # 把当前 ROI 保存下来
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
                #判断采样时间是否结束
                if perf_counter() >= state["capture_deadline"]:
                    #锁定结果
                    lock_round(frame)
                    root.after(80, update_frame)
                    return
            else:
                #空闲状态
                set_waiting_visuals()

            show_frame(frame)
        root.after(20, update_frame)

    # 点关闭按钮时，执行 on_close
    root.protocol("WM_DELETE_WINDOW", on_close)
    # 按空格开始一轮游戏/识别。
    root.bind("<space>", lambda _event: start_round())
    #初始化界面为等待状态。
    set_waiting_visuals()
    #启动摄像头刷新循环。
    update_frame()
    #进入 Tkinter 主事件循环，窗口开始运行。
    root.mainloop() 


# 定义 show_status 函数：打印数据集和模型的当前状态。
def show_status(args):
    # 输出当前项目状态：数据集数量、模型路径和是否存在模型。
    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)
    #打印项目说明
    print("Project: self-trained rock-paper-scissors gesture classifier")
    # 打印数据集路径
    print(f"Data dir: {data_dir}")
    # 打印每类图片数量
    print(f"Counts: {class_counts(data_dir)}")
    # 打印模型输出目录
    print(f"Output dir: {output_dir}")
    # 打印模型文件是否存在
    print(f"Model exists: {(output_dir / 'model.keras').exists()}")
    print("No pretrained model is used. The CNN starts from random weights.")


# 定义 parse_args 函数：定义并解析命令行参数。
def parse_args():
    # 命令行参数。默认 mode 为 ui，所以 VS Code 直接运行脚本会打开图形界面。
    # 创建命令行参数解析器，并准备写入程序说明。
    parser = argparse.ArgumentParser(
        # 计算并保存 `description`，供后续逻辑使用。
        description="Collect your own hand images, train a TensorFlow CNN from scratch, and run live prediction."
    # 结束上面的多行结构。
    )
    # 添加运行模式参数，决定采集、训练、预测、UI 或状态查看。
    parser.add_argument("mode", nargs="?", default="ui", choices=["collect", "train", "predict", "ui", "status"])
    # 添加数据目录参数，用来指定手势图片保存位置。
    parser.add_argument("--data-dir", default=DEFAULT_DATA_DIR)
    # 添加输出目录参数，用来指定模型和报告保存位置。
    parser.add_argument("--output-dir", default=DEFAULT_OUTPUT_DIR)
    # 添加图片尺寸参数，控制模型输入宽高。
    parser.add_argument("--image-size", type=int, default=128)
    # 添加摄像头编号参数，选择要打开的摄像头。
    parser.add_argument("--camera", type=int, default=0)
    # 添加 ROI 比例参数，控制绿色取景框大小。
    parser.add_argument("--roi-ratio", type=float, default=0.62)
    # 添加类别参数，选择采集全部类别或某一类。
    parser.add_argument("--class-name", choices=CLASSES + ["all"], default="all")
    # 添加采样数量参数，控制本次要采集多少张。
    parser.add_argument("--samples", type=int, default=160)
    # 添加目标数量参数，方便补采到指定总数。
    parser.add_argument("--target-count", type=int, default=0)
    # 添加替换旧数据开关，启用时先备份旧图片。
    parser.add_argument("--replace-existing", action=argparse.BooleanOptionalAction, default=False)
    # 添加保存间隔参数，控制隔几帧保存一张样本。
    parser.add_argument("--every", type=int, default=2)
    # 添加训练轮数参数。
    parser.add_argument("--epochs", type=int, default=45)
    # 添加批大小参数。
    parser.add_argument("--batch-size", type=int, default=16)
    # 添加学习率参数。
    parser.add_argument("--learning-rate", type=float, default=7e-4)
    # 添加验证集比例参数。
    parser.add_argument("--validation-split", type=float, default=0.2)
    # 添加平滑窗口参数，减少实时预测抖动。
    parser.add_argument("--smooth", type=int, default=8)
    # 添加最低置信度参数，低于它就判为不确定。
    parser.add_argument("--min-confidence", type=float, default=0.55)
    # 添加倒计时时长参数。
    parser.add_argument("--countdown", type=float, default=3.0)
    # 添加锁定采样时长参数。
    parser.add_argument("--capture-duration", type=float, default=0.8)
    # 添加最终采样窗口参数，控制 UI 锁定时最多取多少帧。
    parser.add_argument("--final-window", type=int, default=24)
    # 添加手部预处理开关，可打开或关闭肤色分割。
    parser.add_argument("--hand-preprocess", action=argparse.BooleanOptionalAction, default=True)
    # 添加剪刀修正开关，用于改善剪刀被误判的问题。
    parser.add_argument("--scissors-correction", action=argparse.BooleanOptionalAction, default=True)
    # 添加剪刀修正所需的肤色比例阈值。
    parser.add_argument("--scissors-skin-threshold", type=float, default=0.04)
    # 添加剪刀修正所需的最低概率。
    parser.add_argument("--scissors-min-prob", type=float, default=0.24)
    # 添加随机种子参数，保证拆分和训练更可复现。
    parser.add_argument("--seed", type=int, default=11)
    # 添加模型路径参数，允许预测时加载指定模型。
    parser.add_argument("--model", default="")
    # 解析命令行参数并返回参数对象。
    return parser.parse_args()

# 定义 main 函数：根据命令行模式调用对应功能。
def main():
    args = parse_args()
    if args.mode == "collect":
        collect_data(args)
    elif args.mode == "train":
        train_model(args)
    elif args.mode == "predict":
        #弃用
        predict_camera(args)
    elif args.mode == "ui":
        run_ui(args)
    else:
        show_status(args)


if __name__ == "__main__":
    main()
