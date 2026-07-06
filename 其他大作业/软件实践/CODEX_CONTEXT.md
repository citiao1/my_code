# 应用软件实践文件夹说明

生成时间：2026-07-06

本目录是《应用软件实践》课程资料集合，位置通常为：

```text
D:\my_code\其他大作业\软件实践
```

## 给 Codex 的快速结论

这门课不是单纯理论课，而是 4 天左右的 Python 上机实践课。老师通知里提到的主题是：Python 的简单使用、人工智能初步程序编写与调试、医学影像处理技术初步。结合本目录资料判断，课程主线是先学 Python 基础和程序设计方法，再跑教材示例，最后可能基于图像处理或 TensorFlow 做一个可展示的小程序，并在答辩时现场讲解程序。

答辩准备时应重点说明：

- 程序要解决什么问题。
- 输入、处理、输出分别是什么。
- 主要用了哪些 Python 库。
- 关键函数或算法逻辑。
- 如何运行，运行结果是什么。
- 调试中遇到的问题和解决方法。

## 目录结构

```text
软件实践/
├─ CODEX_CONTEXT.md
├─ 应用软件实践/
│  ├─ 第一部分-T0-说明.ppt
│  ├─ 第二部分-T1-程序设计基本方法.ppt
│  ├─ 第三部分-T2-Python程序实例解析.ppt
│  ├─ 第四部分-tensorflow安装.pptx
│  └─ 第五部分-库安装与tf导入.pptx
├─ 环境/
│  ├─ pycharm使用(1).pptx
│  ├─ PyTorch-6.29(1).pptx
│  ├─ 第四部分-tensorflow安装.pptx
│  └─ 第五部分-库安装与tf导入.pptx
├─ python PPT/
│  ├─ T0-说明.ppt
│  ├─ T1-程序设计基本方法.ppt
│  ├─ T2-Python程序实例解析.ppt
│  ├─ T3-基本数据类型.ppt
│  ├─ T4-程序的控制结构.ppt
│  ├─ T5-函数和代码复用.ppt
│  ├─ T6-组合数据类型.ppt
│  ├─ T7-文件和数据格式化.ppt
│  ├─ T8-程序设计方法论.ppt
│  ├─ T9-科学计算和可视化.ppt
│  └─ TA-网络爬虫和自动化.ppt
└─ 教材行文代码/
   ├─ P019...P277 系列 Python 示例程序
   ├─ astro.jpg, birdnest.jpg, fcity.jpg
   ├─ hamlet.txt, 三国演义.txt
   └─ price2016.* CSV/JSON/HTML 示例数据
```

## 课件内容

### 应用软件实践

这是本课最核心的专用课件。

- `第一部分-T0-说明.ppt`：课程资料说明和共享协议。
- `第二部分-T1-程序设计基本方法.ppt`：计算机、程序设计语言、Python 介绍、环境配置、IPO 程序编写方法。
- `第三部分-T2-Python程序实例解析.ppt`：温度转换程序、Python 语法元素、缩进、注释、变量、字符串、input、if/elif/else、eval、print、循环、turtle 绘图、函数封装。
- `第四部分-tensorflow安装.pptx`：Anaconda、虚拟环境、TensorFlow 2.0 CPU/GPU 安装、CUDA/cuDNN 说明。
- `第五部分-库安装与tf导入.pptx`：pip 安装库、在 Spyder 中导入 TensorFlow、解释器路径和 spyder-kernels。

### python PPT

这是更完整的 Python 课程课件，短学期可能只选讲部分。

- `T1` 到 `T2`：Python 入门、程序设计方法、温度转换、turtle 绘图。
- `T3` 到 `T6`：基本数据类型、控制结构、函数、列表、字典、集合、jieba 词频统计。
- `T7`：文件读写、PIL 图像处理、CSV、JSON。
- `T8`：自顶向下设计、测试、pyinstaller、pip 和第三方库。
- `T9`：numpy、图像数组处理、matplotlib、科学计算可视化、雷达图。
- `TA`：requests、BeautifulSoup、网络爬虫和自动化。

### 环境

这是 2026-07-06 新增的环境配置课件目录，主要讲开发工具和深度学习环境。

- `pycharm使用(1).pptx`：PyCharm Community/Professional 区别、PyCharm 下载和安装、Python 解释器安装、创建工程、配置解释器、运行 `test.py`、在 PyCharm 中安装第三方库。
- `PyTorch-6.29(1).pptx`：PyTorch 核心概念、安装流程、张量运算、自动微分、GPU 加速、模型结构、训练流程和评估方法。
- `第四部分-tensorflow安装.pptx`：Anaconda3、TensorFlow CPU/GPU 安装、NVIDIA 驱动、CUDA/cuDNN 和环境变量。
- `第五部分-库安装与tf导入.pptx`：pip 安装库、Spyder 解释器路径配置、`spyder_kernels` 和 TensorFlow 导入验证。

## 示例程序分组

`教材行文代码` 是教材配套代码库，可以按能力线索快速定位：

- Python 入门：圆面积、姓名回显、斐波那契、日期时间、温度转换。
- 基础语法：天天向上、月份缩写、凯撒密码、文本进度条、PM2.5 判断、BMI。
- 函数和递归：生日歌、七段数码管、阶乘、字符串反转、科赫曲线。
- 统计和文本：基本统计值、Hamlet 词频、《三国演义》人物统计，使用 `jieba`。
- 文件和数据格式：读写文本、CSV、JSON、HTML 表格转换。
- 图像处理：`P189` 到 `P195` 使用 Pillow 做 GIF 提取、RGB 通道调整、轮廓提取、对比度增强、字符画。
- 图像数组处理：`P245-e17.1HandDrawPic.py` 使用 Pillow 和 numpy 生成手绘风格图像。
- 科学计算和可视化：`P250` 到 `P258` 使用 numpy 和 matplotlib 画坐标图、阻尼曲线、雷达图。
- 爬虫和自动化：`P273`、`P277` 使用 requests 和 BeautifulSoup。

## 本机环境状态

不要把课程重型库直接装进系统 Python 的 C 盘 `site-packages`。本机现在采用 D 盘虚拟环境：

```text
D:\python_envs\software_practice_py311
```

已验证状态：

- 系统 Python 3.11.9 可用，位置是 `C:\Users\123\AppData\Local\Programs\Python\Python311\python.exe`，只作为创建虚拟环境的基础解释器。
- 课程虚拟环境 Python 可用，位置是 `D:\python_envs\software_practice_py311\Scripts\python.exe`。
- PyCharm Community Edition 2025.2.6.1 已安装到 D 盘：`D:\Applications\JetBrains\PyCharm Community`。
- Git 可用。
- CodeGraph CLI 可用。
- NVIDIA RTX 4060 和 `nvidia-smi` 可用；未检测到 `nvcc`。
- 未检测到 `conda` 命令；当前用 `venv + pip` 管理课程环境。如果老师强制要求 Anaconda/Spyder，再另装 Anaconda 或 Miniconda，安装路径也应放 D 盘。

在 D 盘虚拟环境中已安装并 import 验证通过的课程常用库：

- `numpy`
- `Pillow`，导入名为 `PIL`
- `matplotlib`
- `jieba`
- `requests`
- `beautifulsoup4`，导入名为 `bs4`
- `scikit-learn`，导入名为 `sklearn`
- `tensorflow`
- `torch`
- `torchvision`
- `torchaudio`
- `opencv-python`，导入名为 `cv2`
- `pydicom`
- `SimpleITK`

当前关键版本：

- TensorFlow：`2.21.0`
- PyTorch：`torch 2.11.0+cpu`
- torchvision：`0.26.0+cpu`
- torchaudio：`2.11.0+cpu`

注意：这台 Windows 机器虽然有 NVIDIA GPU，但 TensorFlow 2.21 在 Windows 原生环境下不会使用 GPU。PyTorch 当前安装 CPU 版，足够完成课件里的张量、模型结构、调试和基础训练练习。曾尝试安装 PyTorch CUDA 12.8 版，但官方 `torch-2.11.0+cu128` wheel 约 2.7GB，下载超时，未完成安装。

验证命令：

```powershell
@'
import numpy, PIL, matplotlib, jieba, requests, bs4, sklearn, tensorflow, cv2, pydicom, SimpleITK
import torch, torchvision, torchaudio
print("tensorflow:", tensorflow.__version__)
print("torch:", torch.__version__)
print("torchvision:", torchvision.__version__)
print("torchaudio:", torchaudio.__version__)
print("torch cuda available:", torch.cuda.is_available())
print("course env ok")
'@ | D:\python_envs\software_practice_py311\Scripts\python.exe -
```

## 建议答辩项目方向

最贴合现有资料和老师通知的方向是做一个“医学影像处理初步”的小型图像处理程序。即使没有真实 DICOM 数据，也可以先用普通图片模拟流程：

1. 读取图片。
2. 转灰度图。
3. 增强对比度。
4. 做轮廓或边缘提取。
5. 显示处理前后对比图。
6. 可选：用 numpy 计算灰度直方图或阈值分割。

可参考的代码：

- `教材行文代码/P191-m7.5ChangeRGB.py`
- `教材行文代码/P192-m7.6GetImageContour.py`
- `教材行文代码/P193-m7.7EnImageContrast.py`
- `教材行文代码/P195-e12.1DrawCharImage.py`
- `教材行文代码/P245-e17.1HandDrawPic.py`
- `教材行文代码/P250-m9.1PlotTriangle.py`
- `教材行文代码/P254-e18.1PlotDamping.py`

## 给另一台电脑的 Codex 操作建议

如果另一台电脑已经拉取了 `D:\my_code` 仓库：

```powershell
cd D:\my_code
codegraph sync D:\my_code
codegraph node -p D:\my_code -f "其他大作业/软件实践/CODEX_CONTEXT.md" --limit 220
```

如果要补环境，优先在 D 盘创建虚拟环境，不要直接装进 C 盘系统 Python：

```powershell
New-Item -ItemType Directory -Force -Path D:\python_envs | Out-Null
python -m venv D:\python_envs\software_practice_py311
D:\python_envs\software_practice_py311\Scripts\python.exe -m pip install --upgrade pip

New-Item -ItemType Directory -Force -Path D:\pip-tmp | Out-Null
$env:TEMP = "D:\pip-tmp"
$env:TMP = "D:\pip-tmp"
D:\python_envs\software_practice_py311\Scripts\python.exe -m pip install --no-cache-dir numpy matplotlib jieba scikit-learn opencv-python pydicom SimpleITK tensorflow pillow requests beautifulsoup4 torch==2.11.0 torchvision==0.26.0 torchaudio==2.11.0
Remove-Item -LiteralPath D:\pip-tmp -Recurse -Force -ErrorAction SilentlyContinue
```

如果要安装 PyCharm Community，也指定 D 盘路径：

```powershell
winget install --id JetBrains.PyCharm.Community -e --location "D:\Applications\JetBrains\PyCharm Community" --accept-package-agreements --accept-source-agreements
```

环境检查命令：

```powershell
Get-Command python,pip,conda,pycharm,winget,nvidia-smi,nvcc -ErrorAction SilentlyContinue

@'
import tensorflow as tf
import torch, torchvision, torchaudio
print("tensorflow:", tf.__version__)
print("tensorflow gpus:", tf.config.list_physical_devices("GPU"))
print("torch:", torch.__version__)
print("torchvision:", torchvision.__version__)
print("torchaudio:", torchaudio.__version__)
print("torch cuda available:", torch.cuda.is_available())
print("torch cuda version:", torch.version.cuda)
'@ | D:\python_envs\software_practice_py311\Scripts\python.exe -
```

如果要快速理解课程，优先读：

1. `CODEX_CONTEXT.md`
2. `应用软件实践/第二部分-T1-程序设计基本方法.ppt`
3. `应用软件实践/第三部分-T2-Python程序实例解析.ppt`
4. `环境/pycharm使用(1).pptx`
5. `环境/PyTorch-6.29(1).pptx`
6. `应用软件实践/第四部分-tensorflow安装.pptx`
7. `教材行文代码/P245-e17.1HandDrawPic.py`
