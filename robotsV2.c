//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2017-05-01.
//  Copyright © 2017 Jean-Yves Hervé. All rights reserved.
//
//  Modified by Osto Vargas on 2017-05-05.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
//
#include "gl_frontEnd.h"
#include "robot.h"

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);
int assignLoc(int, int, int, int**, int[][2]);

//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern const int	GRID_PANE, STATE_PANE;
extern int	gMainWindow, gSubwindow[2];

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
int** grid;
int numRows = -1;	//	height of the grid
int numCols = -1;	//	width
int numBoxes = -1;	//	also the number of robots
int numDoors = -1;	//	The number of doors.

int** robotLoc;
int** boxLoc;
int* doorAssign;	//	door id assigned to each robot-box pair
int** doorLoc;

int numLiveThreads = 0;		//	the number of live robot threads

//	robot sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int robotSleepTime = 100000;

//	An array of C-strinng where you can store things you want displayd in the spate pane
//	that you want the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;

//  Simulation's output file
FILE* fp;
//  Robots
Robot* robots;
const char* directions = "NWSE";
//
pthread_mutex_t console_mtx;
pthread_mutex_t state_mtx;

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


void displayGridPane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//	Here I hard-code myself some data for robots and doors.  Obviously this code
	//	this code must go away.  I just want to show you how to display the information
	//	about a robot-box pair or a door.
	// int robotLoc[][2] = {{12, 8}, {6, 9}, {3, 14}, {11, 15}};
	// int boxLoc[][2] = {{6, 7}, {4, 12}, {13, 13}, {8, 12}};
	// int doorAssign[] = {1, 0, 0, 2};	//	door id assigned to each robot-box pair
	// int doorLoc[][2] = {{3, 3}, {8, 11}, {7, 10}};
	for (int i=0; i<numBoxes; i++)
	{
		//	here I would test if the robot thread is still live
		if (robots[i].isLive)
			drawRobotAndBox(i, robotLoc[i][1], robotLoc[i][0], boxLoc[i][1], boxLoc[i][0], doorAssign[i]);
	}

	for (int i=0; i<numDoors; i++)
	{
		//	here I would test if the robot thread is still live
		drawDoor(i, doorLoc[i][1], doorLoc[i][0]);
	}

	//	Compared to Assignment 5, this call does nothing important. It only draws lines
	//	There is nothing to synchronize here
	drawGrid();

	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

void displayStatePane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//	Here I hard-code a few messages that I want to see displayed
	//	in my state pane.  The number of live robot threads will
	//	always get displayed.  No need to pass a message about it.
	int numMessages = 3;
	sprintf(message[0], "We have %d doors", numDoors);
	sprintf(message[1], "I like cheese");
	sprintf(message[2], "System time is %ld", time(NULL));
	
	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	pthread_mutex_lock(&state_mtx);
	drawState(numMessages, message);
	pthread_mutex_unlock(&state_mtx);
	
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupRobots(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * robotSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		robotSleepTime = newSleepTime;
	}
}

void slowdownRobots(void)
{
	//	increase sleep time by 20%
	robotSleepTime = (12 * robotSleepTime) / 10;
}




//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	the initialization of numRows, numCos, numDoors, numBoxes.
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of boxes (and robots), and the number of doors.
	//	You are going to have to extract these.  For the time being,
	//	I hard code-some values
    if (argc != 5)
        return 1;
    
    numRows = atoi(argv[1]);
    numCols = atoi(argv[2]);
    numDoors = atoi(argv[4]);
    numBoxes = atoi(argv[3]);
    //  Not enough grid spaces to fit robots, boxes, and doors.
    if  (                                               //  add the perimeter padding
        numRows * numCols <= numBoxes+numBoxes+numDoors+2*(numRows+numCols) ||
        (numRows < numBoxes+1 && numCols < numBoxes+1)  //  rows or columns are too narrow
        )
    {
        printf("Grid is too small!\n");
        return 1;
    }
    
    fp = fopen("robotSimulOut.txt", "w+");
    if (fp == NULL)
    {
        printf("Failed to create file robotSimulOut.txt\n");
        return 1;
    }
    
    fprintf(fp, "%d %d %d %d\n\n", numRows, numCols, numBoxes, numDoors);
	// numRows = 16;
	// numCols = 20;
	// numDoors = 3;
	// numBoxes = 4;

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
	
	//	Now we can do application-level initialization
	initializeApplication();

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (int i=0; i< numRows; i++)
		free(grid[i]);
	free(grid);
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);

	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a part that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void)
{
	//	Allocate the grid
	grid = (int**) malloc(numRows * sizeof(int*));
	for (int i=0; i<numRows; i++)
		grid[i] = (int*) malloc(numCols * sizeof(int));
	
	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));
		
	pthread_mutex_init(&console_mtx, NULL);
	pthread_mutex_init(&state_mtx, NULL);

	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code
	
	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));

	//	normally, here I would initialize the location of my doors, boxes,
	//	and robots, and create threads (not necessarily in that order).
	//	For the handout I have nothing to do.
    //
	// int robotLoc[][2] = {{12, 8}, {6, 9}, {3, 14}, {11, 15}};
	// int boxLoc[][2] = {{6, 7}, {4, 12}, {13, 13}, {8, 12}};
	// int doorAssign[] = {1, 0, 0, 2};	//	door id assigned to each robot-box pair
	// int doorLoc[][2] = {{3, 3}, {8, 11}, {7, 10}};
    //
    int entries = 0;
    int acquiredLoc[numBoxes+numBoxes+numDoors][2];
    
    robotLoc = (int**) malloc(sizeof(void*) * numBoxes);
    boxLoc = (int**) malloc(sizeof(void*) * numBoxes);
    doorAssign = (int*) malloc(sizeof(int) * numBoxes);
    //
    doorLoc = (int**) malloc(sizeof(void*) * numDoors);
    
    //  Assign locations for doors, boxes and robots
    fprintf(fp, "Door Locations:\n");
    entries = assignLoc(numDoors, entries, 0, doorLoc, acquiredLoc);
    
    fprintf(fp, "Box Locations:\n");
    entries = assignLoc(numBoxes, entries, 1, boxLoc, acquiredLoc);
    
    fprintf(fp, "Robot Locations:\n");
    entries = assignLoc(numBoxes, entries, 0, robotLoc, acquiredLoc);
    
    for (int k=0; k<numBoxes; k++)
        doorAssign[k] = rand() % numDoors;

    //  TEST: X Align
	// robotLoc[0][0] = 1;
	// robotLoc[0][1] = 1;
	// boxLoc[0][0] = 2;
	// boxLoc[0][1] = 1;
	// doorLoc[0][0] = 0;
	// doorLoc[0][1] = 1;
    //  TEST: Y Align
	// robotLoc[0][0] = 1;
	// robotLoc[0][1] = 1;
	// boxLoc[0][0] = 1;
	// boxLoc[0][1] = 2;
	// doorLoc[0][0] = 1;
	// doorLoc[0][1] = 0;
	// TEST: Box and Door Y Aligned
	// robotLoc[0][0] = 4;
	// robotLoc[0][1] = 0;
	// boxLoc[0][0] = 2;
	// boxLoc[0][1] = 1;
	// doorLoc[0][0] = 2;
	// doorLoc[0][1] = 5;

    robots = (Robot*) malloc(sizeof(Robot) * numBoxes);
	pthread_t* thread = (pthread_t*) malloc(sizeof(pthread_t) * numBoxes);
	numLiveThreads = numBoxes;

    for (int k = 0; k<numBoxes; k++)
    {
		Robot* robot = &robots[k];
        robot->id = k;
        robot->dx_bd = doorLoc[doorAssign[k]][0] - boxLoc[k][0];
        robot->dy_bd = doorLoc[doorAssign[k]][1] - boxLoc[k][1];
		robot->isLive = 1;

        //  decide starting push direction (x direction is prioritized)
        //
		robot->boxDir = setDir(robot->dx_bd, robot->dy_bd);

        //  calculate delta x & y for robot's initial travel to box
        //
        switch (robot->boxDir)
        {
            case NORTH:
                robot->dx_rb = boxLoc[k][0] - robotLoc[k][0];
                robot->dy_rb = boxLoc[k][1] - robotLoc[k][1] - 1;   //  get on bottom side
                break;
            case WEST:
                robot->dx_rb = boxLoc[k][0] - robotLoc[k][0] + 1;   //  get on right side
                robot->dy_rb = boxLoc[k][1] - robotLoc[k][1];
                break;
            case SOUTH:
                robot->dx_rb = boxLoc[k][0] - robotLoc[k][0];
                robot->dy_rb = boxLoc[k][1] - robotLoc[k][1] + 1;   //  get on top side
                break;
            case EAST:
                robot->dx_rb = boxLoc[k][0] - robotLoc[k][0] - 1;   //  get on left side
                robot->dy_rb = boxLoc[k][1] - robotLoc[k][1];
                break;
        }

        //  DEBUG
		// fprintf(stderr, "box %d direction: %d\n", k, robot->boxDir);
		// fprintf(stderr, "box %d trajected travel:\n\tx: %3d\n\ty: %3d\n", k, robot->dx_bd, robot->dy_bd);
		// fprintf(stderr, "robot %d trajected travel:\n\tx: %3d\n\ty: %3d\n", k, robot->dx_rb, robot->dy_rb);
		// fprintf(stderr, "robotLoc: {%d, %d}\n", robotLoc[k][0], robotLoc[k][1]);
		// fprintf(stderr, "boxLoc: {%d, %d}\n\n", boxLoc[k][0], boxLoc[k][1]);
		
		setAxis(robot);

		pthread_create(&thread[k], NULL, start, (void*) robot);

		// fprintf(stderr, "robot %d direction: %d\n", k, robot->dir);
    }
}

Direction setDir(int dx, int dy)
{
	Direction dir;
	if (dx < 0)       		//  door is westward -dx
		dir = WEST;
	else if (dx > 0)		//  door is eastward +dx
		dir = EAST;
	else
	{
		if (dy < 0)   		//  door is southward -dy
			dir = SOUTH;
		else if (dy > 0)	//  door is northward +dy
			dir = NORTH;
	}

	return dir;
}

void setAxis(Robot* robot)
{
	switch (robot->boxDir)
	{
		case NORTH:
			robot->mode = MOVE | Y;
			if (robot->dy_rb < 0)
				robot->dir = SOUTH;
			else if (robot->dy_rb > 0)
				robot->dir = NORTH;
			else
			{
				robot->dir = setDir(robot->dx_rb, robot->dy_rb);
				robot->mode = MOVE | X;
			}
			break;
		case WEST:
			robot->mode = MOVE | X;
			if (robot->dx_rb < 0)
				robot->dir = WEST;
			else if (robot->dx_rb > 0)
				robot->dir = EAST;
			else
			{
				robot->dir = setDir(robot->dx_rb, robot->dy_rb);
				robot->mode = MOVE | Y;
			}
		case SOUTH:
			robot->mode = MOVE | Y;
			if (robot->dy_rb < 0)
				robot->dir = SOUTH;
			else if (robot->dy_rb > 0)
				robot->dir = NORTH;
			else
			{
				robot->dir = setDir(robot->dx_rb, robot->dy_rb);
				robot->mode = MOVE | X;
			}
		case EAST:
			robot->mode = MOVE | X;
			if (robot->dx_rb < 0)
				robot->dir = WEST;
			else if (robot->dx_rb > 0)
				robot->dir = EAST;
			else
			{
				robot->dir = setDir(robot->dx_rb, robot->dy_rb);
				robot->mode = MOVE | Y;
			}
			break;
	}
}

void adjust(Robot* robot)
{
	//	new direction for box
	robot->boxDir = setDir(robot->dx_bd, robot->dy_bd);

	switch (robot->boxDir)
	{
		case NORTH:
			robot->dx_rb = boxLoc[robot->id][0] - robotLoc[robot->id][0];
			robot->dy_rb = -1;
			break;
		case WEST:
			robot->dx_rb = 1;
			robot->dy_rb = boxLoc[robot->id][1] - robotLoc[robot->id][1];
			break;
		case SOUTH:
			robot->dx_rb = boxLoc[robot->id][0] - robotLoc[robot->id][0];
			robot->dy_rb = 1;
			break;
		case EAST:
			robot->dx_rb = -1;
			robot->dy_rb = boxLoc[robot->id][1] - robotLoc[robot->id][1];
			break;
	}
}

Direction turn(Mode axis, int delta)
{
	Direction dir;
	switch (axis)
	{
		case X:
			if (delta < 0)
				dir = WEST;
			else
				dir = EAST;
			break;
		case Y:
			if (delta < 0)
				dir = SOUTH;
			else
				dir = NORTH;
	}
	return dir;
}

void move(Robot* robot)
{
	switch (robot->dir)
	{
		case NORTH:
			robotLoc[robot->id][1]++;
			robot->dy_rb--;
			break;
		case WEST:
			robotLoc[robot->id][0]--;
			robot->dx_rb++;
			break;
		case SOUTH:
			robotLoc[robot->id][1]--;
			robot->dy_rb++;
			break;
		case EAST:
			robotLoc[robot->id][0]++;
			robot->dx_rb--;
			break;
	}
	pthread_mutex_lock(&console_mtx);
	fprintf(fp, "robot %d move %c\n", robot->id, directions[robot->dir]);
	pthread_mutex_unlock(&console_mtx);
}

void push(Robot* robot)
{
	switch (robot->dir)
	{
		case NORTH:
			robotLoc[robot->id][1]++;
			boxLoc[robot->id][1]++;
			robot->dy_bd--;
			break;
		case WEST:
			robotLoc[robot->id][0]--;
			boxLoc[robot->id][0]--;
			robot->dx_bd++;
			break;
		case SOUTH:
			robotLoc[robot->id][1]--;
			boxLoc[robot->id][1]--;
			robot->dy_bd++;
			break;
		case EAST:
			robotLoc[robot->id][0]++;
			boxLoc[robot->id][0]++;
			robot->dx_bd--;
			break;
	}
	pthread_mutex_lock(&console_mtx);
	fprintf(fp, "robot %d push %c\n", robot->id, directions[robot->dir]);
	pthread_mutex_unlock(&console_mtx);
}

void end(Robot* robot)
{
	robot->isLive = 0;

	pthread_mutex_lock(&state_mtx);
	numLiveThreads--;
	pthread_mutex_unlock(&state_mtx);

	pthread_mutex_lock(&console_mtx);
	fprintf(fp, "robot %d end\n", robot->id);
	pthread_mutex_unlock(&console_mtx);
}

void tick(Robot* robot)
{
	if (robot->mode & MOVE)
	{
		if (robot->dx_rb == 0 && robot->dy_rb == 0)
		{
			if (robot->boxDir == WEST || robot->boxDir == EAST)
			{
				robot->dir = turn(X, robot->dx_bd);
				robot->mode = PUSH | X;
			}
			else if (robot->boxDir == NORTH || robot->boxDir == SOUTH)
			{
				robot->dir = turn(Y, robot->dy_bd);
				robot->mode = PUSH | Y;
			}
		}
		else if (robot->dx_rb == 0)
		{
			robot->dir = turn(Y, robot->dy_rb);
			robot->mode = MOVE | Y;
		}
		else if (robot->dy_rb == 0)
		{
			robot->dir = turn(X, robot->dx_rb);
			robot->mode = MOVE | X;
		}
	}
	else if (robot->mode & PUSH)
	{
		if (robot->dx_bd == 0 && robot->mode & X)
		{
			//	adjust...
			adjust(robot);
			robot->dir = turn(Y, robot->dy_rb);
			robot->mode = MOVE | Y;
		}
		else if (robot->dy_bd == 0 && robot->mode & Y)
		{
			// adjust...
			adjust(robot);
			robot->dir = turn(X, robot->dx_rb);
			robot->mode = MOVE | X;
		}
	}

	switch (robot->mode & (MOVE | PUSH))
	{
		case MOVE:
			move(robot);
			break;
		case PUSH:
			push(robot);
			break;
	}
}

void *start(void* data)
{
	Robot* robot = (Robot*) data;

	while (robot->isLive)
	{
		sleep(1);
		tick(robot);

		if (robot->dx_bd == 0 && robot->dy_bd == 0)
		{
			robot->mode = 0;
			end(robot);
		}	
	}

	return NULL;
}

int assignLoc(int objectCount, int entries, int padding, int **loc, int acquiredLoc[][2])
{
    for (int k=0; k<objectCount; k++)
    {
        int x;
        int y;
        //
        //  Making sure the robots/boxes/doors don't occupy an acquired space.
        int match = 1;
        while (match)
        {
            match = 0;
            x = rand() % (numCols - padding * 2) + padding;
            y = rand() % (numRows - padding * 2) + padding;
            for (int l=0; l<entries; l++)
            {
                if (x == acquiredLoc[l][0] && y == acquiredLoc[l][1])
                    match = 1;
            }
        }
        acquiredLoc[entries][0] = x;
        acquiredLoc[entries][1] = y;
        entries++;
        
        //
        loc[k] = (int*) malloc(sizeof(void*) * 2);
        loc[k][0] = x;  //  x
        loc[k][1] = y;  //  y
        //
        fprintf(fp, "{%d, %d}\n", x, y);
    }
    //
    fprintf(fp, "\n");
    
    return entries;
}


