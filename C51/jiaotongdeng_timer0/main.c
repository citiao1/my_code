/* AT89S51, 12 MHz / 12T. Traffic LEDs are active low. */
#include <reg51.h>

typedef unsigned char u8;

#define PHASE_SECONDS       1       /* N: duration of each normal phase. */
#define T0_RELOAD_HIGH      0x3c    /* 65536 - 50000 = 0x3cb0: 50 ms. */
#define T0_RELOAD_LOW       0xb0
#define T1_RELOAD_HIGH      0xd8    /* 65536 - 10000 = 0xd8f0: 10 ms. */
#define T1_RELOAD_LOW       0xf0
#define ALL_RED             0xdb

#if PHASE_SECONDS < 1 || PHASE_SECONDS > 255
#error PHASE_SECONDS must be between 1 and 255
#endif

sbit K1 = P3^2;

/* P1.0..P1.5: NS green/yellow/red, EW green/yellow/red. */
static const u8 code phase_output[4] = {0xde, 0xdd, 0xf3, 0xeb};
static volatile u8 normal_ticks;
static volatile u8 seconds_pending;
static volatile u8 emergency_ticks;
static volatile u8 release_ticks;
static volatile bit emergency_request;
static volatile bit emergency_finished;
static bit emergency_active;
static u8 phase;
static u8 phase_seconds;

void timer0_isr(void) interrupt 1
{
    TH0 = T0_RELOAD_HIGH;
    TL0 = T0_RELOAD_LOW;
    if (++normal_ticks == 20)       /* 20 x 50 ms = 1 second. */
    {
        normal_ticks = 0;
        ++seconds_pending;
    }
}

void int0_isr(void) interrupt 0
{
    EX0 = 0;                       /* Lock out bounce and a held button. */
    TR0 = 0;                       /* Preserve the interrupted phase time. */
    P1 = ALL_RED;                  /* Respond immediately to INT0. */
    emergency_request = 1;
}

void timer1_isr(void) interrupt 3
{
    TH1 = T1_RELOAD_HIGH;
    TL1 = T1_RELOAD_LOW;
    if (emergency_ticks < 200)
    {
        if (++emergency_ticks == 200)
            emergency_finished = 1; /* 200 x 10 ms = 2 seconds. */
        return;
    }

    /* Require three high samples after the emergency before rearming. */
    if (K1 == 0)
        release_ticks = 0;
    else if (release_ticks < 3)
        ++release_ticks;            /* Three samples span at least 20 ms. */
}

static void traffic_init(void)
{
    EA = 0;
    IE = 0;
    IP = 0;                        /* All ISRs at the same priority. */
    TR0 = 0;
    TR1 = 0;
    phase = 0;
    phase_seconds = 0;
    normal_ticks = 0;
    seconds_pending = 0;
    emergency_ticks = 0;
    release_ticks = 0;
    emergency_request = 0;
    emergency_finished = 0;
    emergency_active = 0;
    P1 = phase_output[phase];
    K1 = 1;
    TMOD = 0x11;                   /* T0/T1: timer, mode 1, GATE = 0. */
    TH0 = T0_RELOAD_HIGH;
    TL0 = T0_RELOAD_LOW;
    TH1 = T1_RELOAD_HIGH;
    TL1 = T1_RELOAD_LOW;
    TF0 = 0;
    TF1 = 0;
    IT0 = 1;                       /* INT0 falling edge. */
    IE0 = 0;
    ET0 = 1;
    ET1 = 1;
    EX0 = 1;
    TR0 = 1;
    EA = 1;
}

static void emergency_start(void)
{
    emergency_request = 0;
    emergency_active = 1;
    emergency_finished = 0;
    emergency_ticks = 0;
    release_ticks = 0;
    TR1 = 0;
    TH1 = T1_RELOAD_HIGH;
    TL1 = T1_RELOAD_LOW;
    TF1 = 0;
    TR1 = 1;
}

static void traffic_process(void)
{
    /* Atomic flags and outputs: INT0 cannot be overwritten by this loop. */
    EA = 0;
    if (emergency_request)
        emergency_start();

    if (emergency_active && emergency_finished)
    {
        emergency_active = 0;
        P1 = phase_output[phase];
        TR0 = 1;                   /* Continue the saved fraction of 50 ms. */
    }

    if (!emergency_active && seconds_pending != 0)
    {
        --seconds_pending;
        if (++phase_seconds == PHASE_SECONDS)
        {
            phase_seconds = 0;
            phase = (phase + 1) & 0x03;
            P1 = phase_output[phase];
        }
    }

    if (!EX0 && !emergency_active && release_ticks >= 3 && K1 != 0)
    {
        TR1 = 0;
        TF1 = 0;
        IE0 = 0;
        EX0 = 1;
    }
    EA = 1;
}

void main(void)
{
    traffic_init();
    while (1)
        traffic_process();
}
