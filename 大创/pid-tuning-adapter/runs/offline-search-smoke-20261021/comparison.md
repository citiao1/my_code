# Offline search comparison

Evidence: simulation_only. No physical vehicle was accessed.
These fixed synthetic plants do not establish BM50 performance or a universally superior searcher.
Held-out cases use both directions, another amplitude and a different plant gain.

| Loop | Noise SD | Method | Completed | Holdout accepted | Mean trials | Failed trials | Bound violations | Mean retained score |
|---|---:|---|---:|---:|---:|---:|---:|---:|
| angular_velocity | 0.0 | heuristic | 1/1 | 0 | 18.0 | 0 | 0 | 0.91988 |
| angular_velocity | 0.0 | bo | 1/1 | 1 | 28.0 | 0 | 0 | 0.66950 |
| angular_velocity | 0.0 | de | 0/1 | 0 | 13.0 | 1 | 0 | n/a |
| angular_velocity | 0.05 | heuristic | 1/1 | 0 | 18.0 | 0 | 0 | 0.93150 |
| angular_velocity | 0.05 | bo | 1/1 | 1 | 28.0 | 0 | 0 | 0.68151 |
| angular_velocity | 0.05 | de | 0/1 | 0 | 13.0 | 1 | 0 | n/a |
| direction | 0.0 | heuristic | 1/1 | 0 | 6.0 | 0 | 0 | 0.14716 |
| direction | 0.0 | bo | 1/1 | 0 | 24.0 | 0 | 0 | 0.14716 |
| direction | 0.0 | de | 1/1 | 1 | 28.0 | 0 | 0 | 0.13297 |
| direction | 0.05 | heuristic | 1/1 | 0 | 6.0 | 0 | 0 | 0.16943 |
| direction | 0.05 | bo | 1/1 | 0 | 24.0 | 0 | 0 | 0.16943 |
| direction | 0.05 | de | 1/1 | 1 | 28.0 | 0 | 0 | 0.15099 |

The JSON retains seeds, bounds, failure reasons, per-trial metrics, dispersion and best-score convergence curves.
Failures abort a campaign and never enter score ranking. Completed counts must be read alongside retained scores.
