from __future__ import annotations

import math

import numpy as np
import pandas as pd


FEATURE_LABELS = {
    "return_1d": "1日收益率",
    "return_3d": "3日收益率",
    "return_5d": "5日收益率",
    "return_10d": "10日收益率",
    "ma5_gap": "价格偏离5日均线",
    "ma20_gap": "价格偏离20日均线",
    "ma_cross_gap": "5日/20日均线差",
    "volume_ratio": "成交量活跃度",
    "volatility20": "20日波动率",
    "rsi14_scaled": "RSI强弱",
    "candle_strength": "日内收盘强度",
    "high_low_range": "日内振幅",
}


def predict_next_move(price_data: pd.DataFrame) -> dict[str, object]:
    dataset, latest_features = _build_training_set(price_data)
    if len(dataset) < 70:
        return {
            "status": "样本不足",
            "message": "机器学习模型至少需要约 70 个有效交易日样本，建议切换到近1年或近2年行情。",
        }

    feature_names = list(FEATURE_LABELS.keys())
    x = dataset[feature_names].to_numpy(dtype=float)
    y = dataset["target_up"].to_numpy(dtype=float)

    split = max(50, int(len(dataset) * 0.8))
    if len(dataset) - split < 12:
        split = len(dataset) - 12
    x_train, y_train = x[:split], y[:split]
    x_test, y_test = x[split:], y[split:]

    mean = x_train.mean(axis=0)
    std = x_train.std(axis=0)
    std[std == 0] = 1
    x_train = (x_train - mean) / std
    x_test = (x_test - mean) / std

    weights, bias = _fit_logistic_regression(x_train, y_train)
    test_prob = _sigmoid(x_test @ weights + bias)
    test_pred = (test_prob >= 0.5).astype(float)
    accuracy = float((test_pred == y_test).mean()) if len(y_test) else 0.0
    baseline = float(max(y_test.mean(), 1 - y_test.mean())) if len(y_test) else 0.5

    latest_vector = latest_features[feature_names].to_numpy(dtype=float)
    latest_scaled = (latest_vector - mean) / std
    probability_up = float(_sigmoid(latest_scaled @ weights + bias))
    probability_down = 1 - probability_up
    direction = _direction_label(probability_up)
    confidence = abs(probability_up - 0.5) * 2

    contributions = latest_scaled * weights
    top_indices = np.argsort(np.abs(contributions))[::-1][:4]
    drivers = [
        {
            "name": FEATURE_LABELS[feature_names[index]],
            "effect": "推高上涨概率" if contributions[index] > 0 else "压低上涨概率",
            "value": round(float(contributions[index]), 3),
        }
        for index in top_indices
        if not math.isclose(float(contributions[index]), 0.0, abs_tol=1e-6)
    ]

    return {
        "status": "已训练",
        "model": "逻辑回归",
        "sample_count": int(len(dataset)),
        "train_count": int(len(x_train)),
        "test_count": int(len(x_test)),
        "accuracy": round(accuracy * 100, 1),
        "baseline_accuracy": round(baseline * 100, 1),
        "probability_up": round(probability_up * 100, 1),
        "probability_down": round(probability_down * 100, 1),
        "direction": direction,
        "confidence": round(confidence * 100, 1),
        "drivers": drivers,
    }


def _build_training_set(price_data: pd.DataFrame) -> tuple[pd.DataFrame, pd.Series]:
    data = price_data.copy().sort_values("date").reset_index(drop=True)
    data["return_1d"] = data["close"].pct_change()
    data["return_3d"] = data["close"].pct_change(3)
    data["return_5d"] = data["close"].pct_change(5)
    data["return_10d"] = data["close"].pct_change(10)
    data["ma5_gap"] = data["close"] / data["ma5"] - 1
    data["ma20_gap"] = data["close"] / data["ma20"] - 1
    data["ma_cross_gap"] = data["ma5"] / data["ma20"] - 1
    data["volume_ratio"] = data["volume"] / data["vol_ma5"] - 1
    data["rsi14_scaled"] = (data["rsi14"] - 50) / 50
    data["candle_strength"] = data["close"] / data["open"] - 1
    data["high_low_range"] = data["high"] / data["low"] - 1
    data["target_up"] = (data["close"].shift(-1) > data["close"]).astype(int)

    feature_names = list(FEATURE_LABELS.keys())
    data = data.replace([np.inf, -np.inf], np.nan)
    trainable = data.dropna(subset=feature_names + ["target_up"]).iloc[:-1].copy()
    latest = data.dropna(subset=feature_names).iloc[-1][feature_names]
    return trainable, latest


def _fit_logistic_regression(x: np.ndarray, y: np.ndarray) -> tuple[np.ndarray, float]:
    weights = np.zeros(x.shape[1], dtype=float)
    bias = 0.0
    learning_rate = 0.06
    l2 = 0.015

    for _ in range(1400):
        probability = _sigmoid(x @ weights + bias)
        error = probability - y
        gradient_w = (x.T @ error) / len(y) + l2 * weights
        gradient_b = float(error.mean())
        weights -= learning_rate * gradient_w
        bias -= learning_rate * gradient_b

    return weights, bias


def _sigmoid(value: np.ndarray | float) -> np.ndarray | float:
    clipped = np.clip(value, -35, 35)
    return 1 / (1 + np.exp(-clipped))


def _direction_label(probability_up: float) -> str:
    if probability_up >= 0.58:
        return "偏上涨"
    if probability_up <= 0.42:
        return "偏下跌"
    return "震荡不确定"
