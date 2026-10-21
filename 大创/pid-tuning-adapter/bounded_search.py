"""Small, deterministic ask/tell searches. No hardware or data-source privileges.

BO uses a fixed RBF Gaussian process and expected improvement; DE uses a
sequential bounded rand/1/bin population. Neither is a safety certificate.
Only validated, repeated training scores belong in observe().
"""
from __future__ import annotations

import math
import random
from statistics import mean


def guarded_step(current, requested, fraction=0.2):
    """One rounded RAM write; a zero coefficient stays fixed at zero."""
    if len(current) != 2 or len(requested) != 2:
        raise ValueError("need exactly two coefficients")
    if not math.isfinite(fraction) or not 0 < fraction <= 0.2:
        raise ValueError("gain change fraction must be in (0, 0.2]")
    if any(not math.isfinite(x) or not 0 <= x <= 100 for x in (*current, *requested)):
        raise ValueError("gains must be finite and in 0..100")
    return tuple(round(max(old * (1-fraction), min(old * (1+fraction), new)), 4)
                 for old, new in zip(current, requested))


def _solve_lower(matrix, vector):
    result = []
    for i, row in enumerate(matrix):
        result.append((vector[i] - sum(row[j] * result[j] for j in range(i))) / row[i])
    return result


def _cholesky(matrix):
    lower = [[0.0] * len(matrix) for _ in matrix]
    for i in range(len(matrix)):
        for j in range(i+1):
            value = matrix[i][j] - sum(lower[i][k]*lower[j][k] for k in range(j))
            lower[i][j] = math.sqrt(max(value, 1e-12)) if i == j else value/lower[j][j]
    return lower


class BoundedSearch:
    def __init__(self, bounds, *, method="bo", seed=0, pool_size=192):
        if method not in ("bo", "de", "heuristic"):
            raise ValueError("search must be bo, de or heuristic")
        self.bounds = tuple(tuple(pair) for pair in bounds)
        if len(self.bounds) != 2 or any(
            len(pair) != 2 or not all(math.isfinite(x) for x in pair) or
            not 0 <= pair[0] <= pair[1] <= 100 for pair in self.bounds
        ):
            raise ValueError("need two finite gain intervals in 0..100")
        if pool_size < 16:
            raise ValueError("candidate pool too small")
        self.method, self.seed, self.pool_size = method, seed, pool_size
        self.random = random.Random(seed)
        self.observations = []
        self.failed = set()
        self.population = []
        self._pending_parent = None

    def contains(self, gains):
        return len(gains) == 2 and all(
            math.isfinite(x) and low <= x <= high
            for x, (low, high) in zip(gains, self.bounds))

    def observe(self, gains, score, *, variance=0.0):
        gains = tuple(gains)
        if not self.contains(gains):
            raise ValueError("observation outside search bounds")
        if score is None:
            self.failed.add(gains)  # Never invent a finite performance score.
            self._pending_parent = None
            return
        if not math.isfinite(score) or score < 0 or not math.isfinite(variance) or variance < 0:
            raise ValueError("invalid measured objective or mean variance")
        self.observations.append((gains, float(score), float(variance)))
        if self._pending_parent is not None:
            parent = self._pending_parent
            if score < self.population[parent][1]:
                self.population[parent] = (gains, score)
        elif len(self.population) < 6:
            self.population.append((gains, score))
        self._pending_parent = None

    def _coordinates(self, gains):
        return tuple((x-low)/(high-low) if high > low else 0.0
                     for x, (low, high) in zip(gains, self.bounds))

    @staticmethod
    def _kernel(a, b):
        return math.exp(-0.5 * sum(((x-y)/0.35)**2 for x, y in zip(a, b)))

    def _expected_improvement(self, pool):
        rows = self.observations
        points = [self._coordinates(row[0]) for row in rows]
        scores = [row[1] for row in rows]
        center = mean(scores)
        scale = max(0.05, max(scores)-min(scores))
        values = [(score-center)/scale for score in scores]
        matrix = [[self._kernel(a, b) for b in points] for a in points]
        for i, row in enumerate(rows):
            matrix[i][i] += max(0.0025, row[2]/scale**2)
        lower = _cholesky(matrix)
        transformed = _solve_lower(lower, values)
        best = min(values)
        result = []
        for gains in pool:
            x = self._coordinates(gains)
            vector = _solve_lower(lower, [self._kernel(x, p) for p in points])
            predicted = sum(a*b for a, b in zip(vector, transformed))
            sigma = math.sqrt(max(1e-12, 1-sum(v*v for v in vector)))
            improvement = best-predicted-0.01
            z = improvement/sigma
            cdf = 0.5*(1+math.erf(z/math.sqrt(2)))
            density = math.exp(-0.5*z*z)/math.sqrt(2*math.pi)
            result.append(improvement*cdf+sigma*density)
        return result

    def ask(self, current, *, heuristic=None):
        current = tuple(current)
        if not self.contains(current):
            raise ValueError("current gains outside search bounds")
        seen = {row[0] for row in self.observations} | self.failed | {current}
        reachable = [(max(low, old*0.8), min(high, old*1.2))
                     for old, (low, high) in zip(current, self.bounds)]

        def constrain(point):
            point = tuple(round(max(low, min(high, value)), 4)
                          for value, (low, high) in zip(point, reachable))
            return point if self.contains(point) and point not in seen else None

        if self.method == "heuristic":
            return constrain(heuristic) if heuristic is not None else None
        if self.method == "de" and len(self.population) >= 4:
            parent = self.random.randrange(len(self.population))
            indices = self.random.sample([i for i in range(len(self.population)) if i != parent], 3)
            a, b, c = [self.population[i][0] for i in indices]
            forced = self.random.randrange(2)
            point = constrain(tuple(a[j]+0.7*(b[j]-c[j])
                                    if j == forced or self.random.random() < 0.8
                                    else self.population[parent][0][j] for j in range(2)))
            if point is not None:
                self._pending_parent = parent
                return point
        pool = []
        for _ in range(self.pool_size):
            point = constrain(tuple(self.random.uniform(low, high) for low, high in reachable))
            if point is not None and point not in pool:
                pool.append(point)
        if not pool:
            return None
        if self.method != "bo" or len(self.observations) < 3:
            return pool[0]
        acquisition = self._expected_improvement(pool)
        return pool[max(range(len(pool)), key=acquisition.__getitem__)]
