# 图片/动图中线对称工具

这个小工具可以把 PNG、JPG、JPEG、JFIF、WebP、BMP、TIFF、GIF 等图片从中线做镜像对称。GIF 会逐帧处理并保留动图。

## 图形界面

如果你拿到的是 `ImageSymmetryTool.exe`，直接双击它就能用，不需要安装 Python。

双击 `start_gui.bat`，选择图片或 GIF，设置方向后点击“开始处理”。

默认效果是保留左半边，并把左半边镜像到右半边：

```powershell
python symmetry_tool.py input.png
```

## 命令行

保留左半边，生成左右对称图：

```powershell
python symmetry_tool.py input.png
```

保留右半边：

```powershell
python symmetry_tool.py input.png --source right
```

处理 GIF：

```powershell
python symmetry_tool.py input.gif
```

指定输出文件：

```powershell
python symmetry_tool.py input.png -o output.png
```

批量处理文件夹，输出到 `out` 文件夹：

```powershell
python symmetry_tool.py . -o out
```

上下对称也支持：

```powershell
python symmetry_tool.py input.png --axis horizontal --source top
```
