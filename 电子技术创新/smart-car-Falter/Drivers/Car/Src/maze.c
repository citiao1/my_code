#include "maze.h"

Action_t Maze_Decide(float L,float F,float R)
{
    if(R>20) return TURN_RIGHT;
    if(F>20) return MOVE_FORWARD;
    if(L>20) return TURN_LEFT;
    return TURN_BACK;
}