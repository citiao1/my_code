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


# 默认目录配置：让数据、模型和输出报告都保存在项目文件夹中，方便演示和提交。
# 计算并保存 `PROJECT_DIR`，供后续逻辑使用。
PROJECT_DIR = Path(__file__).resolve().parent
# 计算并保存 `DEFAULT_DATA_DIR`，供后续逻辑使用。
DEFAULT_DATA_DIR = PROJECT_DIR / "gesture_data"
# 计算并保存 `DEFAULT_OUTPUT_DIR`，供后续逻辑使用。
DEFAULT_OUTPUT_DIR = PROJECT_DIR / "outputs" / "rps_gesture"

# 三个手势类别，模型最后一层 softmax 的输出顺序也和这里一致。
# 计算并保存 `CLASSES`，供后续逻辑使用。
CLASSES = ["rock", "paper", "scissors"]
# 计算并保存 `DISPLAY_NAMES`，供后续逻辑使用。
DISPLAY_NAMES = {
    # 配置字典中 `rock` 对应的显示或规则值。
    "rock": "石头",
    # 配置字典中 `paper` 对应的显示或规则值。
    "paper": "布",
    # 配置字典中 `scissors` 对应的显示或规则值。
    "scissors": "剪刀",
    # 配置字典中 `uncertain` 对应的显示或规则值。
    "uncertain": "未识别",
    # 配置字典中 `waiting` 对应的显示或规则值。
    "waiting": "等待",
# 结束上面的多行结构。
}
# 计算并保存 `GESTURE_EMOJI`，供后续逻辑使用。
GESTURE_EMOJI = {
    # 配置字典中 `rock` 对应的显示或规则值。
    "rock": "✊",
    # 配置字典中 `paper` 对应的显示或规则值。
    "paper": "✋",
    # 配置字典中 `scissors` 对应的显示或规则值。
    "scissors": "✌",
    # 配置字典中 `uncertain` 对应的显示或规则值。
    "uncertain": "❔",
    # 配置字典中 `waiting` 对应的显示或规则值。
    "waiting": "⏳",
# 结束上面的多行结构。
}

# 猜拳胜负规则：识别出用户手势后，电脑选择能赢它的手势。
# 计算并保存 `WINNING_MOVE`，供后续逻辑使用。
WINNING_MOVE = {"rock": "paper", "paper": "scissors", "scissors": "rock"}


# 定义 require_tf 函数：按需加载 TensorFlow，避免程序启动时立刻变慢。
def require_tf():
    # TensorFlow 启动较慢，所以只在训练或预测真正需要时再导入。
    # 声明这里要修改全局变量 `tf`。
    global tf
    # 判断条件 `tf is None` 是否成立。
    if tf is None:
        # 导入 TensorFlow，并先放到临时变量中。
        import tensorflow as loaded_tf

        # 计算并保存 `tf`，供后续逻辑使用。
        tf = loaded_tf
    # 返回 `tf`，把结果交给调用者。
    return tf


# 定义 import_cv2 函数：安全导入 OpenCV，摄像头和图像处理都依赖它。
def import_cv2():
    # OpenCV 负责摄像头读取、画框、裁剪和基础图像处理。
    # 开始执行可能失败的代码，并准备捕获异常。
    try:
        # 导入 OpenCV，用来读取摄像头、裁剪和处理图像。
        import cv2

        # 返回 `cv2`，把结果交给调用者。
        return cv2
    # 捕获指定异常，给出更友好的处理方式。
    except ImportError as exc:
        # 主动抛出错误，让调用者知道程序无法继续。
        raise SystemExit(
            # 执行“安全导入 OpenCV，摄像头和图像处理都依赖它”中的这一行操作。
            "OpenCV is required for camera collection/prediction.\n"
            # 执行“安全导入 OpenCV，摄像头和图像处理都依赖它”中的这一行操作。
            "Install it with: python -m pip install opencv-python"
        # 执行“安全导入 OpenCV，摄像头和图像处理都依赖它”中的这一行操作。
        ) from exc


# 定义 ensure_dirs 函数：确保三类手势的数据文件夹都存在。
def ensure_dirs(data_dir: Path):
    # 创建 rock、paper、scissors 三个数据目录。
    # 遍历 `name`，逐项执行下面的逻辑。
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
    # 计算并保存 `folder`，供后续逻辑使用。
    folder = data_dir / class_name
    # 计算并保存 `files`，供后续逻辑使用。
    files = image_files(folder) if folder.exists() else []
    # 判断条件 `not files` 是否成立。
    if not files:
        # 在控制台输出当前进度或状态信息。
        print(f"{class_name} has no images to archive.")
        # 返回 `None`，把结果交给调用者。
        return None

    # 计算并保存 `timestamp`，供后续逻辑使用。
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    # 计算并保存 `backup_dir`，供后续逻辑使用。
    backup_dir = data_dir.parent / "gesture_data_backups" / f"{class_name}_{timestamp}"
    # 创建目录；如果已经存在就直接复用。
    backup_dir.mkdir(parents=True, exist_ok=True)
    # 遍历 `path`，逐项执行下面的逻辑。
    for path in files:
        # 把旧图片移动到备份目录。
        shutil.move(str(path), str(backup_dir / path.name))
    # 在控制台输出当前进度或状态信息。
    print(f"Archived {len(files)} old {class_name} images to {backup_dir}")
    # 返回 `backup_dir`，把结果交给调用者。
    return backup_dir


# 定义 image_files 函数：列出目录中的图片样本文件。
def image_files(folder: Path):
    # 只读取常见图片格式，避免把其他文件误当成训练样本。
    # 计算并保存 `suffixes`，供后续逻辑使用。
    suffixes = {".jpg", ".jpeg", ".png", ".bmp"}
    # 返回 `sorted([path for path in folder.iterdir() if path.suffix.lower() in suffixes])`，把结果交给调用者。
    return sorted([path for path in folder.iterdir() if path.suffix.lower() in suffixes])


# 定义 center_square 函数：计算画面中心的正方形采集区域。
def center_square(frame, size_ratio: float):
    # 计算画面中心的正方形 ROI，用户把手势放进这个绿色框。
    # 同时计算并保存 `h, w` 这些值。
    h, w = frame.shape[:2]
    # 计算并保存 `side`，供后续逻辑使用。
    side = int(min(h, w) * size_ratio)
    # 计算并保存 `x1`，供后续逻辑使用。
    x1 = (w - side) // 2
    # 计算并保存 `y1`，供后续逻辑使用。
    y1 = (h - side) // 2
    # 返回 `x1, y1, x1 + side, y1 + side`，把结果交给调用者。
    return x1, y1, x1 + side, y1 + side


# 定义 square_crop_with_margin 函数：按手部轮廓裁出带边缘的正方形图片。
def square_crop_with_margin(image, x, y, w, h, margin_ratio=0.28):
    # 根据手部轮廓重新裁剪成正方形，并留出少量边缘，避免切掉手指。
    # 同时计算并保存 `height, width` 这些值。
    height, width = image.shape[:2]
    # 计算并保存 `cx`，供后续逻辑使用。
    cx = x + w / 2
    # 计算并保存 `cy`，供后续逻辑使用。
    cy = y + h / 2
    # 计算并保存 `side`，供后续逻辑使用。
    side = int(max(w, h) * (1.0 + margin_ratio))
    # 计算并保存 `side`，供后续逻辑使用。
    side = max(side, 24)
    # 计算并保存 `x1`，供后续逻辑使用。
    x1 = int(round(cx - side / 2))
    # 计算并保存 `y1`，供后续逻辑使用。
    y1 = int(round(cy - side / 2))
    # 计算并保存 `x2`，供后续逻辑使用。
    x2 = x1 + side
    # 计算并保存 `y2`，供后续逻辑使用。
    y2 = y1 + side

    # 计算并保存 `pad_left`，供后续逻辑使用。
    pad_left = max(0, -x1)
    # 计算并保存 `pad_top`，供后续逻辑使用。
    pad_top = max(0, -y1)
    # 计算并保存 `pad_right`，供后续逻辑使用。
    pad_right = max(0, x2 - width)
    # 计算并保存 `pad_bottom`，供后续逻辑使用。
    pad_bottom = max(0, y2 - height)
    # 判断条件 `pad_left or pad_top or pad_right or pad_bottom` 是否成立。
    if pad_left or pad_top or pad_right or pad_bottom:
        # 计算并保存 `image`，供后续逻辑使用。
        image = cv2_copy_border(image, pad_top, pad_bottom, pad_left, pad_right)
        # 计算并保存 `x1 +`，供后续逻辑使用。
        x1 += pad_left
        # 计算并保存 `y1 +`，供后续逻辑使用。
        y1 += pad_top
        # 计算并保存 `x2 +`，供后续逻辑使用。
        x2 += pad_left
        # 计算并保存 `y2 +`，供后续逻辑使用。
        y2 += pad_top
    # 返回 `image[y1:y2, x1:x2]`，把结果交给调用者。
    return image[y1:y2, x1:x2]


# 定义 cv2_copy_border 函数：给超出边界的裁剪区域补黑边。
def cv2_copy_border(image, top, bottom, left, right):
    # 裁剪框超出图像边界时，用黑色像素补齐边缘。
    # 计算并保存 `cv2`，供后续逻辑使用。
    cv2 = import_cv2()
    # 返回 `cv2.copyMakeBorder(image, top, bottom, left, right, cv2.BORDER_CONSTANT, value=(0, 0, 0))`，把结果交给调用者。
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
    # 计算并保存 `mask`，供后续逻辑使用。
    mask = hand_mask(roi, cv2)
    # 从掩码中找出候选手部轮廓。
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    # 计算并保存 `min_area`，供后续逻辑使用。
    min_area = roi.shape[0] * roi.shape[1] * 0.015
    # 计算轮廓面积。
    contours = [contour for contour in contours if cv2.contourArea(contour) >= min_area]
    # 判断条件 `not contours` 是否成立。
    if not contours:
        # 返回 `resized_fallback`，把结果交给调用者。
        return resized_fallback

    # 同时计算并保存 `height, width` 这些值。
    height, width = roi.shape[:2]

    # 定义 contour_score 函数：给每个候选轮廓打分，帮助选出真正的手。
    def contour_score(contour):
        # 如果脸或衣服也进入绿色框，可能出现多个轮廓；这里优先选择更靠中下方的主轮廓。
        # 计算轮廓面积。
        area = cv2.contourArea(contour)
        # 计算轮廓外接矩形。
        x, y, w, h = cv2.boundingRect(contour)
        # 计算并保存 `center_y`，供后续逻辑使用。
        center_y = (y + h / 2) / max(1, height)
        # 计算并保存 `center_x`，供后续逻辑使用。
        center_x = (x + w / 2) / max(1, width)
        # 计算并保存 `center_bonus`，供后续逻辑使用。
        center_bonus = 1.0 - min(1.0, abs(center_x - 0.5) * 1.2)
        # 计算并保存 `lower_bonus`，供后续逻辑使用。
        lower_bonus = 0.65 + center_y
        # 返回 `area * lower_bonus * (0.75 + 0.25 * center_bonus)`，把结果交给调用者。
        return area * lower_bonus * (0.75 + 0.25 * center_bonus)

    # 计算并保存 `contour`，供后续逻辑使用。
    contour = max(contours, key=contour_score)
    # 计算轮廓外接矩形。
    x, y, w, h = cv2.boundingRect(contour)
    # 计算并保存 `clean_mask`，供后续逻辑使用。
    clean_mask = np.zeros(mask.shape, dtype=np.uint8)
    # 把选中的手部轮廓画到干净掩码上。
    cv2.drawContours(clean_mask, [contour], -1, 255, thickness=cv2.FILLED)
    # 膨胀掩码，保留更多手部边缘。
    clean_mask = cv2.dilate(clean_mask, np.ones((7, 7), np.uint8), iterations=1)
    # 计算并保存 `foreground`，供后续逻辑使用。
    foreground = np.zeros_like(roi)
    # 计算并保存 `foreground[clean_mask > 0]`，供后续逻辑使用。
    foreground[clean_mask > 0] = roi[clean_mask > 0]
    # 计算并保存 `cropped`，供后续逻辑使用。
    cropped = square_crop_with_margin(foreground, x, y, w, h)
    # 返回 `cv2.resize(cropped, (image_size, image_size))`，把结果交给调用者。
    return cv2.resize(cropped, (image_size, image_size))


# 定义 prepare_roi_for_model 函数：根据参数决定是否做手部前景预处理。
def prepare_roi_for_model(roi, cv2, args):
    # 默认启用手部前景提取；如需对比原图训练，可通过 --no-hand-preprocess 关闭。
    # 判断条件 `getattr(args, "hand_preprocess", True)` 是否成立。
    if getattr(args, "hand_preprocess", True):
        # 返回 `preprocess_hand_roi(roi, cv2, args.image_size)`，把结果交给调用者。
        return preprocess_hand_roi(roi, cv2, args.image_size)
    # 返回 `cv2.resize(roi, (args.image_size, args.image_size))`，把结果交给调用者。
    return cv2.resize(roi, (args.image_size, args.image_size))


# 定义 load_image_for_training 函数：读取训练图片并转换成模型训练数组。
def load_image_for_training(path_value, image_size: int, hand_preprocess: bool):
    # TensorFlow 数据管道中调用 OpenCV 读取中文路径图片，并执行和 UI 相同的预处理。
    # 计算并保存 `cv2`，供后续逻辑使用。
    cv2 = import_cv2()
    # 判断条件 `hasattr(path_value, "item")` 是否成立。
    if hasattr(path_value, "item"):
        # 计算并保存 `path_value`，供后续逻辑使用。
        path_value = path_value.item()
    # 计算并保存 `path`，供后续逻辑使用。
    path = path_value.decode("utf-8") if isinstance(path_value, bytes) else str(path_value)
    # 从文件读取原始字节，兼容中文路径。
    data = np.fromfile(path, dtype=np.uint8)
    # 把图片字节解码成 OpenCV 图像。
    image = cv2.imdecode(data, cv2.IMREAD_COLOR)
    # 判断条件 `image is None` 是否成立。
    if image is None:
        # 主动抛出错误，让调用者知道程序无法继续。
        raise ValueError(f"Could not read image: {path}")
    # 判断条件 `hand_preprocess` 是否成立。
    if hand_preprocess:
        # 计算并保存 `image`，供后续逻辑使用。
        image = preprocess_hand_roi(image, cv2, image_size)
    # 处理前面条件都不满足时的情况。
    else:
        # 把图像缩放到指定尺寸。
        image = cv2.resize(image, (image_size, image_size))
    # 转换图像颜色空间。
    rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    # 返回 `rgb.astype("float32")`，把结果交给调用者。
    return rgb.astype("float32")


# 定义 collect_one_class 函数：打开摄像头采集某一个类别的手势图片。
def collect_one_class(args, class_name: str):
    # 采集某一个类别的手势图片，例如 rock、paper 或 scissors。
    # 计算并保存 `cv2`，供后续逻辑使用。
    cv2 = import_cv2()
    # 计算并保存 `data_dir`，供后续逻辑使用。
    data_dir = Path(args.data_dir)
    # 执行“打开摄像头采集某一个类别的手势图片”中的这一行操作。
    ensure_dirs(data_dir)
    # 计算并保存 `save_dir`，供后续逻辑使用。
    save_dir = data_dir / class_name
    # 打开指定编号的摄像头。
    cap = cv2.VideoCapture(args.camera)
    # 判断条件 `not cap.isOpened()` 是否成立。
    if not cap.isOpened():
        # 主动抛出错误，让调用者知道程序无法继续。
        raise SystemExit(f"Could not open camera {args.camera}.")

    # 计算并保存 `existing`，供后续逻辑使用。
    existing = len(image_files(save_dir))
    # 计算并保存 `saved`，供后续逻辑使用。
    saved = 0
    # 计算并保存 `frame_count`，供后续逻辑使用。
    frame_count = 0
    # 计算并保存 `paused`，供后续逻辑使用。
    paused = True
    # 在控制台输出当前进度或状态信息。
    print(f"Collecting '{class_name}' into {save_dir}")
    # 在控制台输出当前进度或状态信息。
    print("Put your hand in the green square. Press SPACE to start/pause, q to quit.")

    # 只要 `saved < args.samples` 成立，就持续循环执行。
    while saved < args.samples:
        # 从摄像头读取一帧画面。
        ok, frame = cap.read()
        # 判断条件 `not ok` 是否成立。
        if not ok:
            # 跳出当前循环。
            break
        # 水平翻转画面，让摄像头预览像照镜子一样。
        frame = cv2.flip(frame, 1)  # 水平翻转后像照镜子，采集时更自然。
        # 同时计算并保存 `x1, y1, x2, y2` 这些值。
        x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
        # 计算并保存 `roi`，供后续逻辑使用。
        roi = frame[y1:y2, x1:x2]
        # 在画面上画出绿色手势采集框。
        cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 2)
        # 计算并保存 `status`，供后续逻辑使用。
        status = "PAUSED" if paused else "SAVING"
        # 计算并保存 `text`，供后续逻辑使用。
        text = f"{class_name} {saved}/{args.samples} {status}"
        # 把提示文字绘制到摄像头画面上。
        cv2.putText(frame, text, (20, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (30, 240, 80), 2)
        # 显示 OpenCV 预览窗口。
        cv2.imshow("gesture collector", frame)

        # 读取键盘按键，用来暂停、继续或退出。
        key = cv2.waitKey(1) & 0xFF
        # 判断条件 `key == ord("q")` 是否成立。
        if key == ord("q"):
            # 跳出当前循环。
            break
        # 判断条件 `key == 32` 是否成立。
        if key == 32:
            # 计算并保存 `paused`，供后续逻辑使用。
            paused = not paused
        # 判断条件 `paused` 是否成立。
        if paused:
            # 跳过本轮循环剩余代码，进入下一轮。
            continue

        # 计算并保存 `frame_count +`，供后续逻辑使用。
        frame_count += 1
        # 判断条件 `frame_count % args.every == 0` 是否成立。
        if frame_count % args.every == 0:
            # 每隔几帧保存一张图片，避免连续样本完全重复。
            # 把图像缩放到指定尺寸。
            img = cv2.resize(roi, (args.image_size, args.image_size))
            # 计算并保存 `out`，供后续逻辑使用。
            out = save_dir / f"{class_name}_{existing + saved:04d}.jpg"
            # 把图像编码成 JPG 数据。
            ok, encoded = cv2.imencode(".jpg", img)
            # 判断条件 `not ok` 是否成立。
            if not ok:
                # 主动抛出错误，让调用者知道程序无法继续。
                raise SystemExit(f"Could not encode image for {out}.")
            # 把编码后的图片写入磁盘，兼容中文路径。
            encoded.tofile(str(out))
            # 计算并保存 `saved +`，供后续逻辑使用。
            saved += 1

    # 释放摄像头资源。
    cap.release()
    # 关闭 OpenCV 创建的所有窗口。
    cv2.destroyAllWindows()
    # 在控制台输出当前进度或状态信息。
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
    # 计算并保存 `original_samples`，供后续逻辑使用。
    original_samples = args.samples
    # 判断条件 `args.replace_existing` 是否成立。
    if args.replace_existing:
        # 计算并保存 `selected_classes`，供后续逻辑使用。
        selected_classes = CLASSES if args.class_name == "all" else [args.class_name]
        # 遍历 `name`，逐项执行下面的逻辑。
        for name in selected_classes:
            # 执行“组织完整的数据采集流程”中的这一行操作。
            archive_class_images(Path(args.data_dir), name)

    # 判断条件 `args.target_count > 0` 是否成立。
    if args.target_count > 0:
        # 计算并保存 `counts`，供后续逻辑使用。
        counts = class_counts(Path(args.data_dir))
        # 计算并保存 `selected_classes`，供后续逻辑使用。
        selected_classes = CLASSES if args.class_name == "all" else [args.class_name]
        # 遍历 `name`，逐项执行下面的逻辑。
        for name in selected_classes:
            # 计算并保存 `remaining`，供后续逻辑使用。
            remaining = max(0, args.target_count - counts.get(name, 0))
            # 判断条件 `remaining <= 0` 是否成立。
            if remaining <= 0:
                # 在控制台输出当前进度或状态信息。
                print(f"{name} already has {counts.get(name, 0)} images, target reached.")
                # 跳过本轮循环剩余代码，进入下一轮。
                continue
            # 执行“组织完整的数据采集流程”中的这一行操作。
            input(f"\nPrepare gesture '{name}', need {remaining} more images, then press Enter.")
            # 计算并保存 `args.samples`，供后续逻辑使用。
            args.samples = remaining
            # 执行“组织完整的数据采集流程”中的这一行操作。
            collect_one_class(args, name)
        # 计算并保存 `args.samples`，供后续逻辑使用。
        args.samples = original_samples
        # 在控制台输出当前进度或状态信息。
        print("Current dataset counts:", class_counts(Path(args.data_dir)))
        # 返回函数计算出的结果。
        return

    # 判断条件 `args.class_name == "all"` 是否成立。
    if args.class_name == "all":
        # 遍历 `name`，逐项执行下面的逻辑。
        for name in CLASSES:
            # 执行“组织完整的数据采集流程”中的这一行操作。
            input(f"\nPrepare gesture '{name}', then press Enter.")
            # 执行“组织完整的数据采集流程”中的这一行操作。
            collect_one_class(args, name)
    # 处理前面条件都不满足时的情况。
    else:
        # 执行“组织完整的数据采集流程”中的这一行操作。
        collect_one_class(args, args.class_name)
    # 在控制台输出当前进度或状态信息。
    print("Current dataset counts:", class_counts(Path(args.data_dir)))


# 定义 split_image_paths 函数：把图片路径按训练集和验证集拆分。
def split_image_paths(data_dir: Path, validation_split: float, seed: int):
    # 分层划分数据集：每个类别都按相同比例拆成训练集和验证集，避免类别不均衡。
    # 计算并保存 `rng`，供后续逻辑使用。
    rng = random.Random(seed)
    # 同时计算并保存 `train_paths, train_labels, val_paths, val_labels` 这些值。
    train_paths, train_labels, val_paths, val_labels = [], [], [], []
    # 遍历 `label, name`，逐项执行下面的逻辑。
    for label, name in enumerate(CLASSES):
        # 计算并保存 `files`，供后续逻辑使用。
        files = image_files(data_dir / name)
        # 把当前类别下的图片顺序随机打乱
        rng.shuffle(files)
        # 计算并保存 `val_count`，供后续逻辑使用。
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
    # 返回 `train_paths, train_labels, val_paths, val_labels`，把结果交给调用者。
    return train_paths, train_labels, val_paths, val_labels


# 定义 decode_image 函数：在 TensorFlow 数据管道中解码单张图片。
def decode_image(path, label, image_size: int, hand_preprocess: bool):
    # 读取图片并转换为 TensorFlow 训练需要的图像张量和 one-hot 标签。
    # 判断条件 `hand_preprocess` 是否成立。
    if hand_preprocess:
        # 计算并保存 `image`，供后续逻辑使用。
        image = tf.numpy_function(
            # 继续填写上一行开始的多行参数或数据结构。
            lambda p: load_image_for_training(p, image_size, True),
            # 继续填写上一行开始的多行参数或数据结构。
            [path],
            # 继续填写上一行开始的多行参数或数据结构。
            tf.float32,
        # 结束上面的多行结构。
        )
        # 执行“在 TensorFlow 数据管道中解码单张图片”中的这一行操作。
        image.set_shape([image_size, image_size, 3])
    # 处理前面条件都不满足时的情况。
    else:
        # 计算并保存 `data`，供后续逻辑使用。
        data = tf.io.read_file(path)
        # 计算并保存 `image`，供后续逻辑使用。
        image = tf.io.decode_image(data, channels=3, expand_animations=False)
        # 执行“在 TensorFlow 数据管道中解码单张图片”中的这一行操作。
        image.set_shape([None, None, 3])
        # 计算并保存 `image`，供后续逻辑使用。
        image = tf.image.resize(image, [image_size, image_size])
    # 计算并保存 `label`，供后续逻辑使用。
    label = tf.one_hot(label, depth=len(CLASSES))
    # 返回 `image, label`，把结果交给调用者。
    return image, label


# 定义 make_dataset 函数：把路径和标签封装成可训练的数据集。
def make_dataset(paths, labels, image_size: int, batch_size: int, seed: int, training: bool, hand_preprocess: bool):
    # 构建 tf.data 数据管道；训练集打乱顺序，验证集保持固定。
    # 调用 TensorFlow 数据集接口组织训练数据。
    ds = tf.data.Dataset.from_tensor_slices((paths, labels))
    # 判断条件 `training` 是否成立。
    if training:
        # 计算并保存 `ds`，供后续逻辑使用。
        ds = ds.shuffle(len(paths), seed=seed, reshuffle_each_iteration=True)
    # 调用 TensorFlow 数据集接口组织训练数据。
    ds = ds.map(lambda p, y: decode_image(p, y, image_size, hand_preprocess), num_parallel_calls=tf.data.AUTOTUNE)
    # 返回 `ds.batch(batch_size).prefetch(tf.data.AUTOTUNE)`，把结果交给调用者。
    return ds.batch(batch_size).prefetch(tf.data.AUTOTUNE)


# 定义 make_datasets 函数：创建训练集、验证集和标签列表。
def make_datasets(data_dir: Path, image_size: int, batch_size: int, seed: int, validation_split: float, hand_preprocess: bool):
    # 同时创建训练集和验证集，并打印各类别样本数量，便于检查数据是否均衡。
    # 执行“创建训练集、验证集和标签列表”中的这一行操作。
    require_tf()
    # 同时计算并保存 `train_paths, train_labels, val_paths, val_labels` 这些值。
    train_paths, train_labels, val_paths, val_labels = split_image_paths(data_dir, validation_split, seed)
    # 计算并保存 `train_counts`，供后续逻辑使用。
    train_counts = {name: train_labels.count(i) for i, name in enumerate(CLASSES)}
    # 计算并保存 `val_counts`，供后续逻辑使用。
    val_counts = {name: val_labels.count(i) for i, name in enumerate(CLASSES)}
    # 在控制台输出当前进度或状态信息。
    print(f"Training split: {train_counts}")
    # 在控制台输出当前进度或状态信息。
    print(f"Validation split: {val_counts}")
    # 更新 Tk 变量，从而刷新界面文字。
    train_ds = make_dataset(train_paths, train_labels, image_size, batch_size, seed, training=True, hand_preprocess=hand_preprocess)
    # 更新 Tk 变量，从而刷新界面文字。
    val_ds = make_dataset(val_paths, val_labels, image_size, batch_size, seed, training=False, hand_preprocess=hand_preprocess)
    # 返回 `train_ds, val_ds, train_counts, val_counts`，把结果交给调用者。
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
        
        # 配置字典中 `time` 对应的显示或规则值。
        "time": datetime.now().isoformat(timespec="seconds"),
        # 配置字典中 `data_dir` 对应的显示或规则值。
        "data_dir": str(data_dir),
        # 配置字典中 `counts` 对应的显示或规则值。
        "counts": counts,
        # 配置字典中 `train_counts` 对应的显示或规则值。
        "train_counts": train_counts,
        # 配置字典中 `validation_counts` 对应的显示或规则值。
        "validation_counts": val_counts,
        # 配置字典中 `classes` 对应的显示或规则值。
        "classes": CLASSES,
        # 配置字典中 `image_size` 对应的显示或规则值。
        "image_size": args.image_size,
        # 配置字典中 `epochs_ran` 对应的显示或规则值。
        "epochs_ran": len(history.history["loss"]),
        # 配置字典中 `validation_loss` 对应的显示或规则值。
        "validation_loss": float(loss),
        # 配置字典中 `validation_accuracy` 对应的显示或规则值。
        "validation_accuracy": float(acc),
        # 配置字典中 `confusion_matrix_rows_true_columns_pred` 对应的显示或规则值。
        "confusion_matrix_rows_true_columns_pred": confusion.tolist(),
        # 配置字典中 `hand_preprocess` 对应的显示或规则值。
        "hand_preprocess": bool(args.hand_preprocess),
        # 配置字典中 `pretrained_model` 对应的显示或规则值。
        "pretrained_model": False,
        # 配置字典中 `framework` 对应的显示或规则值。
        "framework": f"TensorFlow {tf.__version__}",
    # 结束上面的多行结构。
    }
    # 把结果字典转换成格式化 JSON 文本。
    (output_dir / "run_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    # 在控制台输出当前进度或状态信息。
    print(f"Saved model to {output_dir / 'model.keras'}")
    # 在控制台输出当前进度或状态信息。
    print(f"Validation accuracy: {acc:.3f}")


# 定义 load_labels 函数：读取训练时保存的类别标签。
def load_labels(output_dir: Path):
    # 读取训练时保存的类别标签；如果文件不存在，则使用默认类别顺序。
    label_file = output_dir / "labels.txt"
    if label_file.exists():
        return [line.strip() for line in label_file.read_text(encoding="utf-8").splitlines() if line.strip()]
    return CLASSES


# 定义 winning_response 函数：根据猜拳规则生成电脑获胜手势。
def winning_response(label: str):
    # 根据识别结果返回电脑应该出的获胜手势。
    # 判断条件 `label not in WINNING_MOVE` 是否成立。
    if label not in WINNING_MOVE:
        # 返回 `"waiting", "请把清晰手势放在绿色方框内。"`，把结果交给调用者。
        return "waiting", "请把清晰手势放在绿色方框内。"
    # 计算并保存 `response`，供后续逻辑使用。
    response = WINNING_MOVE[label]
    # 返回 `response, f"{DISPLAY_NAMES[response]} 可以赢 {DISPLAY_NAMES[label]}。"`，把结果交给调用者。
    return response, f"{DISPLAY_NAMES[response]} 可以赢 {DISPLAY_NAMES[label]}。"


# 定义 load_predictor 函数：加载预测阶段需要的模型和标签。
def load_predictor(args):
    # 加载已经训练好的 Keras 模型和标签文件。
    # 执行“加载预测阶段需要的模型和标签”中的这一行操作。
    require_tf()
    # 计算并保存 `output_dir`，供后续逻辑使用。
    output_dir = Path(args.output_dir)
    # 计算并保存 `model_path`，供后续逻辑使用。
    model_path = Path(args.model) if args.model else output_dir / "model.keras"
    # 判断条件 `not model_path.exists()` 是否成立。
    if not model_path.exists():
        # 主动抛出错误，让调用者知道程序无法继续。
        raise SystemExit(f"Model not found: {model_path}. Train first.")
    # 计算并保存 `labels`，供后续逻辑使用。
    labels = load_labels(output_dir)
    # 调用 Keras 接口搭建、训练或加载模型。
    model = tf.keras.models.load_model(model_path)
    # 返回 `model, labels`，把结果交给调用者。
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


# 定义 predict_roi_probs 函数：对单个 ROI 输出模型概率。
def predict_roi_probs(model, roi, cv2, args):
    # 对单张 ROI 图像进行预处理，并输出 rock/paper/scissors 三类概率。
    # 计算并保存 `img`，供后续逻辑使用。
    img = prepare_roi_for_model(roi, cv2, args)
    # 转换图像颜色空间。
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    # 返回 `model.predict(np.expand_dims(rgb.astype("float32"), axis=0), verbose=0)[0]`，把结果交给调用者。
    return model.predict(np.expand_dims(rgb.astype("float32"), axis=0), verbose=0)[0]


# 定义 predict_rois_probs 函数：批量预测多个 ROI 的概率。
def predict_rois_probs(model, rois, cv2, args):
    # 对多帧 ROI 批量预测，UI 最终锁定结果时会对这些概率取平均。
    # 计算并保存 `batch`，供后续逻辑使用。
    batch = []
    # 遍历 `roi`，逐项执行下面的逻辑。
    for roi in rois:
        # 计算并保存 `img`，供后续逻辑使用。
        img = prepare_roi_for_model(roi, cv2, args)
        # 转换图像颜色空间。
        rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        # 执行“批量预测多个 ROI 的概率”中的这一行操作。
        batch.append(rgb.astype("float32"))
    # 判断条件 `not batch` 是否成立。
    if not batch:
        # 返回 `np.zeros((1, len(CLASSES)), dtype=np.float32)`，把结果交给调用者。
        return np.zeros((1, len(CLASSES)), dtype=np.float32)
    # 返回 `model.predict(np.stack(batch, axis=0), verbose=0)`，把结果交给调用者。
    return model.predict(np.stack(batch, axis=0), verbose=0)


# 定义 choose_label 函数：根据概率、阈值和剪刀修正规则选择最终标签。
def choose_label(labels, probs, roi, cv2, args):
    # 从概率中选择最终类别；置信度太低时返回 uncertain。
    # 计算并保存 `idx`，供后续逻辑使用。
    idx = int(np.argmax(probs))
    # 计算并保存 `confidence`，供后续逻辑使用。
    confidence = float(probs[idx])
    # 计算并保存 `label`，供后续逻辑使用。
    label = labels[idx]
    # 计算并保存 `corrected`，供后续逻辑使用。
    corrected = False

    # 判断条件 `getattr(args, "scissors_correction", True) and label == "paper" and "scissors" in labels and roi is not None` 是否成立。
    if getattr(args, "scissors_correction", True) and label == "paper" and "scissors" in labels and roi is not None:
        # 剪刀和布都属于张开手指的形态，容易混淆；这里做一个保守的剪刀修正。
        # 计算并保存 `scissors_index`，供后续逻辑使用。
        scissors_index = labels.index("scissors")
        # 计算并保存 `scissors_prob`，供后续逻辑使用。
        scissors_prob = float(probs[scissors_index])
        # 计算并保存 `skin_threshold`，供后续逻辑使用。
        skin_threshold = getattr(args, "scissors_skin_threshold", 0.04)
        # 计算并保存 `min_prob`，供后续逻辑使用。
        min_prob = getattr(args, "scissors_min_prob", 0.24)
        # 判断条件 `skin_fraction(roi, cv2) >= skin_threshold and scissors_prob >= min_prob` 是否成立。
        if skin_fraction(roi, cv2) >= skin_threshold and scissors_prob >= min_prob:
            # 计算并保存 `label`，供后续逻辑使用。
            label = "scissors"
            # 计算并保存 `confidence`，供后续逻辑使用。
            confidence = scissors_prob
            # 计算并保存 `corrected`，供后续逻辑使用。
            corrected = True

    # 判断条件 `confidence < getattr(args, "min_confidence", 0.55) and not corrected` 是否成立。
    if confidence < getattr(args, "min_confidence", 0.55) and not corrected:
        # 计算并保存 `label`，供后续逻辑使用。
        label = "uncertain"
    # 返回 `label, confidence, probs`，把结果交给调用者。
    return label, confidence, probs


# 定义 classify_roi 函数：把单帧 ROI 转成平滑后的分类结果。
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


# 定义 predict_camera 函数：使用 OpenCV 窗口进行实时预测。
def predict_camera(args):
    # 调试用的实时摄像头预测窗口；最终展示主要使用 run_ui。
    # 执行“使用 OpenCV 窗口进行实时预测”中的这一行操作。
    require_tf()
    # 计算并保存 `cv2`，供后续逻辑使用。
    cv2 = import_cv2()
    # 同时计算并保存 `model, labels` 这些值。
    model, labels = load_predictor(args)
    # 打开指定编号的摄像头。
    cap = cv2.VideoCapture(args.camera)
    # 判断条件 `not cap.isOpened()` 是否成立。
    if not cap.isOpened():
        # 主动抛出错误，让调用者知道程序无法继续。
        raise SystemExit(f"Could not open camera {args.camera}.")
    # 在控制台输出当前进度或状态信息。
    print("Running camera prediction. Press q to quit.")
    # 计算并保存 `recent_probs`，供后续逻辑使用。
    recent_probs = deque(maxlen=max(1, args.smooth))

    # 只要 `True` 成立，就持续循环执行。
    while True:
        # 从摄像头读取一帧画面。
        ok, frame = cap.read()
        # 判断条件 `not ok` 是否成立。
        if not ok:
            # 跳出当前循环。
            break
        # 水平翻转画面，让摄像头预览像照镜子一样。
        frame = cv2.flip(frame, 1)
        # 同时计算并保存 `x1, y1, x2, y2` 这些值。
        x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
        # 计算并保存 `roi`，供后续逻辑使用。
        roi = frame[y1:y2, x1:x2]
        # 同时计算并保存 `label, confidence, probs` 这些值。
        label, confidence, probs = classify_roi(model, labels, roi, cv2, args, recent_probs)
        # 同时计算并保存 `response, reason` 这些值。
        response, reason = winning_response(label)

        # 在画面上画出绿色手势采集框。
        cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 2)
        # 计算并保存 `text`，供后续逻辑使用。
        text = f"You: {label} {confidence:.2f}"
        # 把提示文字绘制到摄像头画面上。
        cv2.putText(frame, text, (20, 38), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (30, 240, 80), 2)
        # 把提示文字绘制到摄像头画面上。
        cv2.putText(frame, f"Win with: {response}", (20, 72), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (80, 220, 255), 2)
        # 把提示文字绘制到摄像头画面上。
        cv2.putText(frame, reason, (20, 104), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (240, 240, 240), 2)
        # 计算并保存 `y`，供后续逻辑使用。
        y = 138
        # 遍历 `name, p`，逐项执行下面的逻辑。
        for name, p in zip(labels, probs):
            # 把提示文字绘制到摄像头画面上。
            cv2.putText(frame, f"{name:<8} {p:.2f}", (20, y), cv2.FONT_HERSHEY_SIMPLEX, 0.65, (240, 240, 240), 2)
            # 计算并保存 `y +`，供后续逻辑使用。
            y += 28
        # 显示 OpenCV 预览窗口。
        cv2.imshow("gesture predictor", frame)
        # 判断条件 `cv2.waitKey(1) & 0xFF == ord("q")` 是否成立。
        if cv2.waitKey(1) & 0xFF == ord("q"):
            # 跳出当前循环。
            break

    # 释放摄像头资源。
    cap.release()
    # 关闭 OpenCV 创建的所有窗口。
    cv2.destroyAllWindows()


# 定义 draw_probability_bars 函数：在 Tk 画布上绘制三类概率条。
def draw_probability_bars(canvas, labels, probs):
    # 在 Tkinter 侧边栏绘制三类概率条。
    # 执行“在 Tk 画布上绘制三类概率条”中的这一行操作。
    canvas.delete("all")
    # 计算并保存 `width`，供后续逻辑使用。
    width = int(canvas["width"])
    # 计算并保存 `colors`，供后续逻辑使用。
    colors = {"rock": "#7dd3fc", "paper": "#86efac", "scissors": "#fca5a5"}
    # 遍历 `index, (name, prob)`，逐项执行下面的逻辑。
    for index, (name, prob) in enumerate(zip(labels, probs)):
        # 计算并保存 `top`，供后续逻辑使用。
        top = 12 + index * 48
        # 计算并保存 `bar_width`，供后续逻辑使用。
        bar_width = int((width - 130) * float(prob))
        # 同时计算并保存 `canvas.create_text(10, top + 13, anchor` 这些值。
        canvas.create_text(10, top + 13, anchor="w", text=DISPLAY_NAMES.get(name, name), fill="#dbeafe", font=("Microsoft YaHei UI", 11, "bold"))
        # 同时计算并保存 `canvas.create_rectangle(96, top, width - 20, top + 26, fill` 这些值。
        canvas.create_rectangle(96, top, width - 20, top + 26, fill="#1f2937", outline="#334155")
        # 同时计算并保存 `canvas.create_rectangle(96, top, 96 + bar_width, top + 26, fill` 这些值。
        canvas.create_rectangle(96, top, 96 + bar_width, top + 26, fill=colors.get(name, "#93c5fd"), outline="")
        # 同时计算并保存 `canvas.create_text(width - 16, top + 13, anchor` 这些值。
        canvas.create_text(width - 16, top + 13, anchor="e", text=f"{prob:.0%}", fill="#e5e7eb", font=("Microsoft YaHei UI", 10))


# 定义 run_ui 函数：启动带倒计时和比赛结果的图形界面。
def run_ui(args):
    # 图形化比赛界面：倒计时、采样、锁定识别结果，并显示电脑获胜手势。
    # 执行“启动带倒计时和比赛结果的图形界面”中的这一行操作。
    require_tf()
    # 计算并保存 `cv2`，供后续逻辑使用。
    cv2 = import_cv2()
    # 开始执行可能失败的代码，并准备捕获异常。
    try:
        # 导入 tkinter，用来构建桌面图形界面。
        import tkinter as tk
        # 导入 Pillow 的图像桥接工具，把 OpenCV 画面显示到 Tk。
        from PIL import Image, ImageTk
    # 捕获指定异常，给出更友好的处理方式。
    except ImportError as exc:
        # 主动抛出错误，让调用者知道程序无法继续。
        raise SystemExit("Tkinter and Pillow are required for UI mode.") from exc

    # 同时计算并保存 `model, labels` 这些值。
    model, labels = load_predictor(args)
    # 计算并保存 `output_dir`，供后续逻辑使用。
    output_dir = Path(args.output_dir)
    # 创建目录；如果已经存在就直接复用。
    output_dir.mkdir(parents=True, exist_ok=True)
    # 打开指定编号的摄像头。
    cap = cv2.VideoCapture(args.camera)
    # 判断条件 `not cap.isOpened()` 是否成立。
    if not cap.isOpened():
        # 主动抛出错误，让调用者知道程序无法继续。
        raise SystemExit(f"Could not open camera {args.camera}.")

    # 计算并保存 `root`，供后续逻辑使用。
    root = tk.Tk()
    # 执行“启动带倒计时和比赛结果的图形界面”中的这一行操作。
    root.title("石头剪刀布手势比赛")
    # 更新界面控件的显示状态。
    root.configure(bg="#0f172a")
    # 执行“启动带倒计时和比赛结果的图形界面”中的这一行操作。
    root.minsize(980, 620)

    # 创建文本或图片标签控件。
    video_panel = tk.Label(root, bg="#020617")
    # 把 Tk 控件放入窗口布局中。
    video_panel.pack(side="left", fill="both", expand=True, padx=(18, 10), pady=18)

    # 创建容器控件，用来组织界面布局。
    side = tk.Frame(root, bg="#0f172a", width=330)
    # 把 Tk 控件放入窗口布局中。
    side.pack(side="right", fill="y", padx=(8, 18), pady=18)
    # 执行“启动带倒计时和比赛结果的图形界面”中的这一行操作。
    side.pack_propagate(False)

    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, text="石头剪刀布比赛", fg="#f8fafc", bg="#0f172a", font=("Microsoft YaHei UI", 24, "bold")).pack(anchor="w")
    # 创建文本或图片标签控件。
    tk.Label(
        # 继续填写上一行开始的多行参数或数据结构。
        side,
        # 计算并保存 `text`，供后续逻辑使用。
        text="点击开始，把手势放进绿色方框。出现“出拳！”后保持一下，随后锁定结果。",
        # 计算并保存 `fg`，供后续逻辑使用。
        fg="#94a3b8",
        # 计算并保存 `bg`，供后续逻辑使用。
        bg="#0f172a",
        # 计算并保存 `wraplength`，供后续逻辑使用。
        wraplength=300,
        # 计算并保存 `justify`，供后续逻辑使用。
        justify="left",
        # 计算并保存 `font`，供后续逻辑使用。
        font=("Microsoft YaHei UI", 11),
    # 把 Tk 控件放入窗口布局中。
    ).pack(anchor="w", pady=(4, 22))

    # 计算并保存 `countdown_var`，供后续逻辑使用。
    countdown_var = tk.StringVar(value="准备")
    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, textvariable=countdown_var, fg="#fef3c7", bg="#0f172a", font=("Microsoft YaHei UI", 40, "bold")).pack(anchor="center", pady=(0, 12))

    # 创建按钮控件。
    start_button = tk.Button(
        # 继续填写上一行开始的多行参数或数据结构。
        side,
        # 计算并保存 `text`，供后续逻辑使用。
        text="开始比赛",
        # 计算并保存 `command`，供后续逻辑使用。
        command=lambda: start_round(),
        # 计算并保存 `bg`，供后续逻辑使用。
        bg="#2563eb",
        # 计算并保存 `fg`，供后续逻辑使用。
        fg="#ffffff",
        # 计算并保存 `activebackground`，供后续逻辑使用。
        activebackground="#1d4ed8",
        # 计算并保存 `activeforeground`，供后续逻辑使用。
        activeforeground="#ffffff",
        # 计算并保存 `bd`，供后续逻辑使用。
        bd=0,
        # 计算并保存 `padx`，供后续逻辑使用。
        padx=18,
        # 计算并保存 `pady`，供后续逻辑使用。
        pady=10,
        # 计算并保存 `font`，供后续逻辑使用。
        font=("Microsoft YaHei UI", 13, "bold"),
    # 结束上面的多行结构。
    )
    # 把 Tk 控件放入窗口布局中。
    start_button.pack(fill="x", pady=(0, 22))

    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, text="你的手势", fg="#93c5fd", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w")
    # 计算并保存 `gesture_emoji_var`，供后续逻辑使用。
    gesture_emoji_var = tk.StringVar(value=GESTURE_EMOJI["waiting"])
    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, textvariable=gesture_emoji_var, fg="#f8fafc", bg="#0f172a", font=("Segoe UI Emoji", 78)).pack(anchor="center")
    # 计算并保存 `gesture_var`，供后续逻辑使用。
    gesture_var = tk.StringVar(value="等待")
    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, textvariable=gesture_var, fg="#f8fafc", bg="#0f172a", font=("Microsoft YaHei UI", 18, "bold")).pack(anchor="center", pady=(0, 12))

    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, text="电脑出招", fg="#fbbf24", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w")
    # 计算并保存 `response_emoji_var`，供后续逻辑使用。
    response_emoji_var = tk.StringVar(value=GESTURE_EMOJI["waiting"])
    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, textvariable=response_emoji_var, fg="#fde68a", bg="#0f172a", font=("Segoe UI Emoji", 78)).pack(anchor="center")
    # 计算并保存 `response_var`，供后续逻辑使用。
    response_var = tk.StringVar(value="等待")
    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, textvariable=response_var, fg="#fde68a", bg="#0f172a", font=("Microsoft YaHei UI", 18, "bold")).pack(anchor="center", pady=(0, 10))

    # 计算并保存 `reason_var`，供后续逻辑使用。
    reason_var = tk.StringVar(value="点击开始比赛。")
    # 创建文本或图片标签控件。
    tk.Label(
        # 继续填写上一行开始的多行参数或数据结构。
        side,
        # 计算并保存 `textvariable`，供后续逻辑使用。
        textvariable=reason_var,
        # 计算并保存 `fg`，供后续逻辑使用。
        fg="#cbd5e1",
        # 计算并保存 `bg`，供后续逻辑使用。
        bg="#0f172a",
        # 计算并保存 `wraplength`，供后续逻辑使用。
        wraplength=300,
        # 计算并保存 `justify`，供后续逻辑使用。
        justify="left",
        # 计算并保存 `font`，供后续逻辑使用。
        font=("Microsoft YaHei UI", 12),
    # 把 Tk 控件放入窗口布局中。
    ).pack(anchor="w", pady=(0, 22))

    # 计算并保存 `confidence_var`，供后续逻辑使用。
    confidence_var = tk.StringVar(value="置信度：--")
    # 把 Tk 控件放入窗口布局中。
    tk.Label(side, textvariable=confidence_var, fg="#a7f3d0", bg="#0f172a", font=("Microsoft YaHei UI", 12, "bold")).pack(anchor="w", pady=(0, 8))
    # 创建画布，用来绘制图像或概率条。
    bars = tk.Canvas(side, width=300, height=160, bg="#0f172a", highlightthickness=0)
    # 把 Tk 控件放入窗口布局中。
    bars.pack(anchor="w", pady=(0, 22))

    # 创建文本或图片标签控件。
    tk.Label(
        # 继续填写上一行开始的多行参数或数据结构。
        side,
        # 计算并保存 `text`，供后续逻辑使用。
        text="提示：出拳后保持约 1 秒，不要马上收手；尽量保持和采集数据时相同的距离、角度和光照。",
        # 计算并保存 `fg`，供后续逻辑使用。
        fg="#64748b",
        # 计算并保存 `bg`，供后续逻辑使用。
        bg="#0f172a",
        # 计算并保存 `wraplength`，供后续逻辑使用。
        wraplength=300,
        # 计算并保存 `justify`，供后续逻辑使用。
        justify="left",
        # 计算并保存 `font`，供后续逻辑使用。
        font=("Microsoft YaHei UI", 10),
    # 把 Tk 控件放入窗口布局中。
    ).pack(anchor="w", side="bottom")

    # 计算并保存 `recent_probs`，供后续逻辑使用。
    recent_probs = deque(maxlen=max(1, args.smooth))
    # 计算并保存 `round_rois`，供后续逻辑使用。
    round_rois = deque(maxlen=max(1, args.final_window))
    # 计算并保存 `running`，供后续逻辑使用。
    running = {"value": True}
    # 计算并保存 `state`，供后续逻辑使用。
    state = {"phase": "ready", "deadline": 0.0, "capture_deadline": 0.0, "last_roi": None}

    # 定义 show_frame 函数：把 OpenCV 图像刷新到 Tk 窗口中。
    def show_frame(frame):
        # OpenCV 图像是 BGR，Tkinter 显示前需要转为 RGB。
        # 转换图像颜色空间。
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        # 计算并保存 `image`，供后续逻辑使用。
        image = Image.fromarray(rgb_frame)
        # 执行“把 OpenCV 图像刷新到 Tk 窗口中”中的这一行操作。
        image.thumbnail((720, 560), Image.Resampling.LANCZOS)
        # 计算并保存 `photo`，供后续逻辑使用。
        photo = ImageTk.PhotoImage(image=image)
        # 更新界面控件的显示状态。
        video_panel.configure(image=photo)
        # 计算并保存 `video_panel.image`，供后续逻辑使用。
        video_panel.image = photo

    # 定义 set_waiting_visuals 函数：把界面重置为等待开局状态。
    def set_waiting_visuals():
        # 空闲状态：不显示预测结果，等待用户点击开始。
        # 更新 Tk 变量，从而刷新界面文字。
        countdown_var.set("准备")
        # 更新 Tk 变量，从而刷新界面文字。
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        # 更新 Tk 变量，从而刷新界面文字。
        gesture_var.set("等待")
        # 更新 Tk 变量，从而刷新界面文字。
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        # 更新 Tk 变量，从而刷新界面文字。
        response_var.set("等待")
        # 更新 Tk 变量，从而刷新界面文字。
        reason_var.set("点击开始比赛。")
        # 更新 Tk 变量，从而刷新界面文字。
        confidence_var.set("置信度：--")
        # 执行“把界面重置为等待开局状态”中的这一行操作。
        draw_probability_bars(bars, labels, np.zeros(len(labels)))

    # 定义 set_result_visuals 函数：把识别结果和电脑回应显示到界面上。
    def set_result_visuals(label, confidence, probs, random_result=False):
        # 锁定结果后，更新用户手势、电脑出拳、置信度和概率条。
        # 同时计算并保存 `response, reason` 这些值。
        response, reason = winning_response(label)
        # 更新 Tk 变量，从而刷新界面文字。
        gesture_emoji_var.set(GESTURE_EMOJI.get(label, GESTURE_EMOJI["uncertain"]))
        # 判断条件 `random_result` 是否成立。
        if random_result:
            # 更新 Tk 变量，从而刷新界面文字。
            gesture_var.set(f"随机：{DISPLAY_NAMES.get(label, label)}")
        # 处理前面条件都不满足时的情况。
        else:
            # 更新 Tk 变量，从而刷新界面文字。
            gesture_var.set(f"{DISPLAY_NAMES.get(label, label)}  {confidence:.0%}")
        # 更新 Tk 变量，从而刷新界面文字。
        response_emoji_var.set(GESTURE_EMOJI.get(response, GESTURE_EMOJI["waiting"]))
        # 更新 Tk 变量，从而刷新界面文字。
        response_var.set(DISPLAY_NAMES.get(response, response))
        # 更新 Tk 变量，从而刷新界面文字。
        reason_var.set("未能稳定识别，已随机选择一个结果。" if random_result else reason)
        # 更新 Tk 变量，从而刷新界面文字。
        confidence_var.set("置信度：随机结果" if random_result else f"置信度：{confidence:.1%}")
        # 执行“把识别结果和电脑回应显示到界面上”中的这一行操作。
        draw_probability_bars(bars, labels, probs)

    # 定义 start_round 函数：开始一局新的倒计时比赛。
    def start_round():
        # 开始新一局：清空上一局缓存，进入倒计时阶段。
        # 执行“开始一局新的倒计时比赛”中的这一行操作。
        recent_probs.clear()
        # 执行“开始一局新的倒计时比赛”中的这一行操作。
        round_rois.clear()
        # 计算并保存 `state["phase"]`，供后续逻辑使用。
        state["phase"] = "countdown"
        # 计算并保存 `state["deadline"]`，供后续逻辑使用。
        state["deadline"] = perf_counter() + max(0.5, float(args.countdown))
        # 计算并保存 `state["capture_deadline"]`，供后续逻辑使用。
        state["capture_deadline"] = 0.0
        # 计算并保存 `state["last_roi"]`，供后续逻辑使用。
        state["last_roi"] = None
        # 更新 Tk 变量，从而刷新界面文字。
        countdown_var.set(str(int(np.ceil(args.countdown))))
        # 更新 Tk 变量，从而刷新界面文字。
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        # 更新 Tk 变量，从而刷新界面文字。
        gesture_var.set("结果锁定后显示")
        # 更新 Tk 变量，从而刷新界面文字。
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        # 更新 Tk 变量，从而刷新界面文字。
        response_var.set("结果锁定后显示")
        # 更新 Tk 变量，从而刷新界面文字。
        reason_var.set("请把手势保持在绿色方框内，倒计时结束后才显示结果。")
        # 更新 Tk 变量，从而刷新界面文字。
        confidence_var.set("置信度：隐藏")
        # 执行“开始一局新的倒计时比赛”中的这一行操作。
        draw_probability_bars(bars, labels, np.zeros(len(labels)))
        # 更新界面控件的显示状态。
        start_button.configure(text="倒计时中", state="disabled", bg="#475569")

    # 定义 lock_round 函数：倒计时结束后锁定并保存本局结果。
    def lock_round(frame):
        # 倒计时结束后锁定结果：对采样到的多帧预测概率取平均。
        # 判断条件 `round_rois` 是否成立。
        if round_rois:
            # 计算并保存 `predictions`，供后续逻辑使用。
            predictions = predict_rois_probs(model, list(round_rois), cv2, args)
            # 计算并保存 `final_probs`，供后续逻辑使用。
            final_probs = np.mean(predictions, axis=0)
        # 处理前面条件都不满足时的情况。
        else:
            # 计算并保存 `final_probs`，供后续逻辑使用。
            final_probs = np.zeros(len(labels), dtype=np.float32)
        # 同时计算并保存 `label, confidence, final_probs` 这些值。
        label, confidence, final_probs = choose_label(labels, final_probs, state["last_roi"], cv2, args)
        # 计算并保存 `random_result`，供后续逻辑使用。
        random_result = label == "uncertain"
        # 判断条件 `random_result` 是否成立。
        if random_result:
            # 如果没有稳定识别出来，就随机给出一个手势，保证比赛流程有结果。
            # 计算并保存 `label`，供后续逻辑使用。
            label = random.choice(list(labels))
            # 计算并保存 `confidence`，供后续逻辑使用。
            confidence = 0.0
            # 计算并保存 `final_probs`，供后续逻辑使用。
            final_probs = np.zeros(len(labels), dtype=np.float32)
            # 计算并保存 `final_probs[labels.index(label)]`，供后续逻辑使用。
            final_probs[labels.index(label)] = 1.0
        # 同时计算并保存 `response, reason` 这些值。
        response, reason = winning_response(label)
        # 同时计算并保存 `set_result_visuals(label, confidence, final_probs, random_result` 这些值。
        set_result_visuals(label, confidence, final_probs, random_result=random_result)
        # 计算并保存 `result`，供后续逻辑使用。
        result = {
            # 配置字典中 `time` 对应的显示或规则值。
            "time": datetime.now().isoformat(timespec="seconds"),
            # 配置字典中 `label` 对应的显示或规则值。
            "label": label,
            # 配置字典中 `display_label` 对应的显示或规则值。
            "display_label": DISPLAY_NAMES.get(label, label),
            # 配置字典中 `response` 对应的显示或规则值。
            "response": response,
            # 配置字典中 `display_response` 对应的显示或规则值。
            "display_response": DISPLAY_NAMES.get(response, response),
            # 配置字典中 `confidence` 对应的显示或规则值。
            "confidence": float(confidence),
            # 配置字典中 `random_result` 对应的显示或规则值。
            "random_result": bool(random_result),
            # 配置字典中 `probabilities` 对应的显示或规则值。
            "probabilities": {name: float(prob) for name, prob in zip(labels, final_probs)},
            # 配置字典中 `sampled_frames` 对应的显示或规则值。
            "sampled_frames": len(round_rois),
            # 配置字典中 `capture_duration` 对应的显示或规则值。
            "capture_duration": float(args.capture_duration),
        # 结束上面的多行结构。
        }
        # 开始执行可能失败的代码，并准备捕获异常。
        try:
            # 把文本内容写入文件。
            (output_dir / "latest_ui_result.json").write_text(
                # 把结果字典转换成格式化 JSON 文本。
                json.dumps(result, ensure_ascii=False, indent=2),
                # 计算并保存 `encoding`，供后续逻辑使用。
                encoding="utf-8",
            # 结束上面的多行结构。
            )
            # 判断条件 `state["last_roi"] is not None` 是否成立。
            if state["last_roi"] is not None:
                # 保存调试图：原始 ROI 和模型真正看到的预处理后输入。
                # 执行“倒计时结束后锁定并保存本局结果”中的这一行操作。
                save_jpg(output_dir / "latest_ui_roi.jpg", state["last_roi"], cv2)
                # 执行“倒计时结束后锁定并保存本局结果”中的这一行操作。
                save_jpg(output_dir / "latest_ui_model_input.jpg", prepare_roi_for_model(state["last_roi"], cv2, args), cv2)
            # 执行“倒计时结束后锁定并保存本局结果”中的这一行操作。
            save_jpg(output_dir / "latest_ui_frame.jpg", frame, cv2)
        # 捕获指定异常，给出更友好的处理方式。
        except OSError as exc:
            # 在控制台输出当前进度或状态信息。
            print(f"保存本局调试图片失败：{exc}")
        # 更新 Tk 变量，从而刷新界面文字。
        countdown_var.set("已锁定")
        # 判断条件 `not random_result` 是否成立。
        if not random_result:
            # 更新 Tk 变量，从而刷新界面文字。
            reason_var.set(reason)
        # 更新界面控件的显示状态。
        start_button.configure(text="下一局", state="normal", bg="#2563eb")
        # 计算并保存 `state["phase"]`，供后续逻辑使用。
        state["phase"] = "locked"

        # 执行“倒计时结束后锁定并保存本局结果”中的这一行操作。
        show_frame(frame)

    # 定义 start_capture 函数：进入短暂采样阶段以获得稳定画面。
    def start_capture():
        # 倒计时结束后短暂采样，避免用户最后一秒出手造成单帧模糊。
        # 执行“进入短暂采样阶段以获得稳定画面”中的这一行操作。
        round_rois.clear()
        # 计算并保存 `state["phase"]`，供后续逻辑使用。
        state["phase"] = "capture"
        # 计算并保存 `state["capture_deadline"]`，供后续逻辑使用。
        state["capture_deadline"] = perf_counter() + max(0.05, float(args.capture_duration))
        # 更新 Tk 变量，从而刷新界面文字。
        countdown_var.set("出拳！")
        # 更新 Tk 变量，从而刷新界面文字。
        gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
        # 更新 Tk 变量，从而刷新界面文字。
        gesture_var.set("正在锁定")
        # 更新 Tk 变量，从而刷新界面文字。
        response_emoji_var.set(GESTURE_EMOJI["waiting"])
        # 更新 Tk 变量，从而刷新界面文字。
        response_var.set("正在锁定")
        # 更新 Tk 变量，从而刷新界面文字。
        reason_var.set("请保持刚出的手势不动，正在采集稳定画面。")
        # 更新 Tk 变量，从而刷新界面文字。
        confidence_var.set("置信度：隐藏")
        # 执行“进入短暂采样阶段以获得稳定画面”中的这一行操作。
        draw_probability_bars(bars, labels, np.zeros(len(labels)))

    # 定义 on_close 函数：关闭窗口并释放摄像头资源。
    def on_close():
        # 关闭窗口时释放摄像头资源。
        # 计算并保存 `running["value"]`，供后续逻辑使用。
        running["value"] = False
        # 释放摄像头资源。
        cap.release()
        # 执行“关闭窗口并释放摄像头资源”中的这一行操作。
        root.destroy()

    # 定义 update_frame 函数：持续读取摄像头并驱动 UI 状态更新。
    def update_frame():
        # UI 主循环：持续读取摄像头画面，并根据阶段更新界面状态。
        # 判断条件 `not running["value"]` 是否成立。
        if not running["value"]:
            # 返回函数计算出的结果。
            return
        # 判断条件 `state["phase"] == "locked"` 是否成立。
        if state["phase"] == "locked":
            # 安排 Tk 在短暂延迟后继续刷新画面。
            root.after(80, update_frame)
            # 返回函数计算出的结果。
            return
        # 从摄像头读取一帧画面。
        ok, frame = cap.read()
        # 判断条件 `ok` 是否成立。
        if ok:
            # 水平翻转画面，让摄像头预览像照镜子一样。
            frame = cv2.flip(frame, 1)
            # 同时计算并保存 `x1, y1, x2, y2` 这些值。
            x1, y1, x2, y2 = center_square(frame, args.roi_ratio)
            # 计算并保存 `roi`，供后续逻辑使用。
            roi = frame[y1:y2, x1:x2]
            # 在画面上画出绿色手势采集框。
            cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 80), 3)

            # 判断条件 `state["phase"] == "countdown"` 是否成立。
            if state["phase"] == "countdown":
                # 计算并保存 `remaining`，供后续逻辑使用。
                remaining = state["deadline"] - perf_counter()
                # 判断条件 `remaining <= 0` 是否成立。
                if remaining <= 0:
                    # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
                    start_capture()
                    # 把提示文字绘制到摄像头画面上。
                    cv2.putText(frame, "!", (42, 112), cv2.FONT_HERSHEY_SIMPLEX, 3.0, (80, 220, 255), 7)
                    # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
                    show_frame(frame)
                    # 安排 Tk 在短暂延迟后继续刷新画面。
                    root.after(80, update_frame)
                    # 返回函数计算出的结果。
                    return
                # 计算并保存 `count`，供后续逻辑使用。
                count = int(np.ceil(remaining))
                # 更新 Tk 变量，从而刷新界面文字。
                countdown_var.set(str(count))
                # 更新 Tk 变量，从而刷新界面文字。
                gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
                # 更新 Tk 变量，从而刷新界面文字。
                gesture_var.set("结果锁定后显示")
                # 更新 Tk 变量，从而刷新界面文字。
                response_emoji_var.set(GESTURE_EMOJI["waiting"])
                # 更新 Tk 变量，从而刷新界面文字。
                response_var.set("结果锁定后显示")
                # 更新 Tk 变量，从而刷新界面文字。
                reason_var.set("请保持手势，锁定前不显示预测结果。")
                # 更新 Tk 变量，从而刷新界面文字。
                confidence_var.set("置信度：隐藏")
                # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
                draw_probability_bars(bars, labels, np.zeros(len(labels)))
                # 把提示文字绘制到摄像头画面上。
                cv2.putText(frame, str(count), (42, 112), cv2.FONT_HERSHEY_SIMPLEX, 3.0, (80, 220, 255), 7)
            # 当前一个条件不成立时，继续判断 `state["phase"] == "capture"`。
            elif state["phase"] == "capture":
                # 采样阶段只收集图像，不提前把预测结果显示出来。
                # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
                round_rois.append(roi.copy())
                # 计算并保存 `state["last_roi"]`，供后续逻辑使用。
                state["last_roi"] = roi.copy()
                # 更新 Tk 变量，从而刷新界面文字。
                countdown_var.set("出拳！")
                # 更新 Tk 变量，从而刷新界面文字。
                gesture_emoji_var.set(GESTURE_EMOJI["waiting"])
                # 更新 Tk 变量，从而刷新界面文字。
                gesture_var.set("正在锁定")
                # 更新 Tk 变量，从而刷新界面文字。
                response_emoji_var.set(GESTURE_EMOJI["waiting"])
                # 更新 Tk 变量，从而刷新界面文字。
                response_var.set("正在锁定")
                # 更新 Tk 变量，从而刷新界面文字。
                reason_var.set("正在采集出拳后的稳定画面。")
                # 更新 Tk 变量，从而刷新界面文字。
                confidence_var.set("置信度：隐藏")
                # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
                draw_probability_bars(bars, labels, np.zeros(len(labels)))
                # 把提示文字绘制到摄像头画面上。
                cv2.putText(frame, "!", (42, 112), cv2.FONT_HERSHEY_SIMPLEX, 3.0, (80, 220, 255), 7)
                # 判断条件 `perf_counter() >= state["capture_deadline"]` 是否成立。
                if perf_counter() >= state["capture_deadline"]:
                    # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
                    lock_round(frame)
                    # 安排 Tk 在短暂延迟后继续刷新画面。
                    root.after(80, update_frame)
                    # 返回函数计算出的结果。
                    return
            # 处理前面条件都不满足时的情况。
            else:
                # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
                set_waiting_visuals()

            # 执行“持续读取摄像头并驱动 UI 状态更新”中的这一行操作。
            show_frame(frame)
        # 安排 Tk 在短暂延迟后继续刷新画面。
        root.after(20, update_frame)

    # 绑定窗口关闭事件。
    root.protocol("WM_DELETE_WINDOW", on_close)
    # 绑定键盘快捷键。
    root.bind("<space>", lambda _event: start_round())
    # 执行“当前流程”中的这一行操作。
    set_waiting_visuals()
    # 执行“当前流程”中的这一行操作。
    update_frame()
    # 进入 Tk 事件循环，保持窗口运行。
    root.mainloop()


# 定义 show_status 函数：打印数据集和模型的当前状态。
def show_status(args):
    # 输出当前项目状态：数据集数量、模型路径和是否存在模型。
    # 计算并保存 `data_dir`，供后续逻辑使用。
    data_dir = Path(args.data_dir)
    # 计算并保存 `output_dir`，供后续逻辑使用。
    output_dir = Path(args.output_dir)
    # 在控制台输出当前进度或状态信息。
    print("Project: self-trained rock-paper-scissors gesture classifier")
    # 在控制台输出当前进度或状态信息。
    print(f"Data dir: {data_dir}")
    # 在控制台输出当前进度或状态信息。
    print(f"Counts: {class_counts(data_dir)}")
    # 在控制台输出当前进度或状态信息。
    print(f"Output dir: {output_dir}")
    # 在控制台输出当前进度或状态信息。
    print(f"Model exists: {(output_dir / 'model.keras').exists()}")
    # 在控制台输出当前进度或状态信息。
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
    # 程序入口：根据 mode 分发到采集、训练、预测、UI 或状态查看功能。
    # 计算并保存 `args`，供后续逻辑使用。
    args = parse_args()
    # 判断条件 `args.mode == "collect"` 是否成立。
    if args.mode == "collect":
        # 执行“根据命令行模式调用对应功能”中的这一行操作。
        collect_data(args)
    # 当前一个条件不成立时，继续判断 `args.mode == "train"`。
    elif args.mode == "train":
        # 执行“根据命令行模式调用对应功能”中的这一行操作。
        train_model(args)
    # 当前一个条件不成立时，继续判断 `args.mode == "predict"`。
    elif args.mode == "predict":
        # 执行“根据命令行模式调用对应功能”中的这一行操作。
        predict_camera(args)
    # 当前一个条件不成立时，继续判断 `args.mode == "ui"`。
    elif args.mode == "ui":
        # 执行“根据命令行模式调用对应功能”中的这一行操作。
        run_ui(args)
    # 处理前面条件都不满足时的情况。
    else:
        # 执行“根据命令行模式调用对应功能”中的这一行操作。
        show_status(args)


# 判断当前文件是否作为主程序直接运行。
if __name__ == "__main__":
    # 执行“当前流程”中的这一行操作。
    main()
