from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime, timedelta
import json
import math
import re

import numpy as np
import pandas as pd
import requests


POSITIVE_PATTERNS: dict[str, float] = {
    r"同比增长|环比增长": 0.55,
    r"净利润.*增长|营收.*增长|收入.*增长": 0.60,
    r"超预期|创新高|历史新高": 0.65,
    r"上调|买入评级|增持评级": 0.48,
    r"增持|回购|分红|派息|现金红利": 0.42,
    r"中标|签订合同|订单|获批|突破": 0.36,
    r"增长|盈利|利好|强劲|回暖|改善|上涨|走强": 0.32,
    r"选举|聘任|任职|董事会秘书": 0.08,
}

NEGATIVE_PATTERNS: dict[str, float] = {
    r"死叉|均线现死叉|短线防风险": -0.85,
    r"亏损|预亏|由盈转亏|业绩下滑": -0.75,
    r"减持|清仓|抛售": -0.62,
    r"处罚|立案|调查|问询函|警示函": -0.62,
    r"同比下降|环比下降|净利润.*下降|营收.*下降": -0.58,
    r"下调|卖出评级|评级下调": -0.50,
    r"下跌|回调|走弱|承压|压力|放缓|不确定": -0.35,
    r"监管|风险|利空": -0.32,
}

NEUTRAL_PATTERNS: dict[str, float] = {
    r"公告|法律意见书|股东会|董事会|监事会|提示性公告": 0.0,
    r"暂无|没有|不涉及|不会影响": 0.0,
}


@dataclass(frozen=True)
class NewsTemplate:
    title: str
    source: str
    tone: float


TEMPLATES: tuple[NewsTemplate, ...] = (
    NewsTemplate("{name}季度增长强劲，市场需求出现回暖迹象", "演示新闻", 0.78),
    NewsTemplate("多家机构上调{name}评级，盈利前景有所改善", "演示新闻", 0.72),
    NewsTemplate("{name}受到行业放缓影响，短期经营压力上升", "演示新闻", -0.56),
    NewsTemplate("{name}财报发布前波动加大，投资者保持观望", "演示新闻", -0.12),
    NewsTemplate("{name}公布新一轮创新计划，推动下一阶段产品布局", "演示新闻", 0.64),
    NewsTemplate("监管不确定性影响{name}短期市场情绪", "演示新闻", -0.68),
    NewsTemplate("机构买盘带动{name}成交量明显放大", "演示新闻", 0.58),
    NewsTemplate("{name}多空信号交织，市场等待更明确指引", "演示新闻", 0.04),
)


def fetch_stock_news(symbol: str, company_name: str, days: int = 14) -> tuple[pd.DataFrame, str]:
    frames: list[pd.DataFrame] = []
    sources: list[str] = []

    try:
        frame = _fetch_eastmoney_news(symbol)
        if not frame.empty:
            frames.append(frame)
            sources.append("东方财富新闻")
    except Exception:
        pass

    try:
        frame = _fetch_eastmoney_notices(symbol)
        if not frame.empty:
            frames.append(frame)
            sources.append("东方财富公告")
    except Exception:
        pass

    if frames:
        news = pd.concat(frames, ignore_index=True)
        news["date"] = pd.to_datetime(news["date"], errors="coerce")
        cutoff = pd.Timestamp.now() - pd.Timedelta(days=days)
        recent = news[news["date"].isna() | (news["date"] >= cutoff)].copy()

        # If a quiet stock has too few recent items, keep enough latest items so the page is not sparse.
        if len(recent) < 20:
            recent = news.sort_values("date", ascending=False, na_position="last").head(30).copy()

        recent["sentiment"] = recent.apply(
            lambda row: score_text(
                f"{row.get('title', '')} {row.get('content', '')} {row.get('category', '')}"
            ),
            axis=1,
        )
        recent["label"] = recent["sentiment"].map(sentiment_label)
        recent = _dedupe_news(recent)
        recent = recent.sort_values("date", ascending=False, na_position="last").reset_index(drop=True)
        if not recent.empty:
            return recent, " + ".join(sources)

    return build_demo_news(symbol, company_name, days), "离线演示新闻"


def _fetch_eastmoney_news(symbol: str) -> pd.DataFrame:
    url = "https://search-api-web.eastmoney.com/search/jsonp"
    callback = "jQuery35101792940631092459_1764599530165"
    inner_param = {
        "uid": "",
        "keyword": symbol,
        "type": ["cmsArticleWebOld"],
        "client": "web",
        "clientType": "web",
        "clientVersion": "curr",
        "param": {
            "cmsArticleWebOld": {
                "searchScope": "default",
                "sort": "default",
                "pageIndex": 1,
                "pageSize": 30,
                "preTag": "<em>",
                "postTag": "</em>",
            }
        },
    }
    params = {"cb": callback, "param": json.dumps(inner_param, ensure_ascii=False), "_": "1764599530176"}
    headers = {
        "Referer": f"https://so.eastmoney.com/news/s?keyword={symbol}",
        "User-Agent": "Mozilla/5.0",
    }
    response = requests.get(url, params=params, headers=headers, timeout=15)
    response.raise_for_status()
    text = response.text.strip()
    if text.startswith(callback):
        text = text[len(callback) + 1 : -1]
    payload = json.loads(text)
    rows = (payload.get("result") or {}).get("cmsArticleWebOld") or []
    if not rows:
        return pd.DataFrame()
    news = pd.DataFrame(rows)
    news = pd.DataFrame(
        {
            "date": news.get("date"),
            "title": news.get("title"),
            "source": news.get("mediaName"),
            "content": news.get("content"),
            "url": "http://finance.eastmoney.com/a/" + news.get("code").fillna("") + ".html",
        }
    )
    for column in ["title", "content"]:
        news[column] = (
            news[column]
            .fillna("")
            .str.replace(r"\(<em>", "", regex=True)
            .str.replace(r"</em>\)", "", regex=True)
            .str.replace(r"<em>", "", regex=True)
            .str.replace(r"</em>", "", regex=True)
            .str.replace(r"\u3000", "", regex=True)
            .str.replace(r"\r\n", " ", regex=True)
        )
    news["category"] = "个股新闻"
    return news


def _fetch_eastmoney_notices(symbol: str) -> pd.DataFrame:
    url = "https://np-anotice-stock.eastmoney.com/api/security/ann"
    params = {
        "sr": "-1",
        "page_size": "100",
        "page_index": "1",
        "ann_type": "A",
        "client_source": "web",
        "f_node": "0",
        "s_node": "0",
        "stock_list": symbol,
    }
    response = requests.get(url, params=params, timeout=15)
    response.raise_for_status()
    payload = response.json()
    total_hits = ((payload.get("data") or {}).get("total_hits")) or 0
    total_pages = max(1, min(3, math.ceil(total_hits / 100)))
    rows: list[dict] = []

    for page in range(1, total_pages + 1):
        params["page_index"] = str(page)
        response = requests.get(url, params=params, timeout=15)
        response.raise_for_status()
        payload = response.json()
        rows.extend(((payload.get("data") or {}).get("list")) or [])

    if not rows:
        return pd.DataFrame()
    normalized = []
    for item in rows:
        codes = item.get("codes") or []
        code_info = next((code for code in codes if str(code.get("stock_code", "")).endswith(symbol)), codes[0] if codes else {})
        columns = item.get("columns") or [{}]
        column_info = columns[0] if columns else {}
        stock_code = code_info.get("stock_code") or symbol
        art_code = item.get("art_code") or ""
        normalized.append(
            {
                "date": item.get("notice_date"),
                "title": item.get("title"),
                "category": column_info.get("column_name") or "公司公告",
                "url": f"https://data.eastmoney.com/notices/detail/{stock_code}/{art_code}.html",
            }
        )
    notices = pd.DataFrame(normalized)
    notices["source"] = "东方财富公告"
    notices["content"] = notices["category"].fillna("")
    return notices


def _dedupe_news(news: pd.DataFrame) -> pd.DataFrame:
    data = news.copy()
    data["title_key"] = data["title"].fillna("").map(lambda text: re.sub(r"\s+", "", str(text)))
    data = data.drop_duplicates(subset=["title_key"], keep="first")
    return data.drop(columns=["title_key"])


def build_demo_news(symbol: str, company_name: str, days: int = 14) -> pd.DataFrame:
    seed = sum(ord(char) for char in symbol.upper()) + 2026
    rng = np.random.default_rng(seed)
    now = datetime.now()
    rows = []
    for i in range(days):
        count = int(rng.integers(1, 4))
        for _ in range(count):
            template = TEMPLATES[int(rng.integers(0, len(TEMPLATES)))]
            noise = float(rng.normal(0, 0.12))
            score = max(-1, min(1, template.tone + noise))
            rows.append(
                {
                    "date": now - timedelta(days=i, hours=int(rng.integers(1, 20))),
                    "title": template.title.format(name=company_name),
                    "source": template.source,
                    "category": "演示数据",
                    "content": "",
                    "url": "",
                    "sentiment": score,
                    "label": sentiment_label(score),
                }
            )
    return pd.DataFrame(rows).sort_values("date", ascending=False).reset_index(drop=True)


def score_text(text: str) -> float:
    content = str(text)
    score = 0.0
    hits = 0

    for pattern, weight in POSITIVE_PATTERNS.items():
        count = len(re.findall(pattern, content))
        if count:
            score += weight * min(count, 3)
            hits += count

    for pattern, weight in NEGATIVE_PATTERNS.items():
        count = len(re.findall(pattern, content))
        if count:
            score += weight * min(count, 3)
            hits += count

    if hits == 0:
        for pattern in NEUTRAL_PATTERNS:
            if re.search(pattern, content):
                return 0.0
        return 0.0

    normalized = score / max(1.0, hits ** 0.55)
    return float(max(-1.0, min(1.0, normalized)))


def sentiment_label(score: float) -> str:
    if score >= 0.18:
        return "正面"
    if score <= -0.18:
        return "负面"
    return "中性"


def enrich_news_scores(news: pd.DataFrame) -> pd.DataFrame:
    data = news.copy()
    if "sentiment" not in data.columns:
        data["sentiment"] = data.apply(
            lambda row: score_text(f"{row.get('title', '')} {row.get('content', '')} {row.get('category', '')}"),
            axis=1,
        )
    data["label"] = data["sentiment"].map(sentiment_label)
    data["date"] = pd.to_datetime(data["date"], errors="coerce")
    return data


def daily_sentiment(news: pd.DataFrame) -> pd.DataFrame:
    data = enrich_news_scores(news)
    data = data.dropna(subset=["date"])
    if data.empty:
        return pd.DataFrame(columns=["day", "sentiment", "news_count", "sentiment_index"])
    daily = (
        data.assign(day=data["date"].dt.date)
        .groupby("day", as_index=False)
        .agg(sentiment=("sentiment", "mean"), news_count=("title", "count"))
    )
    daily["day"] = pd.to_datetime(daily["day"])
    daily["sentiment_index"] = ((daily["sentiment"] + 1) * 50).round(2)
    return daily.sort_values("day")


def sentiment_summary(news: pd.DataFrame) -> dict[str, float | str]:
    data = enrich_news_scores(news)
    avg = float(data["sentiment"].mean()) if not data.empty else 0.0
    positive = int((data["sentiment"] >= 0.18).sum())
    negative = int((data["sentiment"] <= -0.18).sum())
    neutral = int(len(data) - positive - negative)
    return {
        "avg_sentiment": avg,
        "sentiment_index": round((avg + 1) * 50, 2),
        "positive": positive,
        "negative": negative,
        "neutral": neutral,
        "dominant_label": sentiment_label(avg),
    }
