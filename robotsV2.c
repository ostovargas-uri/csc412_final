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
assignLoc(int, int, int**);

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
const char* axis = "XY";
//
pthread_mutex_t output_mtx;
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
	{
		printf("./[prog] [number of rows] [number of columns] [number of boxes] [number of doors]\n");
        return 1;
	}

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
	{
		grid[i] = (int*) malloc(numCols * sizeof(int));
		for (int j=0; j<numCols; j++)
			grid[i][j] = 0;
	}
	
	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));
		
	pthread_mutex_init(&output_mtx, NULL);
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
    
    robotLoc = (int**) malloc(sizeof(void*) * numBoxes);
    boxLoc = (int**) malloc(sizeof(void*) * numBoxes);
    doorAssign = (int*) malloc(sizeof(int) * numBoxes);
    //
    doorLoc = (int**) malloc(sizeof(void*) * numDoors);

	assignLocation(numBoxes, 0, robotLoc);
	assignLocation(numBoxes, 1, boxLoc);
	assignLocation(numDoors, 0, doorLoc);
    
    for (int k=0; k<numBoxes; k++)
        doorAssign[k] = rand() % numDoors;

    //  TEST: Robot and Box X Aligned
	// robotLoc[0][0] = 1;
	// robotLoc[0][1] = 1;
	// boxLoc[0][0] = 2;
	// boxLoc[0][1] = 1;
	// doorLoc[0][0] = 0;
	// doorLoc[0][1] = 1;
    //	TEST: Robot and Box Y Aligned
	// robotLoc[0][0] = 1;
	// robotLoc[0][1] = 1;
	// boxLoc[0][0] = 1;
	// boxLoc[0][1] = 2;
	// doorLoc[0][0] = 1;
	// doorLoc[0][1] = 0;
	//	TEST: Box and Door X Aligned
	// robotLoc[0][0] = 0;
	// robotLoc[0][1] = 0;
	// boxLoc[0][0] = 3;
	// boxLoc[0][1] = 3;
	// doorLoc[0][0] = 2;
	// doorLoc[0][1] = 3;
	//	TEST: Box and Door Y Aligned
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
		//
		robot->id = k;
		robot->loc = robotLoc[k];
		robot->box.loc = boxLoc[k];
		//
		robot->isLive = 1;
		//
		setPath(robot);

		//  DEBUG
		// fprintf(stderr, "box %d direction: %d\n", k, robot->box.dir);
		// fprintf(stderr, "box %d trajected travel:\n\tx: %3d\n\ty: %3d\n", k, robot->box.xDistanceFromDoor, robot->box.yDistanceFromDoor);
		// fprintf(stderr, "boxLoc: {%d, %d}\n\n", boxLoc[k][0], boxLoc[k][1]);
		// fprintf(stderr, "robot %d direction: %d\n", k, robot->dir);
		// fprintf(stderr, "robot %d trajected travel:\n\tx: %3d\n\ty: %3d\n", k, robot->xDistanceFromBox, robot->yDistanceFromBox);
		// fprintf(stderr, "robotLoc: {%d, %d}\n", robotLoc[k][0], robotLoc[k][1]);
    
		pthread_create(thread + k, NULL, start, (void*) robot);
	}
}

void* start(void* data)
{
	Robot* robot = (Robot*) data;

	while (robot->isLive)
	{
		sleep(1);
		tick(robot);	
	}

	return NULL;
}

void turn(Robot* robot)
{
	//	PATH PLANNING PRECEDENCE
	//	PRECEDENCE I
	if (robot->xDistanceFromBox == 0 && robot->yDistanceFromBox == 0)
	{
		//	robot is behind a box, ready to push.
		robot->dir = robot->box.dir;
		switch (robot->dir)
		{
			case NORTH:
			case SOUTH:
				robot->axis = Y;
				break;
			case WEST:
			case EAST:
				robot->axis = X;
				break;
		}
		return;
	}

	//	PRECEDENCE II
	//	pick axis.
	switch (robot->box.dir)
	{
		case NORTH:
			if (robot->loc[Y] < robot->box.loc[Y])
				robot->axis = X;
			else
				robot->axis = Y;
			break;
		case SOUTH:
			if (robot->loc[Y] > robot->box.loc[Y])
				robot->axis = X;
			else
				robot->axis = Y;
			break;
		default:
			if (robot->xDistanceFromBox == 0)
				robot->axis = Y;
			else
				robot->axis = X;
			break;
	}
	
	//
	//	from axis, pick direction.
	switch (robot->axis)
	{
		case X:
			if (robot->xDistanceFromBox < 0)
				robot->dir = WEST;
			else if (robot->xDistanceFromBox > 0)
				robot->dir = EAST;
			break;
		case Y:
			if (robot->yDistanceFromBox < 0)
				robot->dir = SOUTH;
			else if (robot->yDistanceFromBox > 0)
				robot->dir = NORTH;
			break;
	}
	// fprintf(stderr, "turned to %c\n", directions[robot->dir]);
}

void setPath(Robot* robot)
{
	robot->mode = MOVE;
	
	//
	robot->box.xDistanceFromDoor = doorLoc[doorAssign[robot->id]][X] - robot->box.loc[X];
	robot->box.yDistanceFromDoor = doorLoc[doorAssign[robot->id]][Y] - robot->box.loc[Y];
	robot->xDistanceFromBox = robot->box.loc[X] - robot->loc[X];
	robot->yDistanceFromBox = robot->box.loc[Y] - robot->loc[Y];

	if (robot->box.xDistanceFromDoor < 0)
	{
		robot->box.dir = WEST;
		robot->xDistanceFromBox++;
	}
	else if (robot->box.xDistanceFromDoor > 0)
	{
		robot->box.dir = EAST;
		robot->xDistanceFromBox--;
	}
	else if (robot->box.yDistanceFromDoor < 0)
	{
		robot->box.dir = SOUTH;
		robot->yDistanceFromBox++;
	}
	else if (robot->box.yDistanceFromDoor > 0)
	{
		robot->box.dir = NORTH;
		robot->yDistanceFromBox--;
	}
	// fprintf("box dir set %c\n", directions[robot->box.dir]);
	turn(robot);
}

void move(Robot* robot)
{
	int x = robot->loc[X];
	int y = robot->loc[Y];
	//
	switch (robot->dir)
	{
		case NORTH:
			grid[x][y++] = 1;
			grid[x][y-1] = 0;
			robot->loc[Y] = y;
			robot->yDistanceFromBox--;

			break;
		case WEST:
			grid[x--][y] = 1;
			grid[x+1][y] = 0;
			robot->loc[X] = x;
			robot->xDistanceFromBox++;

			break;
		case SOUTH:
			grid[x][y--] = 1;
			grid[x][y+1] = 0;
			robot->loc[Y] = y;
			robot->yDistanceFromBox++;

			break;
		case EAST:
			grid[x++][y] = 1;
			grid[x-1][y] = 0;
			robot->loc[X] = x;
			robot->xDistanceFromBox--;

			break;
	}
	pthread_mutex_lock(&output_mtx);
	fprintf(fp, "robot %d move %c\n", robot->id, directions[robot->dir]);
	pthread_mutex_unlock(&output_mtx);
}

void push(Robot* robot)
{
	int robot_x = robot->loc[X];
	int robot_y = robot->loc[Y];
	int box_x = robot->box.loc[X];
	int box_y = robot->box.loc[Y];
	//
	switch (robot->dir)
	{
		case NORTH:
			grid[box_x][box_y++] = 1;	//	can we move to that grid space?
			grid[box_x][box_y-1] = 0;	//	yes, release current spot.
			//
			grid[robot_x][robot_y++] = 1;
			grid[robot_x][robot_y-1] = 0;
			//
			robot->loc[Y] = robot_y;
			robot->box.loc[Y] = box_y;
			robot->box.yDistanceFromDoor--;

			break;
		case WEST:
			grid[box_x--][box_y] = 1;
			grid[box_x+1][box_y] = 0;
			//
			grid[robot_x--][robot_y] = 1;
			grid[robot_x+1][robot_y] = 0;
			//
			robot->loc[X] = robot_x;
			robot->box.loc[X] = box_x;
			robot->box.xDistanceFromDoor++;

			break;
		case SOUTH:
			grid[box_x][box_y--] = 1;
			grid[box_x][box_y+1] = 0;
			//
			grid[robot_x][robot_y--] = 1;
			grid[robot_x][robot_y+1] = 0;
			//
			robot->loc[Y] = robot_y;
			robot->box.loc[Y] = box_y;
			robot->box.yDistanceFromDoor++;

			break;
		case EAST:
			grid[box_x++][box_y] = 1;
			grid[box_x-1][box_y] = 0;
			//
			grid[robot_x++][robot_y] = 1;
			grid[robot_x-1][robot_y] = 0;
			//
			robot->loc[X] = robot_x;
			robot->box.loc[X] = box_x;
			robot->box.xDistanceFromDoor--;

			break;
	}
	pthread_mutex_lock(&output_mtx);
	fprintf(fp, "robot %d push %c\n", robot->id, directions[robot->dir]);
	pthread_mutex_unlock(&output_mtx);
}

void end(Robot* robot)
{
	int robot_x = robot->loc[X];
	int robot_y = robot->loc[Y];
	int box_x = robot->box.loc[X];
	int box_y = robot->box.loc[Y];

	grid[robot_x][robot_y] = 0;
	grid[box_x][box_y] = 0;
	robot->isLive = 0;
	//
	pthread_mutex_lock(&state_mtx);
	numLiveThreads--;
	pthread_mutex_unlock(&state_mtx);

	pthread_mutex_lock(&output_mtx);
	fprintf(fp, "robot %d end\n", robot->id);
	pthread_mutex_unlock(&output_mtx);
}

void tick(Robot* robot)
{
	switch (robot->mode)
	{
		case MOVE:
			if (
				robot->xDistanceFromBox == 0 &&
				robot->yDistanceFromBox == 0
			)
			{
				// fprintf(stderr, "robot is positioned behind box\n");
				turn(robot);
				robot->mode = PUSH;
				push(robot);
				break;
			}
			else if (
				(robot->xDistanceFromBox == 0 && robot->axis == X) ||
				(robot->yDistanceFromBox == 0 && robot->axis == Y)
			)
			{
				// fprintf(stderr, "robot move axis complete: turning direction\n");
				turn(robot);
			}

			move(robot);
			break;
			//
		case PUSH:
			if (
				robot->box.xDistanceFromDoor == 0 &&
				robot->box.yDistanceFromDoor == 0
			)
			{
				// fprintf(stderr, "robot finished\n");
				end(robot);
				break;
			}
			else if (
				(robot->box.xDistanceFromDoor == 0 && robot->axis == X) ||
				(robot->box.yDistanceFromDoor == 0 && robot->axis == Y)
			)
			{
				// fprintf(stderr, "robot push axis complete: recalibrating direction\n");
				setPath(robot);
				//
				move(robot);
				break;
			}
			
			push(robot);
			break;
	}
}

void assignLocation(int object_count, int padding, int** loc)
{
    for (int k=0; k<object_count; k++)
    {
        int x;
        int y;
        //
        //  Making sure the robots/boxes/doors don't occupy an acquired space.
        int match = 1;
        while (match)
        {
            x = rand() % (numCols - padding * 2) + padding;
            y = rand() % (numRows - padding * 2) + padding;
            match = grid[x][y] == 1 ? 1 : 0;
        }
        
        //
        loc[k] = (int*) malloc(sizeof(void*) * 2);
        loc[k][0] = x;  //  x
        loc[k][1] = y;  //  y
		grid[x][y] = 1;
        //
		pthread_mutex_lock(&output_mtx);
        fprintf(fp, "{%d, %d}\n", x, y);
		pthread_mutex_unlock(&output_mtx);
    }
    //
	pthread_mutex_lock(&output_mtx);
    fprintf(fp, "\n");
	pthread_mutex_unlock(&output_mtx);
}


