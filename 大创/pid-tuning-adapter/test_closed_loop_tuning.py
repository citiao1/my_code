import math
import unittest
from closed_loop_tuning import candidate, score_step, wrap_deg
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
        self.assertAlmostEqual(rate.score,heading.score)
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
