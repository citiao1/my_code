from __future__ import annotations

import pandas as pd

from .market_data import trend_score, volume_score
from .sentiment import sentiment_summary


def comprehensive_score(price_data: pd.DataFrame, news_data: pd.DataFrame) -> dict[str, float | str | list[str]]:
    sentiment = sentiment_summary(news_data)
    s_score = float(sentiment["sentiment_index"])
    t_score = trend_score(price_data)
    v_score = volume_score(price_data)
    final = s_score * 0.4 + t_score * 0.35 + v_score * 0.25

    latest = price_data.iloc[-1]
    reasons: list[str] = []
    if s_score >= 60:
        reasons.append("新闻整体偏正面，市场情绪较积极")
    elif s_score <= 40:
        reasons.append("负面新闻占比偏高，短期情绪承压")
    else:
        reasons.append("新闻情绪中性，市场仍在观望")

    if t_score >= 60:
        reasons.append("价格位于均线上方，趋势表现较强")
    elif t_score <= 40:
        reasons.append("价格弱于均线，技术面需要谨慎")

    if float(latest.get("volatility20", 0) or 0) > 0.45:
        reasons.append("近 20 日波动率偏高，价格不确定性较大")
    if float(latest.get("rsi14", 50) or 50) > 70:
        reasons.append("相对强弱指标偏高，存在短线回调风险")
    elif float(latest.get("rsi14", 50) or 50) < 30:
        reasons.append("相对强弱指标偏低，可能处于超卖区域")

    return {
        "final_score": round(final, 2),
        "sentiment_score": round(s_score, 2),
        "trend_score": round(t_score, 2),
        "volume_score": round(v_score, 2),
        "risk_level": risk_level(final, s_score, price_data),
        "suggestion": suggestion(final),
        "reasons": reasons[:4],
    }


def risk_level(final_score: float, sentiment_score: float, price_data: pd.DataFrame) -> str:
    volatility = float(price_data.iloc[-1].get("volatility20", 0) or 0)
    if final_score < 38 or sentiment_score < 35 or volatility > 0.6:
        return "高风险"
    if final_score < 58 or volatility > 0.4:
        return "中等风险"
    return "低风险"


def suggestion(final_score: float) -> str:
    if final_score >= 68:
        return "综合表现偏积极，可继续关注趋势延续性。"
    if final_score >= 48:
        return "信号较为混合，适合结合更多基本面信息观察。"
    return "短期压力较明显，应重点关注负面消息和价格波动。"
