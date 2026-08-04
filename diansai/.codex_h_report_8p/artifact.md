# 2026 H题报告模板执行约束

## Reference

- 权威模板：`D:\my_code\my_code\diansai\.codex_h_report_8p\template.docx`
- SHA-256：`A5F3D12F406534731CD67480569F84A29EDF36E269486DA5F5DF2634A2EE35B9`
- LibreOffice 渲染页数：16 页；证据目录：`D:\my_code\my_code\diansai\.codex_h_report_8p\template_reference_render_2`
- 结构证据：`D:\my_code\my_code\diansai\.codex_h_report_8p\template-style-evidence.json`，以及本任务执行的 section、heading、image、field、footnote、content-control 审计输出。
- 模板结构：153 个正文段落、17 个表格、3 个分节、1 个内嵌图片、22 个 PAGEREF 字段、1 个 PAGE 字段，无内容控件。

## Page system

- 所有分节均为 A4 纵向，8.27 in x 11.69 in。
- 三个分节页边距相同：左 1.25 in，右 1.25 in，上 1.30 in，下 1.00 in。
- 分节 1 为封面，分节 2 为摘要/目录，分节 3 为正文；均以新页开始。
- 三个分节的页眉、页脚均不链接前一节；无奇偶页不同设置。
- 第 3 分节页脚包含 PAGE 字段并作为正文页码来源。不得重建或替换页脚。

## Typography and paragraph roles

- 封面校名：段落 0，居中，宋体/Times New Roman 24 pt，段前 22 pt、段后 44 pt。
- 封面题名：段落 1，居中，宋体/Times New Roman 18 pt、粗体、红色，段后 122 pt。
- 封面模板名：段落 3，居中，宋体/Times New Roman 14 pt、粗体；封面日期为段落 4，16 pt、粗体。
- 摘要/关键词：11/12 pt 模板正文节奏，1.1 倍行距；标签使用粗体，内容使用常规字重。
- 目录标题：20 pt、居中；目录条目复用模板的 `toc 1`、`toc 2` 样式及右对齐页码制表位。
- 正文题名：18 pt、居中；组别：18 pt、居中。
- 一级标题：复用 `Heading 1`，14 pt、粗体、与下段同页。
- 二级标题：复用 `Heading 2`，12 pt、粗体、与下段同页。
- 正文：模板 Normal 11/12 pt，中文宋体、英文与数字 Times New Roman，两端对齐，首行缩进按模板现有正文角色设置。
- 图题居中、约 10.5 pt；表题左对齐、9 pt 粗体；图题/表题与其对象保持同页。
- 不引入第二套颜色、页眉、页脚、封面、标题或列表体系。

## Lists, tables, figures and fields

- 保留模板表格的固定宽度、居中方式、边框、单元格边距和列宽逻辑；不使用自动调整列宽。
- 本稿只保留评分所需的紧凑表格：系统方案、硬件接口、软件时序、测试记录和指标汇总。长段解释使用正文，不塞入表格。
- 所有新图复用模板图示占位位置及图题样式，内嵌、不浮动，不使用文本框覆盖正文。
- 目录使用模板 TOC 条目样式。若 Word 字段未刷新，设置 `w:updateFields=true`，并保持可在 Word/LibreOffice 中编辑更新。

## Content flow and slot map

- `word/document.xml` 段落 0-4：官方封面，完整保留，文本、运行属性、段落属性与分节边界不得改变。
- 分节 2：改写为摘要、关键词和紧凑目录；删除模板使用提示，不改变分节结构。
- 分节 3：用模板的标题、一级标题、二级标题、正文、图题、表题和表格组件重建报告正文。
- 正文顺序：系统方案；理论分析与控制；电路与程序；视觉识别与连续追踪预留；测试方案与待测记录；结论；参考文献。
- “视觉识别与连续追踪”预留约 0.5-1 页，仅允许补写图像采集、ROI、钢球检测、连续追踪、遮挡/丢失重捕获和追踪结果输出接口。小球位置/速度 PID 不得写入该预留段。
- 球位置/速度 PID、目标轨迹和步进电机角度指令放在“滚球控制与执行机构”小节，与视觉追踪章节分离。
- 已确认硬件：MSPM0G3507、AT8236、两台 MG513、LSM6DSV16XTR、8 路自校准红外模块、MaixCAM Pro、自闭环 42 步进电机与摇杆机构、独立 K230 图传模块。
- 未实测的尺寸、参数、性能和表 B 数据统一标为“待实测/待填写”，不得虚构。

## Package preservation

- 可编辑：`word/document.xml`；因新增/删除内嵌图可能按需编辑 `word/_rels/document.xml.rels` 与 `[Content_Types].xml`。
- 保留：`word/styles.xml`、`word/fontTable.xml`、`word/theme/theme1.xml`、`word/numbering.xml`（若存在）、`word/settings.xml` 除 `w:updateFields`、全部 header/footer、footnotes/endnotes、customXml、现有 `word/media/image1.png` 及其关系。
- 基线媒体 `word/media/image1.png` SHA-256：`9E35CF6B7719C72DEF154407F17429ACFC5C19346B585705D927D4D31D2E9A82`。

## Fidelity gates

- 模板源文件交付前仍须匹配记录的 SHA-256。
- 最终文档前 5 个段落的 XML、文本和段落属性必须与模板逐字节相同；封面渲染与模板封面应无像素差异。
- 三分节数量、页面几何、页脚页码、模板样式、现有模板媒体不得丢失或改写。
- 禁止把 K230 写成视觉识别主控；K230 仅用于独立图传。禁止把邻组的 TB6612、MPU6050、6 路红外、舵机或 YOLO 方案混入本队方案。
- 最终 DOCX 必须由 LibreOffice 渲染为逐页 PNG 并逐页检查；不得有裁切、重叠、孤立标题、表格断裂或大面积异常留白。
