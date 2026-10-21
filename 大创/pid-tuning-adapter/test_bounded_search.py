import math
import unittest

from bounded_search import BoundedSearch, guarded_step


class SearchTests(unittest.TestCase):
    def test_bounds_reproducibility_and_real_objective_improvement(self):
        for method in ("bo", "de"):
            traces = []
            for _ in range(2):
                search = BoundedSearch(((0.6, 1.4), (0.6, 1.4)), method=method, seed=4)
                current = (1.0, 1.0)
                objective = lambda x: (x[0]-0.7)**2+(x[1]-1.3)**2
                initial = objective(current)
                search.observe(current, initial)
                trace = []
                for _ in range(16):
                    proposal = search.ask(current)
                    if proposal is None:
                        break
                    self.assertTrue(search.contains(proposal))
                    self.assertTrue(all(abs(a-b) <= b*0.2+0.000051 for a, b in zip(proposal, current)))
                    search.observe(proposal, objective(proposal), variance=0.00001)
                    current = min(search.observations, key=lambda x: x[1])[0]
                    trace.append(proposal)
                self.assertLess(min(x[1] for x in search.observations), initial*0.6)
                traces.append(trace)
            self.assertEqual(*traces)

    def test_failures_never_become_scores_and_fixed_axes_stay_fixed(self):
        search = BoundedSearch(((0, 0), (1, 3)))
        search.observe((0, 2), 1.0)
        failed = search.ask((0, 2))
        search.observe(failed, None)
        self.assertEqual(len(search.observations), 1)
        self.assertNotEqual(search.ask((0, 2)), failed)
        self.assertEqual(search.ask((0, 2))[0], 0)
        fixed = BoundedSearch(((0, 0), (1, 1)))
        self.assertIsNone(fixed.ask((0, 1)))
        with self.assertRaises(ValueError):
            search.observe((0, 2), math.nan)

    def test_rollback_steps_converge_and_do_not_enable_zero_gain(self):
        gains = (0.15, 2.5)
        target = (0.09, 3.5)
        for _ in range(10):
            gains = guarded_step(gains, target)
        self.assertEqual(gains, target)
        self.assertEqual(guarded_step((0, 1), (1, 2)), (0, 1.2))
        with self.assertRaises(ValueError):
            guarded_step((1, 2), (math.nan, 2))


if __name__ == "__main__":
    unittest.main()
