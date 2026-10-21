import math
import unittest
from closed_loop_tuning import ResponseScore, candidate, score_step, wrap_deg
from direction_model import fit_direction_model, search_direction_pd
from pid_tuning import Sample, identify_fopdt


class ClosedLoopTests(unittest.TestCase):
    def samples(self, angle=False, reverse=False):
        values=[]
        for i in range(65):
            t=i*0.2
            target=170 if i<15 else -175
            process=170 if i<15 else wrap_deg(170+(15 if not reverse else -15)*(1-math.exp(-(t-3))))
            if not angle:
                target=0 if i<15 else 15
                process=0 if i<15 else (15 if not reverse else -15)*(1-math.exp(-(t-3)))
            values.append(Sample(t,target,process,5))
        return values

    def test_heading_crossing_matches_rate_response(self):
        rate=score_step(self.samples(),angle=False,output_limit=60)
        heading=score_step(self.samples(True),angle=True,output_limit=80)
        self.assertAlmostEqual(rate.normalized_iae,heading.normalized_iae,places=2)
        self.assertLess(abs(rate.score-heading.score),0.025)
        self.assertTrue(heading.resolved_transient)
        self.assertLess(heading.steady_error,0.01)

    def test_reversed_and_saturated_response_cannot_write(self):
        with self.assertRaises(ValueError):
            score_step(self.samples(reverse=True),angle=False,output_limit=60)
        saturated=[Sample(s.time_s,s.setpoint,s.process,60) for s in self.samples()]
        with self.assertRaises(ValueError):
            score_step(saturated,angle=False,output_limit=60)
        invalid=self.samples(); invalid[16]=invalid[15]
        with self.assertRaises(ValueError):
            score_step(invalid,angle=False,output_limit=60)

    def test_response_speed_and_tracking_outweigh_brief_saturation(self):
        def response(tau, saturated_samples=0):
            values = []
            for i in range(160):
                t = i * 0.02
                target = 0.0 if i < 10 else 8.0
                process = 0.0 if i < 10 else 8.0 * (1.0 - math.exp(-(t - 0.2) / tau))
                output = 60.0 if 10 <= i < 10 + saturated_samples else 20.0
                values.append(Sample(t, target, process, output))
            return score_step(values, angle=False, output_limit=60)

        fast = response(0.16, saturated_samples=8)
        slow = response(0.8)
        self.assertLess(fast.settling_s, slow.settling_s)
        self.assertGreater(fast.saturation_fraction, slow.saturation_fraction)
        self.assertLess(fast.score, slow.score)

    def test_steady_error_includes_ripple_rms_not_only_mean_bias(self):
        values = []
        for i in range(100):
            t = i * 0.02
            target = 8.0 if i >= 10 else 0.0
            process = 8.0 if i >= 10 else 0.0
            if i >= 80:
                process += 0.8 * math.sin(i * 2 * math.pi / 13)
            values.append(Sample(t, target, process, 20.0))
        result = score_step(values, angle=False, output_limit=60)
        self.assertGreater(result.steady_error, 0.05)

    def test_closed_loop_is_not_open_loop_plant_gain(self):
        samples=[Sample(s.time_s,s.setpoint,s.process,0 if s.time_s<3 else 10+5*math.sin(s.time_s-3))
                 for s in self.samples()]
        with self.assertRaisesRegex(ValueError,"closed-loop"):
            identify_fopdt(samples)

    def test_pi_and_pd_proposals_preserve_form_and_bounds(self):
        result=score_step(self.samples(),angle=False,output_limit=60)
        for loop in ("direction","angular_velocity"):
            p,k=candidate((1,2),result,loop)
            self.assertTrue(0.8<=p<=1.2 and 1.6<=k<=2.4)
        with self.assertRaises(ValueError):
            candidate((1,2),result,"gimbal")

    def test_rate_overshoot_candidate_preserves_proportional_response(self):
        result=ResponseScore(0.12,0.19,0.06,None,0.0,True,0.9)
        self.assertEqual(candidate((0.15,2.5),result,"angular_velocity"),
                         (0.15,2.125))

    def test_direction_model_uses_yaw_rate_target_as_plant_input(self):
        records = []
        pole = 0.8
        gain = 0.6
        rate = 0.0
        for index in range(80):
            target = 20.0 if index >= 10 else 0.0
            rate = pole * rate + (1.0 - pole) * gain * target
            records.append({
                "target_rate_dps": target,
                "actual_rate_dps": rate,
            })
        model = fit_direction_model(records, sample_period_s=0.02,
                                    max_delay_s=0.08)
        self.assertEqual(model.delay_samples, 0)
        self.assertAlmostEqual(model.pole, pole, places=2)
        self.assertAlmostEqual(model.rate_gain, gain, places=2)
        ranked = search_direction_pd(model, (5.0, 1.25))
        self.assertEqual(len(ranked), 9)
        self.assertTrue(all(0.0 <= item.saturation_fraction <= 1.0
                            for item in ranked))


if __name__=="__main__":
    unittest.main()
