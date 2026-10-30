# 可打印旋转倒立摆

当前使用 `设计输出_V3_单圈往返理线`。模型结构保持已确认的版本，适用 A1 mini 与 PETG。

- SolidWorks总装：`设计输出_V3_单圈往返理线/SolidWorks/V3_Furuta_DirectDrive_297.SLDASM`，同目录零件、子装配需全部保留。
- 打印文件：V3目录内 `STL打印`，共13种（含测试片）；同名STL与3MF二选一。
- 装配、螺丝数量和走线：`设计输出_V3_单圈往返理线/打印与装配说明_V3.md`。
- `3D`保存原始电机和传感器模型。
- 根目录三个ZIP是V1、V2、V3交付归档。旧版展开目录不纳入Git，需回看时可解压对应ZIP；不要混用版本。

本地CAD运行库、旧版展开目录、临时G-code、运行日志、空白装配备份和重复BMP不纳入Git。V3保留模型、打印文件、脚本、PNG/SVG预览以及检查JSON和切片结果；未修改机械设计。此次批量物理删除被工具审批拦截，因此这些文件仍留在本机，未实际释放对应磁盘空间。

## 重新生成

直接打开SolidWorks文件和切片STL/3MF，不需要安装Python依赖。只有重新运行建模脚本才需要恢复运行库：在本目录使用兼容的64位Python（原环境为3.12）执行：

```powershell
python -m pip install --target .cad_runtime -r requirements-cad.txt
python 设计输出_V3_单圈往返理线/scripts/build_v3.py
```

SolidWorks脚本要求本机安装SolidWorks与对应COM类型库；切片验证脚本要求Bambu Studio。脚本中的软件安装路径按本机实际位置调整。`.cad_runtime`不提交Git。运行打包脚本仅使用保留的切片成功记录；需要重新验证时先运行切片脚本。

导线为中位±180°往返余线方案，没有机械止挡；不能无限同方向旋转。现有数字验证不代替打印试装与真实软线收放检查。
