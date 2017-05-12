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

typedef enum Axis {
    X = 0,
    Y
} Axis;

typedef enum Mode {
    MOVE = 0,
    PUSH
} Mode;

typedef struct Box {
    int xDistanceFromDoor;
    int yDistanceFromDoor;
    int* loc;
    //
    Direction dir;
} Box;

typedef struct Robot {
    int id;  //  keep track of robot/box location
    int xDistanceFromBox;
    int yDistanceFromBox;
    int* loc;
    int isLive;
    //
    Direction dir;
    Axis axis;
    Mode mode;

    Box box;
} Robot;

void move(Robot*);
void push(Robot*);
void end(Robot*);
void tick(Robot*);
void *start(void*); //  multi-threaded

#endif /* robot_h */
