# A股行情与舆情分析可视化系统

这是一个适合 Python 课程项目展示的国内股票分析系统。系统将 A 股真实行情、技术指标、个股新闻舆情和风险评分整合到一个 Streamlit 可视化页面中。

## 项目亮点

- 支持输入国内股票代码，例如 `600519`、`000001`、`300750`、`002594`。
- 通过东方财富和新浪财经接口接入 A 股真实行情，不要求使用者额外安装大型金融数据包。
- 东方财富行情接口不可用时，自动切换到新浪财经真实 K 线行情。
- 优先通过东方财富获取个股真实新闻。
- 自动计算收益率、5 日均线、20 日均线、相对强弱指标、20 日波动率等技术指标。
- 对新闻标题进行情感评分，生成舆情指数和正面/中性/负面分类。
- 使用加权模型输出综合评分、风险等级和分析原因。
- 接口失败时保留离线演示兜底，课堂展示不会白屏。

## 目录结构

```text
stock_sentiment_system/
├── app.py                 # Streamlit 主界面
├── launcher.py            # exe 启动入口，自动打开浏览器
├── build_exe.ps1          # Windows 打包脚本
├── requirements.txt       # Python 依赖
├── run.ps1                # Windows 启动脚本
├── src/
│   ├── market_data.py     # A 股行情获取与技术指标
│   ├── sentiment.py       # 真实新闻获取与舆情分析
│   └── risk_model.py      # 综合评分与风险等级
├── data/                  # 可放置本地数据
└── assets/                # 可放置图片或展示素材
```

## 安装与运行

进入项目目录：

```powershell
cd C:\Users\28097\Desktop\my_code\my_code\stock_sentiment_system
```

安装依赖：

```powershell
pip install -r requirements.txt
```

启动系统：

```powershell
python -m streamlit run app.py
```

或者：

```powershell
.\run.ps1
```

启动后浏览器会打开本地页面，通常地址为：

```text
http://localhost:8501
```

## 打包成 exe

进入项目目录后运行：

```powershell
.\build_exe.ps1
```

打包完成后，成品在：

```text
dist\AStockSentimentSystem\AStockSentimentSystem.exe
```

给别人使用时，不要只发送单独的 exe 文件，需要发送整个文件夹：

```text
dist\AStockSentimentSystem\
```

对方解压或复制到本机后，双击 `AStockSentimentSystem.exe`，程序会启动本地服务并自动打开浏览器页面。关闭启动窗口即可停止系统。

`dist/`、`build/` 和 `.spec` 是打包生成物，体积较大，已经通过 `.gitignore` 排除。git 仓库只保存源码、依赖清单、运行脚本和打包脚本；需要发布成品时，可以单独压缩 `dist\AStockSentimentSystem` 文件夹发给别人。

## 推荐演示流程

1. 在左侧选择 `贵州茅台（600519）`、`平安银行（000001）` 或 `宁德时代（300750）`。
2. 切换行情周期，例如 `近3个月`、`近6个月`、`近1年`。
3. 观察 K 线图、均线图和成交量图。
4. 查看真实新闻列表、舆情指数曲线和舆情分类占比。
5. 说明综合评分模型：

```text
综合评分 = 舆情指数 × 40% + 技术趋势分 × 35% + 成交量活跃度 × 25%
```

## 数据来源

- 历史行情：东方财富接口，失败时切换到新浪财经行情接口。
- 个股新闻：东方财富个股新闻接口。
- 离线兜底：当网络或接口异常时，系统生成演示行情和演示新闻，保证页面仍可展示。

## 可扩展方向

- 加入 SQLite 保存历史查询记录。
- 增加股票对比功能。
- 接入更多新闻来源并做去重。
- 使用机器学习模型进行涨跌分类预测。
- 增加用户登录、收藏股票和自选股功能。

## 注意

本系统用于课程项目和数据分析展示，不构成投资建议。
