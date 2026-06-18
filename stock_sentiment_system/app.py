from __future__ import annotations

import html
import sys
from pathlib import Path

import pandas as pd
import plotly.graph_objects as go
import streamlit as st

ROOT = Path(__file__).parent
sys.path.insert(0, str(ROOT))

from src.market_data import (
    available_demo_symbols,
    fetch_price_history,
    normalize_symbol,
    profile_for,
    summarize_market,
)
from src.ml_predictor import predict_next_move
from src.risk_model import comprehensive_score
from src.sentiment import daily_sentiment, fetch_stock_news, sentiment_summary


PERIOD_OPTIONS = {
    "近1个月": "1mo",
    "近3个月": "3mo",
    "近6个月": "6mo",
    "近1年": "1y",
    "近2年": "2y",
}

PRICE_COLUMNS_CN = {
    "date": "日期",
    "open": "开盘价",
    "high": "最高价",
    "low": "最低价",
    "close": "收盘价",
    "volume": "成交量",
    "return": "日收益率",
    "ma5": "5日均线",
    "ma20": "20日均线",
    "vol_ma5": "5日均量",
    "volatility20": "20日波动率",
    "rsi14": "相对强弱指标",
}


st.set_page_config(
    page_title="A股行情与舆情分析系统",
    layout="wide",
)


def inject_style() -> None:
    st.markdown(
        """
        <style>
        .main .block-container {
            padding-top: 1.5rem;
            max-width: 1280px;
        }
        div[data-testid="stMetric"] {
            background: #ffffff;
            border: 1px solid #dfe5ee;
            border-radius: 8px;
            padding: 12px 14px;
            color: #172033;
        }
        div[data-testid="stMetric"] label,
        div[data-testid="stMetric"] [data-testid="stMetricLabel"],
        div[data-testid="stMetric"] [data-testid="stMetricValue"] {
            color: #172033 !important;
        }
        div[data-testid="stMetric"] [data-testid="stMetricDelta"] {
            color: #16884f !important;
        }
        .risk-box {
            border: 1px solid #d7dde8;
            border-radius: 8px;
            padding: 16px;
            background: #fbfcfe;
            color: #172033;
        }
        .risk-box h4,
        .risk-box p {
            color: #172033;
        }
        .news-card {
            border-bottom: 1px solid #e7ebf2;
            padding: 10px 0;
            color: #172033;
        }
        .news-card a {
            color: #1f5f82;
            text-decoration: none;
        }
        .news-card a:hover {
            text-decoration: underline;
        }
        .small-muted {
            color: #667085;
            font-size: 13px;
        }
        [data-testid="stToolbar"],
        [data-testid="stDecoration"],
        #MainMenu,
        footer {
            display: none !important;
        }
        .score-card {
            background: #ffffff;
            border: 1px solid #dfe5ee;
            border-radius: 8px;
            padding: 18px 20px;
            color: #172033;
        }
        .score-title {
            font-size: 16px;
            font-weight: 700;
            margin-bottom: 6px;
        }
        .score-value {
            font-size: 44px;
            line-height: 1;
            font-weight: 800;
            margin: 10px 0 14px;
        }
        .score-bar {
            height: 10px;
            border-radius: 999px;
            background: linear-gradient(90deg, #e66b6b 0%, #f1c75b 50%, #58a77a 100%);
            overflow: hidden;
            position: relative;
        }
        .score-marker {
            position: absolute;
            top: -3px;
            width: 4px;
            height: 16px;
            border-radius: 2px;
            background: #172033;
        }
        .score-scale {
            display: flex;
            justify-content: space-between;
            margin-top: 8px;
            color: #667085;
            font-size: 12px;
        }
        .source-pill {
            display: inline-block;
            padding: 4px 8px;
            border-radius: 999px;
            background: #eef6f3;
            color: #267252;
            font-size: 12px;
            margin-right: 6px;
        }
        .ml-box {
            border: 1px solid #d7dde8;
            border-radius: 8px;
            padding: 16px;
            background: #ffffff;
            color: #172033;
        }
        .ml-title {
            font-size: 15px;
            font-weight: 700;
            margin-bottom: 8px;
        }
        .ml-direction {
            font-size: 30px;
            line-height: 1.1;
            font-weight: 800;
            margin: 4px 0 10px;
        }
        .prob-row {
            display: grid;
            grid-template-columns: 1fr auto;
            gap: 10px;
            align-items: center;
            margin: 8px 0;
        }
        .prob-track {
            height: 9px;
            border-radius: 999px;
            background: #e8edf4;
            overflow: hidden;
        }
        .prob-fill-up {
            height: 100%;
            background: #c43d3d;
        }
        .prob-fill-down {
            height: 100%;
            background: #1b8a5a;
        }
        </style>
        """,
        unsafe_allow_html=True,
    )


def money(value: float, currency: str) -> str:
    unit = "元" if currency == "CNY" else currency
    return f"{value:,.2f} {unit}"


def pct(value: float) -> str:
    return f"{value * 100:+.2f}%"


def apply_chart_style(fig: go.Figure) -> go.Figure:
    fig.update_layout(
        template="plotly_white",
        paper_bgcolor="#ffffff",
        plot_bgcolor="#ffffff",
        font=dict(color="#172033", family="Microsoft YaHei, SimHei, Arial"),
        title_font=dict(color="#172033"),
        xaxis=dict(gridcolor="#e7ebf2", zerolinecolor="#e7ebf2"),
        yaxis=dict(gridcolor="#e7ebf2", zerolinecolor="#e7ebf2"),
    )
    return fig


def candlestick_chart(data: pd.DataFrame, symbol: str, name: str) -> go.Figure:
    fig = go.Figure()
    fig.add_trace(
        go.Candlestick(
            x=data["date"],
            open=data["open"],
            high=data["high"],
            low=data["low"],
            close=data["close"],
            name="价格K线",
            increasing_line_color="#c43d3d",
            decreasing_line_color="#1b8a5a",
            increasing_fillcolor="#c43d3d",
            decreasing_fillcolor="#1b8a5a",
        )
    )
    fig.add_trace(go.Scatter(x=data["date"], y=data["ma5"], name="5日均线", line=dict(color="#3366cc", width=1.8)))
    fig.add_trace(go.Scatter(x=data["date"], y=data["ma20"], name="20日均线", line=dict(color="#f28e2b", width=1.8)))
    fig.update_layout(
        title=f"{name}（{symbol}）价格走势",
        height=460,
        margin=dict(l=10, r=10, t=48, b=10),
        xaxis_rangeslider_visible=False,
        xaxis=dict(tickformat="%Y年%m月"),
        legend=dict(orientation="h", yanchor="bottom", y=1.02, xanchor="right", x=1),
    )
    return apply_chart_style(fig)


def volume_chart(data: pd.DataFrame) -> go.Figure:
    colors = ["#c43d3d" if row.close >= row.open else "#1b8a5a" for row in data.itertuples()]
    fig = go.Figure()
    fig.add_trace(go.Bar(x=data["date"], y=data["volume"], name="成交量", marker_color=colors))
    fig.add_trace(go.Scatter(x=data["date"], y=data["vol_ma5"], name="5日均量", line=dict(color="#5b6c8f")))
    fig.update_layout(
        height=260,
        margin=dict(l=10, r=10, t=35, b=10),
        title="成交量变化",
        xaxis=dict(tickformat="%Y年%m月"),
    )
    return apply_chart_style(fig)


def sentiment_chart(news: pd.DataFrame) -> go.Figure:
    daily = daily_sentiment(news)
    fig = go.Figure()
    fig.add_trace(
        go.Scatter(
            x=daily["day"],
            y=daily["sentiment_index"],
            mode="lines+markers",
            name="情绪指数",
            line=dict(color="#2f6f73", width=2.4),
        )
    )
    fig.add_bar(x=daily["day"], y=daily["news_count"], name="新闻数量", yaxis="y2", marker_color="#a9b7c8")
    fig.update_layout(
        title="新闻舆情指数",
        height=320,
        margin=dict(l=10, r=10, t=45, b=10),
        xaxis=dict(tickformat="%m月%d日"),
        yaxis=dict(title="情绪指数", range=[0, 100]),
        yaxis2=dict(title="新闻数", overlaying="y", side="right", showgrid=False),
        legend=dict(orientation="h", yanchor="bottom", y=1.02, xanchor="right", x=1),
    )
    return apply_chart_style(fig)


def render_score_card(score: float) -> None:
    marker = max(0, min(100, score))
    st.markdown(
        f"""
        <div class="score-card">
            <div class="score-title">综合评分</div>
            <div class="score-value">{score:.0f}<span style="font-size:18px;color:#667085;"> / 100</span></div>
            <div class="score-bar">
                <div class="score-marker" style="left:calc({marker:.1f}% - 2px);"></div>
            </div>
            <div class="score-scale"><span>风险偏高</span><span>中性观察</span><span>表现积极</span></div>
        </div>
        """,
        unsafe_allow_html=True,
    )


def render_news(news: pd.DataFrame) -> None:
    label_color = {"正面": "#16794c", "中性": "#667085", "负面": "#b42318"}
    for row in news.head(24).itertuples():
        color = label_color.get(row.label, "#667085")
        title = html.escape(str(row.title))
        source = html.escape(str(row.source))
        category = html.escape(str(getattr(row, "category", "") or ""))
        url = html.escape(str(getattr(row, "url", "") or ""))
        title_html = f'<a href="{url}" target="_blank">{title}</a>' if url else title
        date_value = pd.to_datetime(row.date, errors="coerce")
        date_text = date_value.strftime("%Y-%m-%d %H:%M") if not pd.isna(date_value) else "时间未知"
        meta = f"{source} · {category} · {date_text}" if category else f"{source} · {date_text}"
        st.markdown(
            f"""
            <div class="news-card">
                <strong>{title_html}</strong><br>
                <span class="small-muted">{meta}</span>
                <span style="float:right;color:{color};font-weight:600;">{row.label} {row.sentiment:+.2f}</span>
            </div>
            """,
            unsafe_allow_html=True,
        )


def render_prediction_card(prediction: dict[str, object]) -> None:
    if prediction.get("status") != "已训练":
        message = html.escape(str(prediction.get("message", "样本不足，暂时无法训练机器学习模型。")))
        st.markdown(
            f"""
            <div class="ml-box">
                <div class="ml-title">机器学习预测</div>
                <p class="small-muted">{message}</p>
            </div>
            """,
            unsafe_allow_html=True,
        )
        return

    direction = html.escape(str(prediction["direction"]))
    probability_up = float(prediction["probability_up"])
    probability_down = float(prediction["probability_down"])
    accuracy = float(prediction["accuracy"])
    confidence = float(prediction["confidence"])
    st.markdown(
        f"""
        <div class="ml-box">
            <div class="ml-title">机器学习预测</div>
            <div class="ml-direction">{direction}</div>
            <div class="prob-row">
                <div>
                    <div class="small-muted">上涨概率</div>
                    <div class="prob-track"><div class="prob-fill-up" style="width:{probability_up:.1f}%;"></div></div>
                </div>
                <strong>{probability_up:.1f}%</strong>
            </div>
            <div class="prob-row">
                <div>
                    <div class="small-muted">下跌概率</div>
                    <div class="prob-track"><div class="prob-fill-down" style="width:{probability_down:.1f}%;"></div></div>
                </div>
                <strong>{probability_down:.1f}%</strong>
            </div>
            <p class="small-muted">回测准确率 {accuracy:.1f}% · 信心强度 {confidence:.1f}%</p>
        </div>
        """,
        unsafe_allow_html=True,
    )


inject_style()

st.title("A股行情与舆情分析可视化系统")

with st.sidebar:
    st.header("分析参数")
    demo_symbols = list(available_demo_symbols())
    selected = st.selectbox("常用A股", demo_symbols, index=0, format_func=lambda code: f"{profile_for(code).name}（{code}）")
    custom_symbol = st.text_input("自定义股票代码", value=selected, help="例如：600519、000001、300750、002594")
    period_label = st.selectbox("行情周期", list(PERIOD_OPTIONS.keys()), index=2)
    period = PERIOD_OPTIONS[period_label]
    news_days = st.slider("舆情时间范围", min_value=7, max_value=30, value=14, step=1)
    st.caption("系统会优先联网获取国内真实行情和个股新闻；接口失败时自动启用离线演示数据。")

symbol = normalize_symbol(custom_symbol or selected)
profile = profile_for(symbol)

prices, price_source = fetch_price_history(symbol, period)
news, news_source = fetch_stock_news(symbol, profile.name, days=news_days)
market = summarize_market(prices)
sentiment = sentiment_summary(news)
risk = comprehensive_score(prices, news)
prediction = predict_next_move(prices)

top_left, top_right = st.columns([2.4, 1])
with top_left:
    st.subheader(f"{profile.name}（{profile.symbol}）")
    st.caption(f"行业：{profile.sector}")
    st.markdown(
        f"""
        <span class="source-pill">行情：{html.escape(price_source)}</span>
        <span class="source-pill">新闻：{html.escape(news_source)}</span>
        """,
        unsafe_allow_html=True,
    )
with top_right:
    render_score_card(float(risk["final_score"]))

metric_cols = st.columns(6)
metric_cols[0].metric("最新收盘价", money(market["latest_close"], profile.currency), pct(market["daily_change"]))
metric_cols[1].metric("周期涨跌幅", pct(market["period_change"]))
metric_cols[2].metric("舆情指数", f"{sentiment['sentiment_index']:.1f}", sentiment["dominant_label"])
metric_cols[3].metric("20日波动率", f"{market['volatility20'] * 100:.1f}%")
metric_cols[4].metric("相对强弱指标", f"{market['rsi14']:.1f}")
if prediction.get("status") == "已训练":
    metric_cols[5].metric("机器学习预测", str(prediction["direction"]), f"上涨概率 {prediction['probability_up']}%")
else:
    metric_cols[5].metric("机器学习预测", "样本不足", "切换更长周期")

chart_left, chart_right = st.columns([1.7, 1])
with chart_left:
    st.plotly_chart(candlestick_chart(prices, symbol, profile.name), width="stretch")
    st.plotly_chart(volume_chart(prices), width="stretch")
with chart_right:
    st.markdown(
        f"""
        <div class="risk-box">
            <h4 style="margin-top:0;">风险评估：{risk['risk_level']}</h4>
            <p>{risk['suggestion']}</p>
            <p class="small-muted">评分构成：舆情 {risk['sentiment_score']} · 趋势 {risk['trend_score']} · 成交量 {risk['volume_score']}</p>
        </div>
        """,
        unsafe_allow_html=True,
    )
    st.write("")
    for reason in risk["reasons"]:
        st.info(reason)
    render_prediction_card(prediction)
    st.write("")
    st.plotly_chart(sentiment_chart(news), width="stretch")

tab_news, tab_data, tab_ml, tab_model = st.tabs(["新闻舆情", "行情数据", "机器学习预测", "模型说明"])
with tab_news:
    left, right = st.columns([1.4, 1])
    with left:
        render_news(news)
    with right:
        counts = news["label"].value_counts().reindex(["正面", "中性", "负面"]).fillna(0)
        fig = go.Figure(
            data=[
                go.Pie(
                    labels=counts.index,
                    values=counts.values,
                    hole=0.45,
                    marker=dict(colors=["#4c956c", "#9aa7b7", "#d45b5b"]),
                )
            ]
        )
        fig.update_layout(title="舆情分类占比", height=330, margin=dict(l=10, r=10, t=45, b=10))
        fig = apply_chart_style(fig)
        st.plotly_chart(fig, width="stretch")

with tab_data:
    display_prices = (
        prices.sort_values("date", ascending=False)
        .assign(date=lambda df: df["date"].dt.strftime("%Y-%m-%d"))
        .rename(columns=PRICE_COLUMNS_CN)
    )
    st.dataframe(
        display_prices,
        width="stretch",
        hide_index=True,
    )

with tab_ml:
    render_prediction_card(prediction)
    if prediction.get("status") == "已训练":
        st.write("")
        st.subheader("主要影响因素")
        drivers = pd.DataFrame(prediction["drivers"])
        if not drivers.empty:
            drivers = drivers.rename(columns={"name": "特征", "effect": "影响方向", "value": "贡献值"})
            st.dataframe(drivers, width="stretch", hide_index=True)

        st.subheader("训练与验证")
        st.markdown(
            f"""
            - 模型类型：`{prediction['model']}`
            - 有效样本数：`{prediction['sample_count']}` 个交易日
            - 训练集：`{prediction['train_count']}` 条，测试集：`{prediction['test_count']}` 条
            - 测试集准确率：`{prediction['accuracy']}%`
            - 简单基准准确率：`{prediction['baseline_accuracy']}%`

            这里的标签定义为：如果下一交易日收盘价高于当前交易日收盘价，则记为“上涨”，否则记为“下跌”。
            """
        )

with tab_model:
    st.markdown(
        """
        本系统包含两个模型层：

        1. 综合评分模型

        `综合评分 = 舆情指数 × 40% + 技术趋势分 × 35% + 成交量活跃度 × 25%`

        2. 机器学习方向预测模型

        系统会从历史 K 线中自动生成训练样本。每一天的特征包括 1 日/3 日/5 日收益率、均线偏离、成交量活跃度、20 日波动率、RSI、日内振幅等；标签是“下一交易日是否上涨”。

        模型采用轻量级逻辑回归，通过历史样本训练后，用最新交易日的特征预测下一交易日上涨概率。页面展示的“上涨概率”“下跌概率”“回测准确率”和“主要影响因素”都来自这个模型。

        行情数据优先来自东方财富接口，失败时自动切换到新浪财经真实行情接口；新闻数据优先来自东方财富个股新闻。
        舆情指数来自新闻标题关键词情感得分，技术趋势分参考 5 日均线、20 日均线和近期收益率，成交量活跃度参考当前成交量与 5 日均量的关系。

        本系统用于课程项目展示和辅助分析，不构成任何投资建议。
        """
    )
