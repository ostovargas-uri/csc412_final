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

extern int numRows;
extern int numCols;

typedef enum Mode {
    X = 1,
    Y = 2,
    MOVE = 4,
    PUSH = 8
} Mode;

typedef struct Robot {
    int id;  //  keep track of robot/box location
    int dx_rb;      //  delta between robot and box -- x
    int dy_rb;      //  delta between robot and box -- y
    int dx_bd;      //  delta between box and door -- x
    int dy_bd;      //  delta between box and door -- y
    Direction dir;
    Direction boxDir;   //  direction box needs to be pushed
    int mode;
    int isLive;
} Robot;

Direction setDir(int, int);
void setAxis(Robot*);
void adjust(Robot*);
Direction turn(Mode, int);
void move(Robot*);
void push(Robot*);
void end();
void *start(void*); //  multi-threaded

#endif /* robot_h */
