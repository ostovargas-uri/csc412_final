//
//  robot.h
//  
//
//  Created by Osto Vargas on 5/3/17.
//
//

#ifndef robot_h
#define robot_h

#include <stdio.h>
#include <stdlib.h>
//
#include "gl_frontEnd.h"

extern int** grid;
extern int numRows;
extern int numCols;

typedef struct Robot {
    int boxID;  //  keep track of robot/box location
    int dx_rb;      //  delta between robot and box -- x
    int dy_rb;      //  delta between robot and box -- y
    int dx_bd;      //  delta between box and door -- x
    int dy_bd;      //  delta between box and door -- y
    int* loc;       //  robot location
    int* boxLoc;    //  door location
    Direction boxDir;   //  direction box needs to be pushed
} Robot;

void move(Direction);
void push(Direction);
void end();
void *start(void*); //  multi-threaded

#endif /* robot_h */
