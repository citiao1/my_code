# PID tuning data contract

Every loop is represented as a single-input, single-output table:

```text
time_s,setpoint,process,output
```

Units are intentionally not hard-coded. A speed loop can use RPM, an angular
velocity loop deg/s or rad/s, a direction loop degrees, and a gimbal loop
encoder counts or radians. The tuning report must preserve the chosen units
and the controller's actual update period.

## Controller forms

`parallel` means the continuous parallel equation:

```text
u = Kp*e + Ki*integral(e) + Kd*de/dt
```

`incremental` matches the current STM32 implementation:

```text
u[k] = u[k-1] + Kp*(e[k]-e[k-1])
                 + Ki_step*e[k]
                 + Kd_step*(e[k]-2*e[k-1]+e[k-2])
```

The current firmware ignores `dt` inside `Pid_Update`; therefore:

```text
Ki_step = Ki_continuous * T
Kd_step = Kd_continuous / T
```

`position-discrete` uses a summed error and an error difference per sample.
It is not interchangeable with the incremental form.

## Loop applicability

| Loop | Typical mode | Required ordering |
| --- | --- | --- |
| Wheel speed | PI | identify each wheel separately; reject saturated steps |
| Angular velocity | PI | tune rate loop before angle loop |
| Direction/heading | PD or PID | use wrapped angle error and a stable inner rate loop |
| Gimbal angle | PD/PID | use angle units and actuator output; tune after rate loop |
| Gimbal rate | PI/PID | use rate feedback; include output and noise limits |

These are tuning templates, not guarantees. A loop with coupling, backlash,
deadband, unstable open-loop behavior, or significant saturation needs a
different experiment or a closed-loop/relay method.
