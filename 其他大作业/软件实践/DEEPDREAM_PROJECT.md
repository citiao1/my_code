# TensorFlow DeepDream 梦境相机项目说明

生成时间：2026-07-06

本项目是《应用软件实践》课程可展示的深度学习小程序，核心文件只有一个：

```text
tf_deepdream_camera.py
```

程序使用 TensorFlow/Keras 的预训练 InceptionV3 卷积神经网络，通过反向传播和梯度上升优化输入图片本身，生成 DeepDream 风格的“梦境纹理”图像。它不是普通分类程序，而是一个深度学习特征可视化与生成式图像处理示例。

## 项目定位

老师要求运行 200 行以上的深度学习或更高难度程序。本项目主程序约 400 行以上，满足代码量要求，并且能用一个命令完整运行。

项目特点：

- 使用 TensorFlow/Keras，贴合老师课件里的 TensorFlow 路线。
- 使用预训练深度卷积网络 InceptionV3。
- 使用 `tf.GradientTape` 做反向传播。
- 对输入图片做梯度上升，生成视觉效果明显的图像。
- 支持 `soft`、`classic`、`wild` 三种梦境风格。
- 支持 octave 多尺度优化，先小图后大图，增强不同尺度的纹理。
- 默认使用本目录教材图片 `教材行文代码/birdnest.jpg`。
- Keras 缓存默认放到 `D:\keras-cache`，避免大模型权重放进 C 盘。

## 目录构成

```text
软件实践/
├─ tf_deepdream_camera.py
├─ DEEPDREAM_PROJECT.md
├─ 教材行文代码/
│  ├─ birdnest.jpg
│  ├─ astro.jpg
│  └─ fcity.jpg
└─ outputs/
   └─ deepdream_camera_quick_fixed/
      ├─ original.png
      ├─ dream_soft.png
      ├─ dream_classic.png
      ├─ dream_wild.png
      ├─ octave_progress_soft.png
      ├─ octave_progress_classic.png
      ├─ octave_progress_wild.png
      ├─ comparison_grid.png
      └─ run_report.txt
```

`outputs/deepdream_camera_quick_fixed/` 是已经验证过的快速运行结果。正式展示时可以重新运行脚本生成更高清的结果。

## 运行环境

当前电脑使用 D 盘虚拟环境：

```text
D:\python_envs\software_practice_py311\Scripts\python.exe
```

主要依赖：

- `tensorflow 2.21.0`
- `numpy`
- `Pillow`

如果另一台电脑还没有装依赖，可以在对应虚拟环境中执行：

```powershell
python -m pip install tensorflow numpy pillow
```

如果希望和本机一致，建议仍然把虚拟环境放在 D 盘，例如：

```powershell
python -m venv D:\python_envs\software_practice_py311
D:\python_envs\software_practice_py311\Scripts\python.exe -m pip install --upgrade pip
D:\python_envs\software_practice_py311\Scripts\python.exe -m pip install tensorflow numpy pillow
```

## 推荐运行命令

快速验证：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_deepdream_camera.py --iterations 2 --octaves 1 --max-size 256 --output-dir outputs/deepdream_camera_quick_fixed
```

课堂展示推荐：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_deepdream_camera.py --preset all --iterations 20 --octaves 3 --max-size 768
```

如果电脑较慢，可以运行中等参数：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_deepdream_camera.py --preset classic --iterations 15 --octaves 3 --max-size 512
```

自定义图片：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_deepdream_camera.py --image 教材行文代码\astro.jpg --preset wild --iterations 20 --octaves 3 --max-size 768
```

## 参数说明

```text
--image         输入图片路径，默认是 教材行文代码/birdnest.jpg
--preset        梦境风格，可选 all/soft/classic/wild，默认 all
--iterations    每个尺度的梯度上升次数，越大效果越明显也越慢
--octaves       多尺度层数，越大越能保留不同尺度纹理
--octave-scale  尺度放大倍数，默认 1.4
--step          梯度上升步长，默认 0.01
--max-size      工作图片最长边，越大越清晰也越慢
--output-dir    输出目录
--seed          随机种子
```

## 程序结构

`tf_deepdream_camera.py` 的主要模块：

- 环境设置：设置 `KERAS_HOME=D:\keras-cache`，避免模型缓存进入 C 盘。
- 参数解析：用 `argparse` 提供命令行参数。
- 图片处理：读取图片、等比例缩放、InceptionV3 预处理、反预处理。
- 模型加载：加载 `InceptionV3(weights="imagenet", include_top=False)`。
- 风格预设：`soft`、`classic`、`wild` 分别使用不同中间卷积层。
- 损失函数：最大化指定中间层的激活均值。
- 梯度上升：使用 `tf.GradientTape` 计算输入图片对损失的梯度。
- 多尺度处理：从小图开始逐步放大，每个尺度都进行梯度上升。
- 结果输出：保存原图、三种梦境图、octave 过程图、对比图和运行报告。

## 答辩讲解要点

可以按下面顺序讲：

1. 输入是一张普通图片，例如鸟巢夜景。
2. 程序加载已经在 ImageNet 上训练好的 InceptionV3。
3. 不再训练模型参数，而是冻结模型。
4. 选取 CNN 的中间层作为“想要强化的视觉特征”。
5. 用反向传播计算图片像素对这些特征的影响。
6. 用梯度上升修改图片，让这些特征越来越明显。
7. 不同层组合对应不同风格，所以有 `soft`、`classic`、`wild` 三种结果。
8. octave 多尺度让纹理同时出现在大结构和小细节上。

一句话总结：

> DeepDream 是把深度神经网络学到的特征反过来画到图片上，用反向传播优化图片本身，从而可视化 CNN 的内部视觉模式。

## 已验证结果

已经在当前电脑上完成快速测试：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe tf_deepdream_camera.py --iterations 2 --octaves 1 --max-size 256 --output-dir outputs/deepdream_camera_quick_fixed
```

验证结果：

- 脚本可以正常运行。
- TensorFlow 版本：`2.21.0`。
- InceptionV3 权重已下载到 `D:\keras-cache`。
- 已生成 `dream_soft.png`、`dream_classic.png`、`dream_wild.png`。
- 已生成 `comparison_grid.png` 和 `run_report.txt`。

注意：TensorFlow 2.11 以后在原生 Windows 上通常不直接使用 CUDA GPU。当前项目即使使用 CPU 也能运行，显卡不是必须条件。
