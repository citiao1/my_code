# VGT6 BM50 module test

This STM32F407VGT6 HAL project tests the extension board one module at a
time: OLED, Bluetooth, IMU660RC, four WHEELTEC BM50 motors, four quadrature
encoders and the battery-voltage ADC. The `.ioc` file is the pin/peripheral
source of truth and the MDK-ARM project is built with Keil/Arm Compiler 5.

## Hardware mapping

| Function | Pins | Configuration |
| --- | --- | --- |
| OLED CLK/DIN/RST/DC | PD14/PD13/PD12/PD11 | SSD1306 software SPI |
| IMU660RC SCK/MISO/MOSI | PA5/PA6/PA7 | SPI1 mode 3, prescaler 64 |
| IMU660RC CS/INT1 | PB12/PC4 | software CS; INT1 reserved |
| Bluetooth TX/RX | PA2/PA3 | USART2, 9600 8N1, DMA |
| BM50 A PWM/DIR/BTAK | PC6/PD0/PD4 | TIM8_CH1, direction, brake |
| BM50 B PWM/DIR/BTAK | PC7/PD1/PD5 | TIM8_CH2, direction, brake |
| BM50 C PWM/DIR/BTAK | PC8/PD2/PD6 | TIM8_CH3, direction, brake |
| BM50 D PWM/DIR/BTAK | PC9/PD3/PD7 | TIM8_CH4, direction, brake |
| Encoder A | PA8/PA9 | TIM1 encoder mode |
| Encoder B | PA0/PA1 | TIM2 encoder mode |
| Encoder C | PB14/PB15 | both-edge EXTI software quadrature |
| Encoder D | PB6/PB7 | TIM4 encoder mode |
| Battery ADC | PC0 | ADC1_IN10, 100 kOhm / 22 kOhm divider |
| Status LED | PE2 | on while IMU communication is healthy |

PB14/PB15 can be routed to TIM12_CH1/CH2, but STM32F407 TIM12 does not have
hardware encoder mode. Therefore encoder C is decoded from both GPIO edges;
the other three encoders use timer encoder mode.

## BM50 safety behavior

The BM50 interface is single PWM + DIR + BTAK, not the dual-PWM H-bridge
interface used by the older motor project. TIM8 is configured for 10 kHz
PWM2. A compare value of zero keeps PWM high (stop); increasing compare
increases the low-level run duty.

At reset all compare values are zero and all four BTAK pins are low, so the
motors are actively braked. Motion commands are rejected until `ARM` is
received. Only one motor can jog at a time, the duty is limited to 30%, and
the command automatically stops after at most 5 seconds. `STOP` and
`DISARM` immediately set PWM to stop and pull all BTAK pins low.

Before applying the motor supply, raise all four wheels off the bench. The
BM50 supply must be 9--14 V and its ground must be connected to the MCU
ground. Do not power a motor from the MCU 5 V/3.3 V rail.

## Bluetooth commands

Send one ASCII line ending in `CR`, `LF` or `CRLF`:

```text
PING
STATUS
ARM
DISARM
STOP
A+
A-
B+
B-
C+
C-
D+
D-
JOG,A,10,1000
JOG,A,-10,1000
ENC
ENCZERO
CPR,380
BAT
RECOVER,A
CAL
ZERO
WHO
HELP
```

`A+` through `D-` are fixed 10% / 1000 ms jogs. `JOG` accepts a signed duty
from -30 to +30 percent and a duration from 50 to 5000 ms. The physical
meaning of `+` is initially just `DIR=high`; verify each wheel and encoder
sign on the raised-wheel bench before using the values for chassis control.

The default encoder count is 380 transitions per output-shaft revolution,
which assumes the BM50 motor with a 19:1 gearbox. Raw totals and deltas are
valid even if this assumption is wrong; use `CPR,<value>` to correct only the
RPM calculation. `RECOVER,<A-D>` performs the BM50 BTAK high-low-high reset
sequence while stopped, then returns to the braked state.

`BAT` reports ADC raw value and filtered millivolts using:

```text
Vbat = ADC / 4095 * 3.3 V * (100 + 22) / 22
```

This image measures voltage only. It does not impose an undervoltage cutoff
because the battery chemistry/cell count and threshold have not been set.

The diagnostic response formats are:

```text
IMU,...
ENC,tick,ready,cpr,totalA,totalB,totalC,totalD,deltaA,...,rpmA_x10,...
BAT,tick,ready,raw,millivolts,failures
DRV,tick,motor_ready,armed,active,channel,direction,duty,remaining_ms
```

Periodic telemetry is disabled in the current motor-test image. The four
formats above are emitted only when `STATUS`, `ENC` or `BAT` is requested.

## First bench test order

1. Leave the 9--14 V motor supply disconnected and flash the MCU.
2. Confirm the OLED, Bluetooth `PING`, IMU `WHO` and IMU calibration still
   work.
3. Compare `BAT` with a multimeter at the battery input.
4. Rotate each wheel by hand and use `ENC` to confirm the corresponding
   channel counts and determine its sign.
5. Raise all wheels, connect the BM50 supply and common ground, then send
   `ARM`.
6. Test one short command at a time: `A+`, `A-`, then B, C and D.
7. Verify that each jog stops after one second and that `STOP`/`DISARM`
   brakes immediately.
8. Record required per-wheel DIR and encoder sign corrections before adding
   any mecanum kinematics or closed-loop control.

## Build

Open `vgt6_module_test.ioc` in STM32CubeMX 6.17, keep MDK-ARM V5 selected,
and regenerate when pin/peripheral settings change. Then build
`MDK-ARM/vgt6_module_test.uvprojx` in Keil.

On September 14, 2026, the checked-in image was built with Keil MDK-ARM
5.38 / Arm Compiler 5.06: 0 errors and 0 warnings. The `.axf` and `.hex`
outputs are under `MDK-ARM/vgt6_module_test/`.
