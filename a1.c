
/* Derived from scene.c in the The OpenGL Programming Guide */
/* Keyboard and mouse rotation taken from Swiftless Tutorials #23 Part 2 */
/* http://www.swiftless.com/tutorials/opengl/camera2.html */

/* Frames per second code taken from : */
/* http://www.lighthouse3d.com/opengl/glut/index.php?fps */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "graphics.h"

#define FLOOR_COLOR 3

///
/// Wall Settings
///
#define WALL_TOGGLE_PERCENT_CHANCE 10
#define WALL_SPAWN_PERCENT_CHANCE 40
#define CHANGE_WALLS_TIME 1500
#define AUTO_CHANGE_WALLS 1
#define WALL_COLOUR 1
#define WALL_HEIGHT 2

#define WALL_COUNT_X 6
#define WALL_COUNT_Y 6
#define WALL_LENGTH 3

///
/// Player Settings
///
#define GRAVITY_ENABLED 1
#define GRAVITY_RATE 9.8f
#define PLAYER_HEIGHT 2


///
/// Collision Constants
///
#define NOT_WALKABLE 0
#define EMPTY_PIECE 0
#define WALKABLE 1

///
/// Delta Time
///
int lastGravityTime;

///
/// Nodes and Walls
///
Node nodes[WALL_COUNT_X][WALL_COUNT_Y];
Wall verticalWalls[WALL_COUNT_X - 1][WALL_COUNT_Y];
Wall horizontalWalls[WALL_COUNT_X][WALL_COUNT_Y - 1];



///
///Extension forward declarations
///
void SetupNodesWalls();
void ChangeWalls();
void BuildWorld();
void OuterWalls();

///
/// Utility forward delcarations
///
int WalkablePiece(int x, int y, int z);
int WalkablePiece_I3(Int3 xyz);
int PercentChance(int chance);

float Clamp(float value, float minVal, float maxVal);
float DeltaGravity(int timeSinceLastCollision);




/* mouse function called by GLUT when a button is pressed or released */
void mouse(int, int, int, int);

/* initialize graphics library */
extern void graphicsInit(int *, char **);

/* lighting control */
extern void setLightPosition(GLfloat, GLfloat, GLfloat);
extern GLfloat* getLightPosition();

/* viewpoint control */
extern void setViewPosition(float, float, float);
extern void getViewPosition(float *, float *, float *);
extern void getOldViewPosition(float *, float *, float *);
extern void setViewOrientation(float, float, float);
extern void getViewOrientation(float *, float *, float *);

/* add cube to display list so it will be drawn */
extern int addDisplayList(int, int, int);

/* mob controls */
extern void createMob(int, float, float, float, float);
extern void setMobPosition(int, float, float, float, float);
extern void hideMob(int);
extern void showMob(int);

/* player controls */
extern void createPlayer(int, float, float, float, float);
extern void setPlayerPosition(int, float, float, float, float);
extern void hidePlayer(int);
extern void showPlayer(int);

/* 2D drawing functions */
extern void  draw2Dline(int, int, int, int, int);
extern void  draw2Dbox(int, int, int, int);
extern void  draw2Dtriangle(int, int, int, int, int, int);
extern void  set2Dcolour(float []);


/* flag which is set to 1 when flying behaviour is desired */
extern int flycontrol;
/* flag used to indicate that the test world should be used */
extern int testWorld;
/* flag to print out frames per second */
extern int fps;
/* flag to indicate the space bar has been pressed */
extern int space;
/* flag indicates the program is a client when set = 1 */
extern int netClient;
/* flag indicates the program is a server when set = 1 */
extern int netServer;
/* size of the window in pixels */
extern int screenWidth, screenHeight;
/* flag indicates if map is to be printed */
extern int displayMap;

/* frustum corner coordinates, used for visibility determination  */
extern float corners[4][3];

/* determine which cubes are visible e.g. in view frustum */
extern void ExtractFrustum();
extern void tree(float, float, float, float, float, float, int);

/********* end of extern variable declarations **************/


/*** collisionResponse() ***/
/* -performs collision detection and response */
/*  sets new xyz  to position of the viewpoint after collision */
/* -can also be used to implement gravity by updating y position of vp*/
/* note that the world coordinates returned from getViewPosition()
will be the negative value of the array indices */
void collisionResponse() {
    ///
    /// Variables
    ///
    Vector3 curPos, oldPos;
    Int3 curIndex, oldIndex;

    int floorLevel;
    float deltaGravity = DeltaGravity(lastGravityTime);

    int previousPiece;
    int currentPiece;


    ///
    /// Initial Setup
    ///
    getOldViewPosition(&oldPos.x, &oldPos.y, &oldPos.z);
    getViewPosition(&curPos.x, &curPos.y, &curPos.z);

    oldIndex.x = (int)oldPos.x * -1;
    oldIndex.y = (int)oldPos.y * -1;
    oldIndex.z = (int)oldPos.z * -1;

    curIndex.x = (int)curPos.x * -1;
    curIndex.y = (int)curPos.y * -1;
    curIndex.z = (int)curPos.z * -1;

    previousPiece = WalkablePiece(oldIndex.x, oldIndex.y, oldIndex.z);
    currentPiece = WalkablePiece(curIndex.x, curIndex.y, curIndex.z);


    ///
    /// PLAYER MOVEMENT: Collision with walls and floors
    ///
    //Handle: camera moving down into blocks below
    if(currentPiece == NOT_WALKABLE){

        if(oldIndex.y > curIndex.y){
            curPos.y = (curIndex.y + 1) * -1;
            curIndex.y = (int)curPos.y * -1;
            currentPiece = WalkablePiece(curIndex.x, curIndex.y, curIndex.z);
        }

    }

    //Handle: camera moving sideways into walls
    if(currentPiece == NOT_WALKABLE){

        if(curIndex.x != oldIndex.x || curIndex.z != oldIndex.z){
            curPos.x = oldPos.x;
            curPos.z = oldPos.z;

            //TODO: if moved to its own fucntion, set the viewport position here
        }

    }


    ///
    /// GRAVITY: collision with walls and floors
    ///
    oldIndex.x = curIndex.x;
    oldIndex.y = curIndex.y;
    oldIndex.z = curIndex.z;
    oldPos.x = curPos.x;
    oldPos.y = curPos.y;
    oldPos.z = curPos.z;

    getViewPosition(&curPos.x, &curPos.y, &curPos.z);
    curIndex.x = (int)curPos.x * -1;
    curIndex.y = (int)curPos.y * -1;
    curIndex.z = (int)curPos.z * -1;


    if(GRAVITY_ENABLED){
        for(floorLevel = curIndex.y; floorLevel > 0; floorLevel--){

            if(WalkablePiece(curIndex.x, floorLevel, curIndex.z) == NOT_WALKABLE){
                break;
            }
        }
        floorLevel = floorLevel + PLAYER_HEIGHT;

        if(floorLevel >= (curPos.y * -1) - deltaGravity){
            curPos.y = floorLevel * -1;
        }
        else{
            curPos.y = curPos.y + deltaGravity;
        }

    }

    ///
    /// Prevent climbing walls higher than one block tall
    ///
    curIndex.y = (int)curPos.y * -1;
    oldIndex.y = (int)oldPos.y * -1;

    if(curIndex.y > oldIndex.y + 1){
        curPos.x = oldPos.x;
        curPos.y = oldPos.y;
        curPos.z = curPos.z;
    }


    ///
    /// Finish
    ///
    setViewPosition(curPos.x, curPos.y, curPos.z);
    lastGravityTime = glutGet(GLUT_ELAPSED_TIME);
}


/******* draw2D() *******/
/* draws 2D shapes on screen */
/* use the following functions: 			*/
/*	draw2Dline(int, int, int, int, int);		*/
/*	draw2Dbox(int, int, int, int);			*/
/*	draw2Dtriangle(int, int, int, int, int, int);	*/
/*	set2Dcolour(float []); 				*/
/* colour must be set before other functions are called	*/
void draw2D() {
    GLfloat green[] = {0.0, 0.5, 0.0, 0.5};
    GLfloat black[] = {0.0, 0.0, 0.0, 0.5};

    if (testWorld) {
        /* draw some sample 2d shapes */
        set2Dcolour(green);
        //    draw2Dline(0, 0, 500, 500, 15);
        //    draw2Dtriangle(0, 0, 200, 200, 0, 200);

        set2Dcolour(black);
        draw2Dbox(500, 380, 524, 388);
    } else {

        //DRAW GUI HERE ////////////////////////////////////////////////////////

    }

}


/*** update() ***/
/* background process, it is called when there are no other events */
/* -used to control animations and perform calculations while the  */
/*  system is running */
/* -gravity must also implemented here, duplicate collisionResponse */
void update() {
    int i, j, k;
    float *la;

    /* sample animation for the test world, don't remove this code */
    /* -demo of animating mobs */
    if (testWorld) {

        /* sample of rotation and positioning of mob */
        /* coordinates for mob 0 */
        static float mob0x = 50.0, mob0y = 25.0, mob0z = 52.0;
        static float mob0ry = 0.0;
        static int increasingmob0 = 1;
        /* coordinates for mob 1 */
        static float mob1x = 50.0, mob1y = 25.0, mob1z = 52.0;
        static float mob1ry = 0.0;
        static int increasingmob1 = 1;

        /* move mob 0 and rotate */
        /* set mob 0 position */
        setMobPosition(0, mob0x, mob0y, mob0z, mob0ry);

        /* move mob 0 in the x axis */
        if (increasingmob0 == 1)
        mob0x += 0.2;
        else
        mob0x -= 0.2;
        if (mob0x > 50) increasingmob0 = 0;
        if (mob0x < 30) increasingmob0 = 1;

        /* rotate mob 0 around the y axis */
        mob0ry += 1.0;
        if (mob0ry > 360.0) mob0ry -= 360.0;

        /* move mob 1 and rotate */
        setMobPosition(1, mob1x, mob1y, mob1z, mob1ry);

        /* move mob 1 in the z axis */
        /* when mob is moving away it is visible, when moving back it */
        /* is hidden */
        if (increasingmob1 == 1) {
            mob1z += 0.2;
            showMob(1);
        } else {
            mob1z -= 0.2;
            hideMob(1);
        }
        if (mob1z > 72) increasingmob1 = 0;
        if (mob1z < 52) increasingmob1 = 1;

        /* rotate mob 1 around the y axis */
        mob1ry += 1.0;
        if (mob1ry > 360.0) mob1ry -= 360.0;
        /* end testworld animation */




    } else {//////////////////////////// LOGIC GOES HERE


        collisionResponse();


    }
}


/* called by GLUT when a mouse button is pressed or released */
/* -button indicates which button was pressed or released */
/* -state indicates a button down or button up event */
/* -x,y are the screen coordinates when the mouse is pressed or */
/*  released */
void mouse(int button, int state, int x, int y) {

    if (button == GLUT_LEFT_BUTTON)
    printf("left button - ");
    else if (button == GLUT_MIDDLE_BUTTON)
    printf("middle button - ");
    else
    printf("right button - ");

    if (state == GLUT_UP)
    printf("up - ");
    else
    printf("down - ");

    printf("%d %d\n", x, y);
}



int main(int argc, char** argv)
{
    int i, j, k;
    /* initialize the graphics system */
    graphicsInit(&argc, argv);

    /* the first part of this if statement builds a sample */
    /* world which will be used for testing */
    /* DO NOT remove this code. */
    /* Put your code in the else statment below */
    /* The testworld is only guaranteed to work with a world of
    with dimensions of 100,50,100. */
    if (testWorld == 1) {
        /* initialize world to empty */
        for(i=0; i<WORLDX; i++)
        for(j=0; j<WORLDY; j++)
        for(k=0; k<WORLDZ; k++)
        world[i][j][k] = 0;

        /* some sample objects */
        /* build a red platform */
        for(i=0; i<WORLDX; i++) {
            for(j=0; j<WORLDZ; j++) {
                world[i][24][j] = 3;
            }
        }
        /* create some green and blue cubes */
        world[50][25][50] = 1;
        world[49][25][50] = 1;
        world[49][26][50] = 1;
        world[52][25][52] = 2;
        world[52][26][52] = 2;

        /* blue box shows xy bounds of the world */
        for(i=0; i<WORLDX-1; i++) {
            world[i][25][0] = 2;
            world[i][25][WORLDZ-1] = 2;
        }
        for(i=0; i<WORLDZ-1; i++) {
            world[0][25][i] = 2;
            world[WORLDX-1][25][i] = 2;
        }

        /* create two sample mobs */
        /* these are animated in the update() function */
        createMob(0, 50.0, 25.0, 52.0, 0.0);
        createMob(1, 50.0, 25.0, 52.0, 0.0);

        /* create sample player */
        createPlayer(0, 52.0, 27.0, 52.0, 0.0);

    } else {

        ///
        /// initialize random
        ///
        srand((unsigned) time(NULL));

        ///
        /// Set lastUpdateTime to zero
        ///
        lastGravityTime = 0;

        BuildWorld();
        world[0][2][1] = 5;
        world[0][3][1] = 5;

        world[1][2][1] = 5;
        world[2][2][1] = 5;
        world[3][2][1] = 5;
        world[3][1][2] = 5;

        if(AUTO_CHANGE_WALLS == 1){
        //    glutTimerFunc(CHANGE_WALLS_TIME, ChangeWalls, CHANGE_WALLS_TIME);
        }



        /* create sample player */
        createPlayer(0, 52.0, 1.0, 52.0, 0.0);


    }


    /* starts the graphics processing loop */
    /* code after this will not run until the program exits */
    glutMainLoop();
    return 0;
}



/////
///// World Building Functions -------------------------------------------------
/////

void BuildWorld(){
    int offset;
    int i, j, k;

    /* initialize world to empty */
    for(i=0; i<WORLDX; i++){
        for(j=0; j<WORLDY; j++){
            for(k=0; k<WORLDZ; k++){
                world[i][j][k] = 0;
            }
        }
    }


    /* build the floor*/
    for(i=0; i<WORLDX; i++) {
        for(j=0; j<WORLDZ; j++) {
            world[i][0][j] = FLOOR_COLOR;
        }
    }

    OuterWalls();
    SetupNodesWalls();




}

void SetupNodesWalls(){
    int x, y;

    ///
    /// Clear Horizontal Walls
    ///
    for(x = 0; x < WALL_COUNT_X; x++){
        for(y = 0; y < WALL_COUNT_Y - 1; y++){
            horizontalWalls[x][y].movementDirection = none;
            horizontalWalls[x][y].percentClosed = 0;
            horizontalWalls[x][y].state = open;
            horizontalWalls[x][y].x = WALL_LENGTH;
            horizontalWalls[x][y].y = x * WALL_LENGTH - 1;//TODO: is this right?
        }
    }

    ///
    /// Clear Vertical Walls
    ///
    for(x = 0; x < WALL_COUNT_X - 1; x++){
        for(y = 0; y < WALL_COUNT_Y; y++){
            verticalWalls[x][y].movementDirection = none;
            verticalWalls[x][y].percentClosed = 0;
            verticalWalls[x][y].state = open;
            verticalWalls[x][y].x = y * WALL_LENGTH - 1;
            verticalWalls[x][y].y = WALL_LENGTH;
        }
    }

    ///
    /// Clear Nodes
    ///
    for(x = 0; x < WALL_COUNT_X; x++){
        for(y = 0; y < WALL_COUNT_Y; y++){
            nodes[x][y].north = &verticalWalls[x][y];
            nodes[x][y].west = &horizontalWalls[x][y];

            if(x > 0){
                nodes[x][y].west = &horizontalWalls[x + 1][y];
            }

            if(y > 0){
                nodes[x][y].south = &horizontalWalls[x][y + 1];
            }
        }
    }
}

void OuterWalls(){
    int i, height;

    for(height = 0; height < WALL_HEIGHT; height++){
        /* blue box shows xy bounds of the world */
        for(i=0; i<WORLDX-1; i++) {
            world[i][1 + height][0] = WALL_COLOUR;
            world[i][1 + height][WORLDZ-1] = WALL_COLOUR;
        }
        for(i=0; i<WORLDZ-1; i++) {
            world[0][1 + height][i] = WALL_COLOUR;
            world[WORLDX-1][1 + height][i] = WALL_COLOUR;
        }
    }

}


void ChangeWalls(int temp){
    int i, j, offset;

    if(AUTO_CHANGE_WALLS == 1){
        glutTimerFunc(CHANGE_WALLS_TIME, ChangeWalls, CHANGE_WALLS_TIME);
    }


    //Randomize walls running along the x axis
    for(i=10; i<WORLDX-1; i+=10){
        for(j=10; j<WORLDZ-1; j+=10){

            int toggleColour;

            if(PercentChance(WALL_TOGGLE_PERCENT_CHANCE) == 0){
                continue;
            }

            if(world[i + 1][1][j] == 0){
                toggleColour = WALL_COLOUR;
            }
            else{
                toggleColour = 0;
            }


            for(offset = 1; offset < 10; offset++){
                world[i + offset][1][j] = toggleColour;
            }


        }
    }

    //Randomize walls running along the y axis
    for(i=10; i<WORLDX-1; i+=10){
        for(j=10; j<WORLDZ-1; j+=10){

            int toggleColour;

            if(PercentChance(WALL_TOGGLE_PERCENT_CHANCE) == 0){
                continue;
            }

            if(world[i][1][j + 1] == 0){
                toggleColour = WALL_COLOUR;
            }
            else{
                toggleColour = 0;
            }


            for(offset = 1; offset < 10; offset++){
                world[i][1][j + offset] = toggleColour;
            }


        }
    }

    OuterWalls();



}


/////
///// Utility Functions --------------------------------------------------------
/////

//Generates a random boolean in the form of 0 and 1.
//Parameter "percent": is the chance out of 100 that this function will return 1.
int PercentChance(int percent){
    int rnd;
    rnd = rand() % 100 + 1;

    if(percent > rnd){
        return 1;
    }
    return 0;
}

int WalkablePiece(int x, int y, int z){
    int count = 0, height;

    for(height = 0; height < PLAYER_HEIGHT; height++){
        if(world[x][y + height][z] != EMPTY_PIECE){
            count++;
        }
    }

    return count == 0;
}

int WalkablePiece_I3(Int3 xyz){
    int count = 0, height;

    for(height = 0; height < PLAYER_HEIGHT; height++){
        if(world[xyz.x][xyz.y + height][xyz.z] != EMPTY_PIECE){
            count++;
        }
    }

    return count == 0;
}


float Clamp(float value, float minVal, float maxVal){
    if(value < minVal){
        return minVal;
    }
    else if(value > maxVal){
        return maxVal;
    }

    return value;
}

float DeltaGravity(int lastCollisionTime){
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int deltaTime = currentTime - lastCollisionTime;
    return GRAVITY_RATE * deltaTime / 1000;
}
