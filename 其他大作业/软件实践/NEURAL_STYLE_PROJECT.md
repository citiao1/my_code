# TensorFlow 神经风格迁移项目说明

生成时间：2026-07-07

本项目用于替代原来的 DeepDream 试验方向，核心文件是：

```text
tf_neural_style_transfer.py
```

程序把一张“内容图片”和一张“风格图片”合成一张新图片：新图片保留内容图片的主体结构，同时吸收风格图片的色彩、纹理和笔触。相比 DeepDream，这个项目更容易解释，也更容易看出“输入、处理、输出”的完整项目感。

## 当前结论

推荐把 `hub` 模式作为课堂展示主线：

- 使用 TensorFlow Hub 的预训练任意风格迁移模型。
- 运行时不训练模型，只做推理，速度快，效果稳定。
- 已在本机跑通，512 像素输出约 21 秒。
- 输出图主体清楚，风格纹理明显，展示效果明显优于手写优化版。

脚本中仍保留 `optimize` 模式：

- 使用 VGG19 提取内容特征和风格特征。
- 用 Gram matrix 表示风格纹理。
- 用梯度下降优化输出图片。
- 适合答辩时讲神经风格迁移原理，但当前实际效果不如 Hub 模型稳定。

## 运行环境

本机已验证过的虚拟环境：

```powershell
D:\python_envs\software_practice_py311\Scripts\python.exe
```

换到新电脑时不需要照抄这个路径。进入项目目录后，使用新电脑自己的 Python 或已激活的虚拟环境即可：

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install tensorflow tensorflow-hub numpy pillow
```

已验证依赖：

```text
tensorflow 2.21.0
keras 3.15.0
tensorflow-hub 0.16.1
numpy
Pillow
```

如果另一台电脑缺依赖：

```powershell
python -m pip install tensorflow tensorflow-hub numpy pillow
```

注意：当前原生 Windows 环境下 TensorFlow 2.21.0 没有使用 NVIDIA GPU。4060 要用于 TensorFlow GPU，建议后续走 WSL2/Linux 环境。

## 另一台电脑同步和运行

在另一台电脑上同步时，推荐按下面顺序操作：

```powershell
git clone https://github.com/citiao1/my_code.git
cd "my_code\其他大作业\软件实践"
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install tensorflow tensorflow-hub numpy pillow
```

如果仓库已经 clone 过，只需要进入仓库后拉取最新内容：

```powershell
git pull
cd "其他大作业\软件实践"
```

本项目已经把常用演示输出同步到仓库中，例如：

```text
outputs/neural_style_transfer_vscode/
outputs/neural_style_transfer_hub_fcity_astro/
outputs/neural_style_transfer_default_python_smoke/
```

因此另一台电脑即使暂时不重新运行程序，也可以直接打开这些目录里的 `comparison_grid.png` 查看效果。

需要注意的是，TensorFlow Hub 下载的模型缓存不会提交到仓库。第一次在新电脑运行 `hub` 模式时，程序会自动下载模型，可能需要等待一会儿；下载完成后后续运行会快很多。脚本会优先使用项目下的 `.model-cache`，如果当前路径包含中文导致缓存路径不合适，会自动退到用户目录或系统临时目录。

同步后建议先运行一次轻量验证：

```powershell
.\.venv\Scripts\python.exe tf_neural_style_transfer.py --method hub --demo-content --max-size 256 --output-dir outputs/neural_style_transfer_sync_test
```

如果这个命令能生成 `comparison_grid.png`，就说明新电脑的环境和依赖已经可用。

## 推荐展示命令

使用 TensorFlow 官方示例照片和 Kandinsky 风格图，效果最好：

```powershell
python tf_neural_style_transfer.py --method hub --demo-content --max-size 512 --output-dir outputs/neural_style_transfer_hub_demo
```

使用已经下载好的本地 demo 内容图：

```powershell
python tf_neural_style_transfer.py --method hub --content-image outputs/neural_style_transfer_demo_labrador/content_demo_labrador.jpg --max-size 512 --output-dir outputs/neural_style_transfer_hub_final
```

使用本课程素材鸟巢图也能跑，但夜景太暗，展示效果一般：

```powershell
python tf_neural_style_transfer.py --method hub --content-image 教材行文代码/birdnest.jpg --max-size 512 --output-dir outputs/neural_style_transfer_birdnest_hub
```

讲原理时可运行手写优化版：

```powershell
python tf_neural_style_transfer.py --method optimize --demo-content --iterations 120 --max-size 384 --preset balanced --output-dir outputs/neural_style_transfer_optimize_demo
```

## 已验证输出

主推荐结果：

```text
outputs/neural_style_transfer_hub_final/
├─ content.png
├─ style_reference.png
├─ stylized_result.png
├─ comparison_grid.png
└─ run_report.txt
```

其中 `comparison_grid.png` 是答辩最适合展示的一张图，依次包含内容图、风格图和生成结果。

## 答辩讲解要点

可以按下面顺序讲：

1. 输入两张图片：一张提供内容，一张提供艺术风格。
2. 神经网络把内容图的主体结构和风格图的纹理信息提取出来。
3. `hub` 模式直接使用已经训练好的风格迁移网络，所以运行很快。
4. 输出图片既保留原图主体，又带有风格图的笔触和颜色。
5. `optimize` 模式展示了底层原理：内容损失负责保留结构，风格损失负责匹配纹理，总变差损失负责让图像更平滑。

一句话总结：

> 神经风格迁移是用深度神经网络把一张图片的内容和另一张图片的艺术风格组合起来，生成一张新的风格化图片。

## 是否值得继续

值得保留，但建议主线使用 TensorFlow Hub 预训练模型，而不是纯手写优化版。

如果老师特别强调“必须训练模型”，这个项目就不如 Conditional GAN；但如果要求是“深度学习或更高难度程序，能完整运行并展示效果”，这个风格迁移项目比 DeepDream 更稳。
