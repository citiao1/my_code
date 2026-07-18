#include "vehicle_command.h"

#include <stdio.h>
#include <string.h>

static void SetArguments(VehicleCommand *command, VehicleCommandType type,
                         uint8_t count, int a0, int a1, int a2, int a3)
{
    command->type = type;
    command->argument_count = count;
    command->argument[0] = a0;
    command->argument[1] = a1;
    command->argument[2] = a2;
    command->argument[3] = a3;
}

void VehicleCommand_Parse(const char *line, VehicleCommand *command)
{
    int a0 = 0;
    int a1 = 0;
    int a2 = 0;
    int a3 = 0;
    int count;

    if (command == NULL) return;
    memset(command, 0, sizeof(*command));
    if (line == NULL || line[0] == '\0') return;

    /*
     * 顺序与既有 V24 协议保持一致。带参数命令继续使用十进制有符号整数，
     * 从而兼容现有网页、BLE 桥和串口调试工具发出的文本。
     */
    if (sscanf(line, "SQUARE,%d", &a0) == 1)
        SetArguments(command, VEHICLE_COMMAND_SQUARE, 1U, a0, 0, 0, 0);
    else if (sscanf(line, "DRV,%d,%d", &a0, &a1) == 2)
        SetArguments(command, VEHICLE_COMMAND_DRIVE, 2U, a0, a1, 0, 0);
    else if (sscanf(line, "MOTOR,%d,%d", &a0, &a1) == 2)
        SetArguments(command, VEHICLE_COMMAND_MOTOR, 2U, a0, a1, 0, 0);
    else if (sscanf(line, "PIDL,%d,%d,%d", &a0, &a1, &a2) == 3 ||
             sscanf(line, "PIDR,%d,%d,%d", &a0, &a1, &a2) == 3 ||
             sscanf(line, "PID,%d,%d,%d", &a0, &a1, &a2) == 3)
        SetArguments(command, VEHICLE_COMMAND_SPEED_PID, 3U, a0, a1, a2, 0);
    else if (sscanf(line, "YAW,%d", &a0) == 1)
        SetArguments(command, VEHICLE_COMMAND_YAW_ENABLE, 1U, a0, 0, 0, 0);
    else if (sscanf(line, "YAWRATE,%d", &a0) == 1)
        SetArguments(command, VEHICLE_COMMAND_YAW_RATE, 1U, a0, 0, 0, 0);
    else if (strncmp(line, "YAWPID,", 7U) == 0)
    {
        count = sscanf(line, "YAWPID,%d,%d,%d,%d", &a0, &a1, &a2, &a3);
        SetArguments(command, VEHICLE_COMMAND_YAW_PID,
                     (uint8_t)(count > 0 ? count : 0), a0, a1, a2, a3);
    }
    else if (sscanf(line, "HEADPID,%d,%d,%d,%d", &a0, &a1, &a2, &a3) == 4)
        SetArguments(command, VEHICLE_COMMAND_HEADING_PID, 4U, a0, a1, a2, a3);
    else if (sscanf(line, "HEADSET,%d", &a0) == 1)
        SetArguments(command, VEHICLE_COMMAND_HEADING_SET, 1U, a0, 0, 0, 0);
    else if (sscanf(line, "HEAD,%d", &a0) == 1)
        SetArguments(command, VEHICLE_COMMAND_HEADING_ENABLE, 1U, a0, 0, 0, 0);
    else if (strcmp(line, "HEADCFG") == 0)
        command->type = VEHICLE_COMMAND_HEADING_CONFIG;
    else if (sscanf(line, "LINEPID,%d,%d,%d", &a0, &a1, &a2) == 3)
        SetArguments(command, VEHICLE_COMMAND_LINE_PID, 3U, a0, a1, a2, 0);
    else if (sscanf(line, "LINEDIFF,%d", &a0) == 1)
        SetArguments(command, VEHICLE_COMMAND_LINE_DIFF, 1U, a0, 0, 0, 0);
    else if (strcmp(line, "LINECFG") == 0)
        command->type = VEHICLE_COMMAND_LINE_CONFIG;
    else if (sscanf(line, "LINE,%d,%d", &a0, &a1) == 2)
        SetArguments(command, VEHICLE_COMMAND_LINE, 2U, a0, a1, 0, 0);
    else if (sscanf(line, "BEEP,%d", &a0) == 1)
        SetArguments(command, VEHICLE_COMMAND_BEEP, 1U, a0, 0, 0, 0);
    else if (strcmp(line, "KEYS") == 0)
        command->type = VEHICLE_COMMAND_KEYS;
    else if (strcmp(line, "STOP") == 0)
        command->type = VEHICLE_COMMAND_STOP;
    else if (strcmp(line, "PING") == 0)
        command->type = VEHICLE_COMMAND_PING;
    else if (strcmp(line, "ZERO") == 0 || strcmp(line, "ENCZERO") == 0)
        command->type = VEHICLE_COMMAND_ZERO;
    else if (strcmp(line, "IMUZERO") == 0)
        command->type = VEHICLE_COMMAND_IMU_ZERO;
    else if (strcmp(line, "GRAYWHITE") == 0)
        command->type = VEHICLE_COMMAND_GRAY_WHITE;
    else if (strcmp(line, "GRAYBLACK") == 0)
        command->type = VEHICLE_COMMAND_GRAY_BLACK;
    else if (strcmp(line, "GRAYCAL") == 0 || strcmp(line, "GRAY") == 0)
        command->type = VEHICLE_COMMAND_GRAY_QUERY;
    else if (strcmp(line, "HELP") == 0)
        command->type = VEHICLE_COMMAND_HELP;
}
