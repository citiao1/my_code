# Offline search comparison

Evidence: simulation_only. No physical vehicle was accessed.
These fixed synthetic plants do not establish BM50 performance or a universally superior searcher.
Held-out cases use both directions, another amplitude and a different plant gain.

| Loop | Noise SD | Method | Completed | Holdout accepted | Mean trials | Failed trials | Bound violations | Mean retained score |
|---|---:|---|---:|---:|---:|---:|---:|---:|
| angular_velocity | 0.0 | heuristic | 3/3 | 0 | 18.0 | 0 | 0 | 0.91988 |
| angular_velocity | 0.0 | bo | 2/3 | 2 | 21.0 | 1 | 0 | 0.67080 |
| angular_velocity | 0.0 | de | 1/3 | 1 | 16.0 | 2 | 0 | 0.88390 |
| angular_velocity | 0.05 | heuristic | 3/3 | 0 | 18.0 | 0 | 0 | 0.93372 |
| angular_velocity | 0.05 | bo | 2/3 | 2 | 21.0 | 1 | 0 | 0.68150 |
| angular_velocity | 0.05 | de | 1/3 | 1 | 16.0 | 2 | 0 | 0.89346 |
| direction | 0.0 | heuristic | 3/3 | 0 | 6.0 | 0 | 0 | 0.14716 |
| direction | 0.0 | bo | 3/3 | 2 | 26.7 | 0 | 0 | 0.13414 |
| direction | 0.0 | de | 3/3 | 2 | 26.7 | 0 | 0 | 0.13587 |
| direction | 0.05 | heuristic | 3/3 | 0 | 6.0 | 0 | 0 | 0.17347 |
| direction | 0.05 | bo | 3/3 | 2 | 26.7 | 0 | 0 | 0.15536 |
| direction | 0.05 | de | 3/3 | 2 | 26.7 | 0 | 0 | 0.15755 |

The JSON retains seeds, bounds, failure reasons, per-trial metrics, dispersion and best-score convergence curves.
Failures abort a campaign and never enter score ranking. Completed counts must be read alongside retained scores.
