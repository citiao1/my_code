#include <assert.h>
#include <stdio.h>

#include "vehicle_command.h"

static void Expect(const char *text, VehicleCommandType type, unsigned int count,
                   int a0, int a1, int a2, int a3)
{
    VehicleCommand command;

    VehicleCommand_Parse(text, &command);
    assert(command.type == type);
    assert(command.argument_count == count);
    assert(command.argument[0] == a0);
    assert(command.argument[1] == a1);
    assert(command.argument[2] == a2);
    assert(command.argument[3] == a3);
}

int main(void)
{
    Expect("SQUARE,2", VEHICLE_COMMAND_SQUARE, 1U, 2, 0, 0, 0);
    Expect("DRV,-30,40", VEHICLE_COMMAND_DRIVE, 2U, -30, 40, 0, 0);
    Expect("MOTOR,8,-7", VEHICLE_COMMAND_MOTOR, 2U, 8, -7, 0, 0);
    Expect("PIDL,4000,800,0", VEHICLE_COMMAND_SPEED_PID, 3U, 4000, 800, 0, 0);
    Expect("PIDR,4000,800,0", VEHICLE_COMMAND_SPEED_PID, 3U, 4000, 800, 0, 0);
    Expect("PID,4000,800,0", VEHICLE_COMMAND_SPEED_PID, 3U, 4000, 800, 0, 0);
    Expect("YAW,1", VEHICLE_COMMAND_YAW_ENABLE, 1U, 1, 0, 0, 0);
    Expect("YAWRATE,150", VEHICLE_COMMAND_YAW_RATE, 1U, 150, 0, 0, 0);
    Expect("YAWPID,1000,2000,0,1205", VEHICLE_COMMAND_YAW_PID, 4U,
           1000, 2000, 0, 1205);
    Expect("YAWPID,1000,2000,0", VEHICLE_COMMAND_YAW_PID, 3U,
           1000, 2000, 0, 0);
    Expect("HEADPID,4000,300,1000,80", VEHICLE_COMMAND_HEADING_PID, 4U,
           4000, 300, 1000, 80);
    Expect("HEADSET,-900", VEHICLE_COMMAND_HEADING_SET, 1U, -900, 0, 0, 0);
    Expect("HEAD,1", VEHICLE_COMMAND_HEADING_ENABLE, 1U, 1, 0, 0, 0);
    Expect("LINEPID,200000,0,350000", VEHICLE_COMMAND_LINE_PID, 3U,
           200000, 0, 350000, 0);
    Expect("LINEDIFF,650", VEHICLE_COMMAND_LINE_DIFF, 1U, 650, 0, 0, 0);
    Expect("LINE,1,200", VEHICLE_COMMAND_LINE, 2U, 1, 200, 0, 0);
    Expect("BEEP,80", VEHICLE_COMMAND_BEEP, 1U, 80, 0, 0, 0);

    Expect("HEADCFG", VEHICLE_COMMAND_HEADING_CONFIG, 0U, 0, 0, 0, 0);
    Expect("LINECFG", VEHICLE_COMMAND_LINE_CONFIG, 0U, 0, 0, 0, 0);
    Expect("KEYS", VEHICLE_COMMAND_KEYS, 0U, 0, 0, 0, 0);
    Expect("STOP", VEHICLE_COMMAND_STOP, 0U, 0, 0, 0, 0);
    Expect("PING", VEHICLE_COMMAND_PING, 0U, 0, 0, 0, 0);
    Expect("ZERO", VEHICLE_COMMAND_ZERO, 0U, 0, 0, 0, 0);
    Expect("ENCZERO", VEHICLE_COMMAND_ZERO, 0U, 0, 0, 0, 0);
    Expect("IMUZERO", VEHICLE_COMMAND_IMU_ZERO, 0U, 0, 0, 0, 0);
    Expect("GRAYWHITE", VEHICLE_COMMAND_GRAY_WHITE, 0U, 0, 0, 0, 0);
    Expect("GRAYBLACK", VEHICLE_COMMAND_GRAY_BLACK, 0U, 0, 0, 0, 0);
    Expect("GRAY", VEHICLE_COMMAND_GRAY_QUERY, 0U, 0, 0, 0, 0);
    Expect("GRAYCAL", VEHICLE_COMMAND_GRAY_QUERY, 0U, 0, 0, 0, 0);
    Expect("HELP", VEHICLE_COMMAND_HELP, 0U, 0, 0, 0, 0);
    Expect("NOT_A_COMMAND", VEHICLE_COMMAND_UNKNOWN, 0U, 0, 0, 0, 0);

    puts("vehicle_command: all parser cases passed");
    return 0;
}
