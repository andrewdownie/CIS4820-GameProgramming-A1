
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
#include <stdbool.h>

#include "graphics.h"



///
/// Wall and floor settings
///
#define CHANGE_WALLS_TIME 3000
#define AUTO_CHANGE_WALLS 1
#define TARGET_WALL_COUNT 25
#define MAX_WALL_COUNT 25

#define OUTER_WALL_COLOR 7
#define WALL_COLOUR 1
#define NODE_COLOUR 2
#define FLOOR_COLOR 3

#define WALL_COUNT_X 6
#define WALL_COUNT_Z 6
#define WALL_HEIGHT 2
#define WALL_LENGTH 5



///
/// Runtime Generated "Constants"
///
int MAP_SIZE_X;
int MAP_SIZE_Z;
int totalWalls;

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
Node nodes[WALL_COUNT_X - 1][WALL_COUNT_Z - 1];



///
///Extension forward declarations
///
void SetupWalls();
void BuildWorldShell();
void PlaceVerticalWall(Wall *wall, int wallX, int wallZ);
void PlaceHorizontalWall(Wall *wall, int wallX, int wallZ);
void PlaceWalls();

void ChangeWalls();
int Node_WallCount();
int CountAllWalls();
void SetupWall(Wall **targetWall, Wall **adjacentWall, GenerationInfo *genInfo);


///
/// Utility forward delcarations
///
int WalkablePiece(int x, int y, int z);
int WalkablePiece_I3(Int3 xyz);
int PercentChance(float chance);

float Clamp(float value, float minVal, float maxVal);
float DeltaGravity(int timeSinceLastCollision);
void PrintWallGeneration();




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

        MAP_SIZE_X = (WALL_COUNT_X * WALL_LENGTH) + WALL_COUNT_X + 2;
        MAP_SIZE_Z = (WALL_COUNT_Z * WALL_LENGTH) + WALL_COUNT_Z + 2;

        BuildWorldShell();
        SetupWalls();
        PrintWallGeneration();
        PlaceWalls();

        printf("Wall count: %d\n", CountAllWalls());


        ///Setup some cubes to climb up for testing
        world[0][2][1] = 5;
        world[0][3][1] = 5;

        world[1][2][1] = 5;
        world[2][2][1] = 5;
        world[3][2][1] = 5;
        world[3][1][2] = 5;
        world[2][3][3] = 5;

        world[2][3][2] = 5;



        if(AUTO_CHANGE_WALLS == 1){
            glutTimerFunc(CHANGE_WALLS_TIME, ChangeWalls, CHANGE_WALLS_TIME);
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

void BuildWorldShell(){
    int x, y, z;
    int height;

    ///
    /// initialize world to empty
    ///
    for(x=0; x<WORLDX; x++){
        for(y=0; y<WORLDY; y++){
            for(z=0; z<WORLDZ; z++){
                world[x][y][z] = 0;
            }
        }
    }

    ///
    /// build the floor
    ///
    for(x=0; x < MAP_SIZE_X - 1; x++) {
        for(z=0; z < MAP_SIZE_Z - 1; z++) {
            world[x][0][z] = FLOOR_COLOR;
        }
    }

    ///
    /// build the outer walls
    ///
    for(height = 0; height < WALL_HEIGHT; height++){

        for(x=0; x < MAP_SIZE_X - 1; x++) {
            world[x][1 + height][0] = OUTER_WALL_COLOR;
            world[x][1 + height][MAP_SIZE_Z-2] = OUTER_WALL_COLOR;
        }

        for(z=0; z < MAP_SIZE_Z - 1; z++) {
            world[0][1 + height][z] = OUTER_WALL_COLOR;
            world[MAP_SIZE_X-2][1 + height][z] = OUTER_WALL_COLOR;
        }

    }

    ///
    /// Create the nodes
    ///
    for(x = 1; x < WALL_COUNT_X; x++){
        for(z = 1; z < WALL_COUNT_Z; z++){

            for(height = 0; height < WALL_HEIGHT; height++){
                world[x * (WALL_LENGTH + 1)][1 + height][z * (WALL_LENGTH + 1)] = NODE_COLOUR;
            }

        }
    }

}



void SetupWalls(){
    int wallCount;
    int x, z;

    Wall *targetWall, *adjacentWall;
    GenerationInfo genInfo;


    genInfo.spawnChanceModifier = 0;
    genInfo.spawnChance = 0;

    genInfo.creationAttempts = 0;
    genInfo.wallsCreated = 0;

    wallCount = ((WALL_COUNT_X - 1) * WALL_COUNT_Z) + (WALL_COUNT_X * (WALL_COUNT_Z - 1));
    genInfo.spawnChance = (TARGET_WALL_COUNT * 100) / wallCount;
    //printf("Spawn chance is: %f\n", genInfo.spawnChanceModifier);


    ///
    /// Set nodes to null
    ///
    for(x = 0; x < WALL_COUNT_X; x++){
        for(z = 0; z < WALL_COUNT_Z; z++){
            nodes[x][z].north = NULL;
            nodes[x][z].south = NULL;
            nodes[x][z].east = NULL;
            nodes[x][z].west = NULL;
        }
    }






    for(x = 0; x < WALL_COUNT_X - 1; x++){//DO I NEED TO MINUS ONE??? (there is one less node than wall) -> seems to work...
        for(z = 0; z < WALL_COUNT_Z - 1; z++){

            ///
            /// North wall
            ///
            if(z > 0){
                SetupWall( &(nodes[x][z].north), &(nodes[x][z - 1].south), &genInfo);
            }
            else{
                SetupWall( &(nodes[x][z].north), NULL, &genInfo);
            }


            ///
            /// South wall
            ///
            if(z < WALL_COUNT_Z - 1){
                SetupWall( &(nodes[x][z].south), &(nodes[x][z + 1].north), &genInfo);
            }
            else{
                SetupWall( &(nodes[x][z].south), NULL, &genInfo);
            }


            ///
            /// East Wall
            ///
            if(x < WALL_COUNT_X - 1){
                SetupWall( &(nodes[x][z].east), &(nodes[x + 1][z].west), &genInfo);
            }
            else{
                SetupWall( &(nodes[x][z].east), NULL, &genInfo);
            }


            ///
            /// West Wall
            ///
            if(x > 0){
                SetupWall( &(nodes[x][z].west), &(nodes[x - 1][z].east), &genInfo);
            }
            else{
                SetupWall( &(nodes[x][z].west), NULL, &genInfo);
            }


        }



    }

}

void PlaceVerticalWall(Wall *wall, int wallX, int wallZ){
    int actualWallLength;
    int yOffset, z;



    actualWallLength = (WALL_LENGTH * wall->percentClosed) / 100;
    for(z = 0; z < actualWallLength; z++){

        for(yOffset = 0; yOffset < WALL_HEIGHT; yOffset++){
            world[wallX][1 + yOffset][wallZ + z] = WALL_COLOUR;
        }

    }

}

void PlaceHorizontalWall(Wall *wall, int wallX, int wallZ){
    int actualWallLength;
    int yOffset, x;

    actualWallLength = (WALL_LENGTH * wall->percentClosed) / 100;
    for(x = 0; x < actualWallLength; x++){

        for(yOffset = 0; yOffset < WALL_HEIGHT; yOffset++){
            world[wallX + x][1 + yOffset][wallZ] = WALL_COLOUR;
        }

    }

}

void PlaceWalls(){
    int x, z;

    //printf("PLACE WALLS\n");

    for(x = 0; x < WALL_COUNT_X - 1; x++){
        for(z = 0; z < WALL_COUNT_Z - 1; z++){
            if(x == 0){
                PlaceHorizontalWall(nodes[x][z].west, 1, (WALL_LENGTH + 1) * (z + 1));
            }
            if(z == 0){
                PlaceVerticalWall(nodes[x][z].north, (WALL_LENGTH + 1) * (x + 1), 1);
            }

            PlaceHorizontalWall(nodes[x][z].east, (x + 1) * (WALL_LENGTH + 1) + 1, (WALL_LENGTH + 1) * (z + 1));
            PlaceVerticalWall(nodes[x][z].south, (WALL_LENGTH + 1) * (x + 1), (z + 1) * (WALL_LENGTH + 1) + 1);
        }
    }


}


void ChangeWalls(){
    int randomNode;
    int nodeCount;
    int randX, randZ;
    int x, z;

    Node *currentNode;

    bool walls[4];

    int i;




    ///
    /// Pick a random node
    ///
    nodeCount = (WALL_COUNT_X - 1) * (WALL_COUNT_Z - 1);
    randomNode = rand() % nodeCount + 1;

    randX = randomNode / WALL_COUNT_X;
    randZ = randomNode % WALL_COUNT_X;

    currentNode = nodes[randX, randZ];

    ///
    /// Pick a random open wall, and closed wall from that node
    ///
    walls[0] = currentNode->north != NULL;
    walls[1] = currentNode->east != NULL;
    walls[2] = currentNode->south != NULL;
    walls[3] = currentNode->west != NULL;









    PlaceWalls();
    glutTimerFunc(CHANGE_WALLS_TIME, ChangeWalls, CHANGE_WALLS_TIME);
}

int Node_WallCount(Node *node){
    int count = 0;

    if(node->north != NULL){
        count++;
    }

    if(node->east != NULL){
        count++;
    }

    if(node->south != NULL){
        count++;
    }

    if(node->west != NULL){
        count++;
    }


    return count;
}



int CountAllWalls(){
    int count = 0, x, z;
    Node *currentNode;

    for(x = 0; x < WALL_COUNT_X - 1; x++){
        for(z = 0; z < WALL_COUNT_Z - 1; z++){
            currentNode = &(nodes[x][z]);

            if(x == 0 && currentNode->west->state == closed){
                count++;
            }

            if(z == 0 && currentNode->north->state == closed){
                count++;
            }

            if(currentNode->east->state == closed){
                count++;
            }

            if(currentNode->south->state == closed){
                count++;
            }
        }
    }

    return count;
}



void SetupWall(Wall **targetWall, Wall **adjacentWall, GenerationInfo *genInfo){
    float currentSpawnPercentage;

    ///
    /// Error checking
    ///
    if(targetWall == NULL){
        printf("ERROR!: pointer to targetWall-pointer was null\n");
        return;
    }

    ///
    /// Make sure we're not going to spawn a wall where a wall exists already
    ///
    if(*targetWall != NULL){
        return;
    }






    ///
    /// Malloc the new wall
    ///
    Wall *newWall = (Wall*)malloc(sizeof(Wall));
    newWall->direction = none;
    genInfo->creationAttempts++;


    ///
    /// Randomly decide if the wall should be open or closed
    ///
    if(PercentChance(genInfo->spawnChance + genInfo->spawnChanceModifier) && genInfo->wallsCreated <= MAX_WALL_COUNT){
        newWall->percentClosed = 100;
        newWall->state = closed;
        genInfo->wallsCreated++;

        //printf("Walls created %d\n", genInfo->wallsCreated);
    }
    else{
        newWall->percentClosed = 0;
        newWall->state = open;
    }

    ///
    /// Use the spawnChanceModifier to push the spawnChance
    ///         towards spawning the target number of walls
    currentSpawnPercentage = (100 * ((float)genInfo->wallsCreated / (float)genInfo->creationAttempts));
    genInfo->spawnChanceModifier = genInfo->spawnChance - currentSpawnPercentage;

    //printf("SPAWN CHANCE MODIFIER: %f, %f\n", currentSpawnPercentage, genInfo->spawnChanceModifier);




    ///
    /// Assign the new wall
    ///
    *targetWall = newWall;

    if(adjacentWall != NULL){
        *adjacentWall = newWall;
    }
}




/////
///// Utility Functions --------------------------------------------------------
/////

//Generates a random boolean in the form of 0 and 1.
//Parameter "percent": is the chance out of 100 that this function will return 1.
int PercentChance(float percent){
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


void PrintWallGeneration(){
    int x, z;

    ///
    /// This is for debugging purposes only
    ///
    printf("DEBUGGING INFO FOR WALL GENERATION\n");
    for(z = 0; z < WALL_COUNT_Z - 1; z++){

        for(x = 0; x < WALL_COUNT_X - 1; x++){
            putchar(' ');


            if(nodes[x][z].north->state == closed){
                putchar('|');
            }
            else{
                putchar(' ');
            }
            putchar(' ');
        }
        putchar('\n');

        for(x = 0; x < WALL_COUNT_X - 1; x++){
            if(nodes[x][z].west->state == closed){
                putchar('-');
            }
            else{
                putchar(' ');
            }
            putchar('+');
            if(nodes[x][z].east->state == closed){
                putchar('-');
            }
            else{
                putchar(' ');
            }
        }
        putchar('\n');

        for(x = 0; x < WALL_COUNT_X - 1; x++){
            putchar(' ');
            if(nodes[x][z].south->state == closed){
                putchar('|');
            }
            else{
                putchar(' ');
            }
            putchar(' ');
        }
        putchar('\n');
    }

}
