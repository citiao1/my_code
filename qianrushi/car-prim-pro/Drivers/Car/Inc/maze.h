#ifndef MAZE_H
#define MAZE_H

typedef enum{
    MOVE_FORWARD,
    TURN_LEFT,
    TURN_RIGHT,
    TURN_BACK
}Action_t;

Action_t Maze_Decide(float L,float F,float R);

#endif