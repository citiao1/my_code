# 石头剪刀布手势识别项目答辩讲解

## 1. 项目一句话介绍

本项目使用 Python、OpenCV、TensorFlow 和 Tkinter 编写了一个“石头剪刀布手势识别比赛程序”。

程序可以通过摄像头采集我自己的手势图片，使用 TensorFlow 从零开始训练一个卷积神经网络模型，不使用任何预训练模型。训练完成后，程序通过图形界面进行猜拳比赛：倒计时结束后识别我的手势，并给出能够战胜我的电脑出拳结果。

## 2. 为什么这个项目算深度学习

这个项目算深度学习，原因如下：

1. 使用了 TensorFlow/Keras 深度学习框架。
2. 模型是卷积神经网络 CNN，包含多层 `Conv2D`、`MaxPooling2D`、`Dense` 等神经网络层。
3. 模型不是规则判断，也不是手写 if-else 分类，而是通过大量图片样本自动学习手势特征。
4. 模型没有使用预训练模型，权重从随机初始化开始训练。
5. 训练过程包含前向传播、损失计算、反向传播和参数更新。

可以这样向老师解释：

```text
我这个项目的核心不是用传统图像处理规则判断手势，而是先采集自己真实的手势数据，
再用 TensorFlow 搭建 CNN 卷积神经网络，让模型自己从图片中学习石头、剪刀、布三类特征。
训练完成后再把摄像头图像输入模型进行分类，所以它属于深度学习图像分类项目。
```

## 3. 项目文件说明

主要文件：

```text
tf_rps_gesture_classifier.py
```

这是整个项目的主程序，采集数据、训练模型、摄像头预测、图形界面都写在这一个 Python 文件中。

当前版本已经给主程序补充了逐行中文注释。阅读代码时可以直接从注释版主程序入手，不需要先猜每一行变量和函数的含义。

辅助备份文件：

```text
tf_rps_gesture_classifier_before_line_comments.py
tf_rps_gesture_classifier_before_dense_comments.py
```

这两个文件保留了加逐行注释前的较简洁版本，主要用于对比和备份。实际运行和答辩展示时使用 `tf_rps_gesture_classifier.py`。

数据目录：

```text
gesture_data/
  rock/
  paper/
  scissors/
```

这里保存自己采集的三类手势图片。

模型输出目录：

```text
outputs/rps_gesture/
  model.keras
  best_model.keras
  labels.txt
  run_report.json
  training_curve.png
  latest_ui_result.json
  latest_ui_roi.jpg
  latest_ui_model_input.jpg
```

其中：

- `model.keras`：训练好的 TensorFlow 模型。
- `labels.txt`：类别标签，分别是 rock、paper、scissors。
- `run_report.json`：训练结果报告。
- `training_curve.png`：训练准确率和损失曲线。
- `latest_ui_roi.jpg`：最近一次比赛从摄像头裁剪出来的原始识别区域。
- `latest_ui_model_input.jpg`：最近一次实际输入模型的图像，已经做了手部前景提取。

## 4. 当前训练结果

当前数据集数量：

```text
rock: 500 张
paper: 500 张
scissors: 500 张
```

训练集：

```text
rock: 400 张
paper: 400 张
scissors: 400 张
```

验证集：

```text
rock: 100 张
paper: 100 张
scissors: 100 张
```

当前验证集准确率：

```text
99.67%
```

混淆矩阵：

```text
真实\预测    rock    paper    scissors
rock          100       0          0
paper           0     100          0
scissors        0       1         99
```

解释方式：

```text
验证集中一共有 300 张图片，每类 100 张。
石头和布全部识别正确，剪刀有 1 张被识别成布，所以整体准确率约为 99.67%。
```

## 5. 如何运行程序

### 5.1 打开图形界面

最简单方式：

```text
双击 启动比赛界面.bat
```

或者在 VS Code 中直接运行：

```text
tf_rps_gesture_classifier.py
```

因为程序默认模式是 `ui`，所以直接运行会打开比赛界面。

命令行方式：

```powershell
cd "D:\my_code\my_code\其他大作业\软件实践"
D:\python_envs\software_practice_py311\Scripts\python.exe tf_rps_gesture_classifier.py ui
```

### 5.2 采集数据

采集三类手势，补到每类 500 张：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_rps_gesture_classifier.py collect --class-name all --target-count 500
```

单独重新采集剪刀：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_rps_gesture_classifier.py collect --class-name scissors --target-count 500 --replace-existing
```

### 5.3 训练模型

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_rps_gesture_classifier.py train
```

### 5.4 查看项目状态

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_rps_gesture_classifier.py status
```

## 6. 程序整体流程

整个程序可以分成五个阶段：

```text
摄像头采集数据
    ↓
图像预处理
    ↓
训练 CNN 模型
    ↓
打开 UI 比赛界面
    ↓
倒计时结束后识别手势并给出获胜手势
```

更详细一点：

1. 使用 OpenCV 打开摄像头。
2. 在画面中间画出绿色识别框。
3. 采集绿色框里的手势图像。
4. 将图片保存到 `gesture_data/rock`、`gesture_data/paper`、`gesture_data/scissors`。
5. 训练时读取图片，做手部前景提取和缩放。
6. 使用 CNN 模型学习三类手势特征。
7. UI 运行时再次读取摄像头画面。
8. 倒计时结束后采集短时间内的多帧图像。
9. 将多帧预测结果取平均，得到最终手势。
10. 根据石头剪刀布规则，电脑给出能赢我的手势。

## 7. 代码主要部分讲解

建议不要从第一行硬看到最后一行，而是先看程序入口，再顺着函数调用关系看。推荐阅读顺序：

```text
main()
    ↓
parse_args()
    ↓
collect_data() / train_model() / run_ui()
    ↓
图像预处理函数
    ↓
模型训练和预测细节
```

其中：

- `main()`：看清楚程序有哪些运行模式。
- `parse_args()`：理解 `args.samples`、`args.class_name`、`args.data_dir` 等参数从哪里来。
- `collect_data()`：理解如何采集数据。
- `train_model()`：理解如何训练 CNN 模型。
- `run_ui()`：理解比赛界面和倒计时识别流程。
- `hand_mask()`、`preprocess_hand_roi()`：最后再看，因为这部分涉及图像处理，难度稍高。

### 7.1 导入库和全局配置

代码开头导入了这些库：

```python
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

import matplotlib.pyplot as plt
import numpy as np
```

这些库分别用于：

- `argparse`：解析运行模式，比如 collect、train、ui。
- `json`：保存训练报告和 UI 调试结果。
- `Path`：处理文件路径。
- `deque`：保存最近几帧预测结果。
- `numpy`：处理图像数组和概率数组。
- `matplotlib`：保存训练曲线图。
- `tensorflow`：训练深度学习模型。
- `opencv-python`：摄像头采集和图像处理。
- `tkinter`：制作图形界面。

类别定义：

```python
CLASSES = ["rock", "paper", "scissors"]
WINNING_MOVE = {"rock": "paper", "paper": "scissors", "scissors": "rock"}
```

含义：

- 模型识别三类：石头、布、剪刀。
- 如果我出石头，电脑出布才能赢。
- 如果我出布，电脑出剪刀才能赢。
- 如果我出剪刀，电脑出石头才能赢。

### 7.2 摄像头采集数据

相关函数：

```text
collect_data()
collect_one_class()
center_square()
save_jpg()
```

核心逻辑：

1. 打开摄像头。
2. 水平翻转画面，让画面更像照镜子。
3. 在画面中心取一个正方形区域。
4. 用户把手放进绿色框。
5. 按空格开始或暂停采集。
6. 每隔几帧保存一张图片。
7. 图片按照类别保存到对应目录。

采集石头时保存到：

```text
gesture_data/rock/
```

采集布时保存到：

```text
gesture_data/paper/
```

采集剪刀时保存到：

```text
gesture_data/scissors/
```

答辩时可以这样说：

```text
为了满足老师要求的数据自主性，我没有使用网上下载好的数据集，
而是用摄像头自己采集石头、剪刀、布三种手势，每类 500 张。
```

### 7.3 手部前景提取

相关函数：

```text
hand_mask()
preprocess_hand_roi()
prepare_roi_for_model()
square_crop_with_margin()
```

这个部分是后来为了解决“换背景后识别率下降”加上的。

问题：

```text
最开始模型容易记住背景特征。
比如训练时背景固定，换一个背景后，模型可能把背景误认为某个手势特征。
```

解决方法：

1. 将图像从 BGR 转换到 YCrCb 色彩空间。
2. 用肤色阈值提取可能的手部区域。
3. 通过轮廓检测找到主要手部轮廓。
4. 去掉背景，只保留手部前景。
5. 将手部区域重新裁剪成正方形。
6. 缩放到 `128 x 128`，输入模型。

简化后的核心思想：

```python
mask = cv2.inRange(ycrcb, lower, upper)
contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
contour = max(contours, key=contour_score)
foreground[clean_mask > 0] = roi[clean_mask > 0]
```

这段代码实现了：

```text
从绿色框中找出最像手的区域，把背景尽量变成黑色，减少背景对模型判断的影响。
```

为什么不完全依赖传统图像处理？

```text
图像处理这里只负责减少背景干扰，真正判断石头、剪刀、布的仍然是 CNN 神经网络。
```

### 7.4 数据集划分

相关函数：

```text
split_image_paths()
make_dataset()
make_datasets()
decode_image()
```

功能：

1. 读取三类图片路径。
2. 每一类单独打乱。
3. 按比例划分训练集和验证集。
4. 默认 80% 用于训练，20% 用于验证。

为什么要每类单独划分？

```text
如果直接整体随机划分，可能会出现验证集类别不均衡。
我这里对 rock、paper、scissors 分别划分，保证验证集每类都有样本。
```

当前划分结果：

```text
每类 500 张
训练集每类 400 张
验证集每类 100 张
```

### 7.5 CNN 模型结构

相关函数：

```text
build_model()
```

模型结构：

```text
Input: 128 x 128 x 3 彩色图像
Rescaling: 像素归一化
RandomFlip: 随机水平翻转
RandomRotation: 随机旋转
RandomTranslation: 随机平移
RandomZoom: 随机缩放
RandomBrightness: 随机亮度变化
RandomContrast: 随机对比度变化
Conv2D + MaxPooling2D
Conv2D + MaxPooling2D
Conv2D + MaxPooling2D
Flatten
Dropout
Dense
Dense(3, softmax)
```

核心代码：

```python
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(image_size, image_size, 3)),
    tf.keras.layers.Rescaling(1.0 / 255),
    tf.keras.layers.RandomFlip("horizontal"),
    tf.keras.layers.RandomRotation(0.03),
    tf.keras.layers.RandomTranslation(0.04, 0.04),
    tf.keras.layers.RandomZoom(0.06),
    tf.keras.layers.RandomBrightness(0.16, value_range=(0, 1)),
    tf.keras.layers.RandomContrast(0.18),
    tf.keras.layers.Conv2D(32, 3, padding="same", activation="relu"),
    tf.keras.layers.MaxPooling2D(),
    tf.keras.layers.Conv2D(64, 3, padding="same", activation="relu"),
    tf.keras.layers.MaxPooling2D(),
    tf.keras.layers.Conv2D(128, 3, padding="same", activation="relu"),
    tf.keras.layers.MaxPooling2D(),
    tf.keras.layers.Flatten(),
    tf.keras.layers.Dropout(0.35),
    tf.keras.layers.Dense(128, activation="relu"),
    tf.keras.layers.Dense(len(CLASSES), activation="softmax"),
])
```

各层作用：

- `Rescaling`：把 0 到 255 的像素值缩放到 0 到 1。
- `RandomFlip`、`RandomRotation`、`RandomZoom`：数据增强，提高泛化能力。
- `Conv2D`：提取局部图像特征，比如手指边缘、掌心轮廓。
- `MaxPooling2D`：降低特征图尺寸，减少计算量，同时保留主要特征。
- `Flatten`：把二维特征图展开成一维向量。
- `Dropout`：防止过拟合。
- `Dense`：根据提取到的特征进行分类。
- `softmax`：输出三类手势的概率。

### 7.6 模型训练

相关函数：

```text
train_model()
compute_confusion_matrix()
plot_history()
save_keras_model()
```

训练过程：

1. 设置随机种子，保证结果尽量可复现。
2. 检查数据集数量。
3. 加载训练集和验证集。
4. 创建 CNN 模型。
5. 使用 Adam 优化器训练。
6. 使用交叉熵作为分类损失函数。
7. 训练结束后计算验证集准确率。
8. 保存模型、标签、训练曲线、训练报告。

核心训练代码：

```python
model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate),
    loss="categorical_crossentropy",
    metrics=["accuracy"],
)

history = model.fit(
    train_ds,
    validation_data=val_ds,
    epochs=args.epochs,
    callbacks=callbacks,
)
```

回调函数：

```python
tf.keras.callbacks.EarlyStopping(...)
tf.keras.callbacks.ReduceLROnPlateau(...)
```

作用：

- `EarlyStopping`：验证集效果长时间不提升时提前停止训练，避免过拟合。
- `ReduceLROnPlateau`：验证损失不下降时降低学习率，让训练更稳定。

### 7.7 摄像头预测

相关函数：

```text
load_predictor()
predict_roi_probs()
predict_rois_probs()
choose_label()
classify_roi()
```

预测流程：

1. 加载 `model.keras`。
2. 读取摄像头画面。
3. 截取绿色框中的 ROI。
4. 对 ROI 做手部前景提取。
5. 缩放成 `128 x 128`。
6. 转换成 RGB。
7. 输入模型得到三类概率。
8. 选择概率最大的类别作为预测结果。

示例：

```text
rock: 0.02
paper: 0.05
scissors: 0.93
```

说明模型认为当前手势最可能是剪刀。

### 7.8 UI 图形界面

相关函数：

```text
run_ui()
draw_probability_bars()
winning_response()
```

UI 使用的是 Tkinter。

界面功能：

1. 显示摄像头画面。
2. 显示绿色识别框。
3. 点击开始比赛。
4. 倒计时。
5. 倒计时结束后进入短暂采样阶段。
6. 锁定结果。
7. 显示我的手势。
8. 显示电脑能够获胜的手势。
9. 显示预测置信度和概率条。

为什么不是一直显示预测？

```text
为了更像真实比赛，我设计成倒计时结束后才识别。
比赛开始前不显示预测结果，倒计时结束后锁定一次结果，不会一直变化。
```

为什么采集多帧？

```text
单帧画面可能因为手刚动完、摄像头模糊、光照变化导致误判。
所以倒计时结束后短时间采集多帧，把多帧预测概率取平均，使结果更稳定。
```

### 7.9 猜拳获胜逻辑

相关代码：

```python
WINNING_MOVE = {
    "rock": "paper",
    "paper": "scissors",
    "scissors": "rock",
}
```

逻辑：

```text
识别到我出石头，电脑出布。
识别到我出布，电脑出剪刀。
识别到我出剪刀，电脑出石头。
```

这个功能不是深度学习部分，而是业务逻辑部分。

可以这样说：

```text
深度学习模型负责识别我出了什么手势；
程序规则负责根据石头剪刀布的胜负关系，给出电脑应该出的获胜手势。
```

## 8. 程序运行模式

`parse_args()` 定义了五种运行模式：

```text
collect    采集数据
train      训练模型
predict    摄像头实时预测
ui         打开图形比赛界面
status     查看项目状态
```

主函数：

```python
def main():
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
```

解释：

```text
程序启动后先解析参数，根据用户选择的模式调用不同函数。
如果不写模式，默认进入 ui 模式，所以可以直接点击运行打开界面。
```

## 9. 项目中遇到的问题和改进

### 问题 1：模型一开始容易识别错剪刀

原因：

```text
剪刀和布都属于手指张开的状态，特征比较接近。
如果剪刀样本角度不够丰富，模型容易混淆。
```

解决：

```text
重新采集剪刀数据，并保证角度、距离、光照更丰富。
```

### 问题 2：最后一秒才出手时识别不准

原因：

```text
手刚动完时画面容易模糊，单帧预测不稳定。
```

解决：

```text
倒计时结束后增加短暂采样时间，采集多帧图像取平均结果。
```

### 问题 3：换背景后全部识别成石头

原因：

```text
模型可能学习到了背景特征，而不是只学习手势特征。
```

解决：

```text
加入手部前景提取，把背景尽量去掉，只把手部区域送进模型。
```

### 问题 4：前景提取后又容易识别成布

原因：

```text
如果人脸或白衣服进入绿色框，前景提取可能把它们也当成手的一部分。
```

解决：

```text
改进轮廓筛选逻辑，只选择更像手的主轮廓，并重新训练模型。
```

## 10. 答辩演示步骤

建议演示顺序：

1. 打开项目目录。
2. 展示 `gesture_data`，说明数据是自己采集的。
3. 展示 `run_report.json`，说明数据量和准确率。
4. 打开 `training_curve.png`，说明训练过程。
5. 运行 UI。
6. 点击开始比赛。
7. 做一个手势。
8. 展示程序识别结果和电脑获胜手势。
9. 展示 `latest_ui_model_input.jpg`，说明程序会去掉背景再识别。

## 11. 答辩讲稿

可以直接照着下面讲：

```text
老师好，我做的是一个基于 TensorFlow 的石头剪刀布手势识别比赛程序。

这个项目的数据集是我自己通过摄像头采集的，不使用网上现成数据集，也不使用预训练模型。
一共采集了三类手势，分别是石头、布、剪刀，每类 500 张图片。

程序主要分为数据采集、图像预处理、模型训练、摄像头识别和 UI 比赛界面五个部分。

在数据采集阶段，我使用 OpenCV 打开摄像头，在画面中央设置一个绿色方框，
用户把手势放进方框后，程序会按类别保存图片。

在图像预处理阶段，为了减少背景变化对模型的影响，我加入了手部前景提取。
程序会在绿色框内寻找更像手的主要区域，去掉大部分背景，然后把图片缩放到 128 x 128。

模型部分使用 TensorFlow/Keras 搭建了一个 CNN 卷积神经网络。
网络包含三组卷积层和池化层，用来提取手指、掌心和轮廓等图像特征。
后面接 Flatten、Dropout 和全连接层，最后通过 softmax 输出三类手势的概率。

训练时我把每类图片按 8:2 划分为训练集和验证集。
当前每类有 400 张训练图片和 100 张验证图片，最终验证集准确率约为 99.67%。

在 UI 界面中，我设计了倒计时比赛流程。
点击开始后，倒计时结束才进行识别，比赛前不显示预测结果。
倒计时结束后程序会采集短时间内的多帧图像，对多帧预测概率取平均，
这样可以减少手刚移动时的模糊和抖动影响。

识别出我的手势后，程序根据石头剪刀布规则自动给出能赢我的手势。
比如识别到我出剪刀，电脑就出石头。

所以这个项目的核心是深度学习图像分类，UI 和猜拳逻辑是在模型识别结果基础上做的应用展示。
```

## 12. 老师可能会问的问题

### Q1：你有没有使用预训练模型？

回答：

```text
没有。我的模型是用 TensorFlow/Keras 自己搭建的 CNN，
权重从随机初始化开始训练，训练数据也是我自己用摄像头采集的。
```

### Q2：为什么这是深度学习，不是普通图像处理？

回答：

```text
普通图像处理只负责辅助，比如裁剪和去背景。
真正判断石头、剪刀、布的是 CNN 神经网络。
模型通过训练自动学习特征，而不是我手写规则判断手指数量。
```

### Q3：为什么要做手部前景提取？

回答：

```text
因为摄像头背景变化会影响模型。
如果训练时背景固定，模型可能误学背景特征。
前景提取可以减少背景干扰，让模型更关注手势本身。
```

### Q4：为什么要采集多帧再判断？

回答：

```text
比赛时手刚出完可能有运动模糊，单帧预测不稳定。
多帧取平均可以让预测更平滑，减少偶然误判。
```

### Q5：模型准确率是多少？

回答：

```text
当前验证集准确率约为 99.67%。
验证集每类 100 张，总共 300 张，只有 1 张剪刀被识别成布。
```

### Q6：程序有哪些不足？

回答：

```text
目前模型主要适应我的手、我的摄像头和当前光照。
如果换成其他人、特别暗的光线，或者手离镜头太远，仍然可能误判。
后续可以继续增加不同人、不同光照、不同背景的数据，提高泛化能力。
```

## 13. 每个主要函数做什么

| 函数名 | 作用 |
| --- | --- |
| `require_tf()` | 延迟导入 TensorFlow，只有训练或预测时才加载 |
| `import_cv2()` | 导入 OpenCV，如果没安装则提示 |
| `ensure_dirs()` | 创建 rock、paper、scissors 数据目录 |
| `class_counts()` | 统计每类图片数量 |
| `archive_class_images()` | 重新采集某类时备份旧图片 |
| `image_files()` | 获取目录下所有图片文件 |
| `center_square()` | 计算摄像头中央绿色识别框的位置 |
| `hand_mask()` | 根据肤色范围提取手部候选区域 |
| `preprocess_hand_roi()` | 去背景、裁剪手部、缩放图片 |
| `collect_one_class()` | 采集某一类手势图片 |
| `collect_data()` | 控制采集全部类别或某个类别 |
| `split_image_paths()` | 分层划分训练集和验证集 |
| `decode_image()` | 读取图片并转换成模型输入 |
| `make_dataset()` | 构建 TensorFlow 数据集 |
| `build_model()` | 搭建 CNN 神经网络 |
| `train_model()` | 训练模型并保存结果 |
| `compute_confusion_matrix()` | 计算混淆矩阵 |
| `plot_history()` | 保存训练曲线 |
| `load_predictor()` | 加载训练好的模型和标签 |
| `predict_roi_probs()` | 对单张 ROI 图片预测概率 |
| `predict_rois_probs()` | 对多帧 ROI 图片批量预测 |
| `choose_label()` | 根据概率选择最终类别 |
| `winning_response()` | 根据识别结果给出获胜手势 |
| `predict_camera()` | 摄像头实时预测模式 |
| `run_ui()` | 图形比赛界面 |
| `show_status()` | 显示项目状态 |
| `parse_args()` | 解析命令行参数 |
| `main()` | 程序入口 |

## 14. 可以重点展示的代码片段

### 14.1 模型代码

```python
def build_model(image_size: int, learning_rate: float):
    require_tf()
    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input(shape=(image_size, image_size, 3)),
            tf.keras.layers.Rescaling(1.0 / 255),
            tf.keras.layers.RandomFlip("horizontal"),
            tf.keras.layers.RandomRotation(0.03),
            tf.keras.layers.RandomTranslation(0.04, 0.04),
            tf.keras.layers.RandomZoom(0.06),
            tf.keras.layers.RandomBrightness(0.16, value_range=(0, 1)),
            tf.keras.layers.RandomContrast(0.18),
            tf.keras.layers.Conv2D(32, 3, padding="same", activation="relu"),
            tf.keras.layers.MaxPooling2D(),
            tf.keras.layers.Conv2D(64, 3, padding="same", activation="relu"),
            tf.keras.layers.MaxPooling2D(),
            tf.keras.layers.Conv2D(128, 3, padding="same", activation="relu"),
            tf.keras.layers.MaxPooling2D(),
            tf.keras.layers.Flatten(),
            tf.keras.layers.Dropout(0.35),
            tf.keras.layers.Dense(128, activation="relu"),
            tf.keras.layers.Dense(len(CLASSES), activation="softmax"),
        ]
    )
```

讲解重点：

```text
这是 CNN 模型结构，输入是 128x128 彩色图片，输出是三类手势概率。
```

### 14.2 训练代码

```python
history = model.fit(
    train_ds,
    validation_data=val_ds,
    epochs=args.epochs,
    callbacks=callbacks,
)
```

讲解重点：

```text
这里调用 TensorFlow 的 fit 方法开始训练模型，
每轮训练后会在验证集上测试准确率。
```

### 14.3 预测代码

```python
img = prepare_roi_for_model(roi, cv2, args)
rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
probs = model.predict(np.expand_dims(rgb.astype("float32"), axis=0), verbose=0)[0]
```

讲解重点：

```text
先把摄像头中的绿色框图像预处理成模型输入格式，
然后调用模型输出三类概率。
```

### 14.4 比赛规则代码

```python
WINNING_MOVE = {"rock": "paper", "paper": "scissors", "scissors": "rock"}
```

讲解重点：

```text
模型负责识别我的手势，这个字典负责决定电脑出什么可以赢。
```

## 15. 答辩时不要说错的点

不要说：

```text
我用了现成模型。
我下载了别人做好的数据集。
我用 if 判断手指数量。
```

应该说：

```text
数据是自己采集的。
模型是自己用 TensorFlow 搭建并从零训练的。
图像处理只做辅助预处理，最终分类由 CNN 完成。
```

## 16. 简短版总结

如果老师只让你用一分钟介绍，可以这样说：

```text
我做的是一个基于 TensorFlow 的石头剪刀布手势识别程序。
我先用 OpenCV 摄像头自己采集三类手势数据，每类 500 张，
然后用 TensorFlow 搭建 CNN 模型从零训练，不使用预训练模型。
为了减少背景干扰，我加入了手部前景提取，把绿色框里的手部区域裁剪出来再输入模型。
训练后验证集准确率约为 99.67%。
最后我用 Tkinter 做了一个比赛界面，倒计时结束后识别我的手势，
并根据石头剪刀布规则给出电脑能获胜的手势。
```
