from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
import json
import re
from typing import Iterable

import numpy as np
import pandas as pd
import requests


@dataclass(frozen=True)
class StockProfile:
    symbol: str
    name: str
    sector: str
    currency: str = "CNY"


A_SHARE_PROFILES: dict[str, StockProfile] = {
    "600519": StockProfile("600519", "贵州茅台", "白酒消费"),
    "000001": StockProfile("000001", "平安银行", "银行金融"),
    "000858": StockProfile("000858", "五粮液", "白酒消费"),
    "300750": StockProfile("300750", "宁德时代", "新能源电池"),
    "000333": StockProfile("000333", "美的集团", "家用电器"),
    "601318": StockProfile("601318", "中国平安", "保险金融"),
    "002594": StockProfile("002594", "比亚迪", "新能源汽车"),
    "600036": StockProfile("600036", "招商银行", "银行金融"),
}


def fetch_price_history(symbol: str, period: str = "6mo") -> tuple[pd.DataFrame, str]:
    symbol = normalize_symbol(symbol)
    start_date = _period_start_date(period)
    end_date = datetime.now().strftime("%Y%m%d")

    try:
        data = _fetch_akshare_history(symbol, start_date, end_date)
        if not data.empty:
            return add_technical_indicators(data), "东方财富真实行情"
    except Exception:
        pass

    try:
        data = _fetch_sina_history(symbol, period)
        if not data.empty:
            return add_technical_indicators(data), "新浪财经真实行情"
    except Exception:
        pass

    return add_technical_indicators(generate_demo_history(symbol, period)), "离线演示行情"


def fetch_realtime_quote(symbol: str) -> dict[str, float | str]:
    symbol = normalize_symbol(symbol)
    sina_symbol = to_sina_symbol(symbol)
    url = f"https://hq.sinajs.cn/list={sina_symbol}"
    headers = {"Referer": "https://finance.sina.com.cn", "User-Agent": "Mozilla/5.0"}
    response = requests.get(url, headers=headers, timeout=12)
    response.raise_for_status()
    text = response.text
    match = re.search(r'="(.*)"', text)
    if not match:
        return {}
    values = match.group(1).split(",")
    if len(values) < 32 or not values[0]:
        return {}
    return {
        "name": values[0],
        "open": _to_float(values[1]),
        "previous_close": _to_float(values[2]),
        "price": _to_float(values[3]),
        "high": _to_float(values[4]),
        "low": _to_float(values[5]),
        "volume": _to_float(values[8]),
        "amount": _to_float(values[9]),
        "date": values[30],
        "time": values[31],
    }


def _fetch_akshare_history(symbol: str, start_date: str, end_date: str) -> pd.DataFrame:
    market_code = 1 if symbol.startswith("6") else 0
    url = "https://push2his.eastmoney.com/api/qt/stock/kline/get"
    params = {
        "fields1": "f1,f2,f3,f4,f5,f6",
        "fields2": "f51,f52,f53,f54,f55,f56,f57,f58,f59,f60,f61,f116",
        "ut": "7eea3edcaed734bea9cbfc24409ed989",
        "klt": "101",
        "fqt": "1",
        "secid": f"{market_code}.{symbol}",
        "beg": start_date,
        "end": end_date,
    }
    response = requests.get(url, params=params, timeout=15)
    response.raise_for_status()
    payload = response.json()
    klines = (payload.get("data") or {}).get("klines") or []
    if not klines:
        return pd.DataFrame()
    data = pd.DataFrame([item.split(",") for item in klines])
    data = data.iloc[:, :6]
    data.columns = ["date", "open", "close", "high", "low", "volume"]
    data = data[["date", "open", "high", "low", "close", "volume"]].copy()
    data["date"] = pd.to_datetime(data["date"])
    for column in ["open", "high", "low", "close", "volume"]:
        data[column] = pd.to_numeric(data[column], errors="coerce")
    return data.dropna()


def _fetch_sina_history(symbol: str, period: str) -> pd.DataFrame:
    datalen = _period_to_days(period)
    sina_symbol = to_sina_symbol(symbol)
    url = "https://money.finance.sina.com.cn/quotes_service/api/json_v2.php/CN_MarketData.getKLineData"
    params = {"symbol": sina_symbol, "scale": "240", "ma": "no", "datalen": datalen}
    headers = {"Referer": "https://finance.sina.com.cn", "User-Agent": "Mozilla/5.0"}
    response = requests.get(url, params=params, headers=headers, timeout=15)
    response.raise_for_status()
    rows = json.loads(response.text)
    if not rows:
        return pd.DataFrame()
    data = pd.DataFrame(rows).rename(columns={"day": "date"})
    data = data[["date", "open", "high", "low", "close", "volume"]].copy()
    data["date"] = pd.to_datetime(data["date"])
    for column in ["open", "high", "low", "close", "volume"]:
        data[column] = pd.to_numeric(data[column], errors="coerce")
    return data.dropna()


def generate_demo_history(symbol: str, period: str = "6mo") -> pd.DataFrame:
    days = _period_to_days(period)
    end = datetime.now().date()
    dates = pd.bdate_range(end=end, periods=days)
    seed = sum(ord(char) for char in normalize_symbol(symbol))
    rng = np.random.default_rng(seed)

    base_price = 8 + seed % 180
    drift = rng.normal(0.0004, 0.0012, len(dates))
    volatility = rng.normal(0, 0.017, len(dates))
    shock = np.sin(np.linspace(0, 4 * np.pi, len(dates))) * 0.005
    returns = drift + volatility + shock
    close = base_price * np.cumprod(1 + returns)

    open_price = close * (1 + rng.normal(0, 0.006, len(dates)))
    high = np.maximum(open_price, close) * (1 + rng.uniform(0.002, 0.025, len(dates)))
    low = np.minimum(open_price, close) * (1 - rng.uniform(0.002, 0.025, len(dates)))
    volume = rng.integers(2_000_000, 120_000_000, len(dates))

    return pd.DataFrame(
        {
            "date": dates,
            "open": open_price,
            "high": high,
            "low": low,
            "close": close,
            "volume": volume,
        }
    )


def add_technical_indicators(df: pd.DataFrame) -> pd.DataFrame:
    data = df.copy()
    data = data.sort_values("date").reset_index(drop=True)
    data["return"] = data["close"].pct_change().fillna(0)
    data["ma5"] = data["close"].rolling(5).mean()
    data["ma20"] = data["close"].rolling(20).mean()
    data["vol_ma5"] = data["volume"].rolling(5).mean()
    data["volatility20"] = data["return"].rolling(20).std() * np.sqrt(252)
    data["rsi14"] = calculate_rsi(data["close"])
    return data


def calculate_rsi(close: pd.Series, window: int = 14) -> pd.Series:
    delta = close.diff()
    gain = delta.clip(lower=0).rolling(window).mean()
    loss = -delta.clip(upper=0).rolling(window).mean()
    rs = gain / loss.replace(0, np.nan)
    rsi = 100 - (100 / (1 + rs))
    return rsi.fillna(50)


def summarize_market(data: pd.DataFrame) -> dict[str, float]:
    latest = data.iloc[-1]
    previous = data.iloc[-2] if len(data) > 1 else latest
    start = data.iloc[0]
    return {
        "latest_close": float(latest["close"]),
        "daily_change": float((latest["close"] - previous["close"]) / previous["close"]),
        "period_change": float((latest["close"] - start["close"]) / start["close"]),
        "latest_volume": float(latest["volume"]),
        "volatility20": float(latest.get("volatility20", 0) or 0),
        "rsi14": float(latest.get("rsi14", 50) or 50),
    }


def trend_score(data: pd.DataFrame) -> float:
    latest = data.iloc[-1]
    score = 50.0
    if latest["close"] > latest.get("ma20", latest["close"]):
        score += 15
    else:
        score -= 15
    if latest.get("ma5", latest["close"]) > latest.get("ma20", latest["close"]):
        score += 10
    else:
        score -= 10
    score += max(-15, min(15, data["return"].tail(10).mean() * 1000))
    return float(max(0, min(100, score)))


def volume_score(data: pd.DataFrame) -> float:
    latest = data.iloc[-1]
    avg = latest.get("vol_ma5", latest["volume"])
    if not avg or np.isnan(avg):
        return 50.0
    ratio = latest["volume"] / avg
    return float(max(0, min(100, 50 + (ratio - 1) * 45)))


def normalize_symbol(symbol: str) -> str:
    digits = re.sub(r"\D", "", str(symbol or "600519"))
    return digits[-6:].zfill(6)


def to_sina_symbol(symbol: str) -> str:
    symbol = normalize_symbol(symbol)
    prefix = "sh" if symbol.startswith(("5", "6", "9")) else "sz"
    return f"{prefix}{symbol}"


def profile_for(symbol: str) -> StockProfile:
    symbol = normalize_symbol(symbol)
    profile = A_SHARE_PROFILES.get(symbol)
    if profile:
        try:
            quote = fetch_realtime_quote(symbol)
            name = str(quote.get("name") or profile.name)
            return StockProfile(symbol, name, profile.sector, profile.currency)
        except Exception:
            return profile
    try:
        quote = fetch_realtime_quote(symbol)
        name = str(quote.get("name") or f"{symbol} 股票")
        return StockProfile(symbol, name, "A股市场")
    except Exception:
        return StockProfile(symbol, f"{symbol} 股票", "A股市场")


def available_demo_symbols() -> Iterable[str]:
    return A_SHARE_PROFILES.keys()


def _period_start_date(period: str) -> str:
    days = _period_to_days(period)
    start = pd.Timestamp.today() - pd.tseries.offsets.BDay(days)
    return start.strftime("%Y%m%d")


def _period_to_days(period: str) -> int:
    mapping = {
        "1mo": 22,
        "3mo": 66,
        "6mo": 132,
        "1y": 252,
        "2y": 504,
    }
    return mapping.get(period, 132)


def _to_float(value: str) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return 0.0
