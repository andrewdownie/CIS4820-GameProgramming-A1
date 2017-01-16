
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


///
/// Wall Settings
///
#define WALL_COLOUR 1
#define AUTO_CHANGE_WALLS 1
#define CHANGE_WALLS_TIME 1500
#define WALL_TOGGLE_PERCENT_CHANCE 10
#define WALL_SPAWN_PERCENT_CHANCE 40

///
/// Player Settings
///
#define GRAVITY_RATE 9.8f
#define GRAVITY_ENABLED 1
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
int lastCollisionTime;
int timeSinceLastUpdate;


///
///Extension forward declarations
///
void BuildWorld();
void ChangeWalls();
void OuterWalls();

///
/// Utility forward delcarations
///
int PercentChance(int chance);
int WalkablePiece(int x, int y, int z);
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
    //Viewport position values
    int oldIndexX, oldIndexY, oldIndexZ;
    int indexX, indexY, indexZ;
    int firstFloorBelowPlayer;

    float oldX, oldY, oldZ;
    float x, y, z;
    float deltaGravity;


    int previousPiece;
    int currentPiece;


    getOldViewPosition(&oldX, &oldY, &oldZ);
    getViewPosition(&x, &y, &z);

    ///
    /// PLAYER MOVEMENT: Collision with walls and floors
    ///
    oldIndexX = (int)oldX * -1;
    oldIndexY = (int)oldY * -1;
    oldIndexZ = (int)oldZ * -1;

    indexX = (int)x * -1;
    indexY = (int)y * -1;
    indexZ = (int)z * -1;

    previousPiece = WalkablePiece(oldIndexX, oldIndexY, oldIndexZ);
    currentPiece = WalkablePiece(indexX, indexY, indexZ);


    //Handle: camera moving down into blocks below
    if(currentPiece == NOT_WALKABLE){

        if(oldIndexY > indexY){
            y = (indexY + 1) * -1;
            indexY = (int)y * -1;
            currentPiece = WalkablePiece(indexX, indexY, indexZ);
        }
    }



    //Handle: camera moving sideways into walls
    if(currentPiece == NOT_WALKABLE){

        if(indexX != oldIndexX || indexZ != oldIndexZ){
            x = oldX;
            z = oldZ;

            //ADDED BELOW W/O testing///////////////////////////////////////////
            if(indexY > oldIndexY + 1){//Double height walls go here??
                printf("Gone up wall\n");
            }
            //TODO: if moved to its own fucntion, set the viewport position here
        }

    }

    //Handle: limiting the player to climbing walls of height one
    //if(y > oldY + 1){
    //    printf("you have climbed a wall of greater than one height, stop that.\n");
    //}

    ///
    /// GRAVITY: collision with walls and floors
    ///
    oldIndexX = indexX;
    oldIndexY = indexY;
    oldIndexZ = indexZ;
    oldX = x;
    oldY = y;
    oldZ = z;



    for(firstFloorBelowPlayer = indexY; firstFloorBelowPlayer > 0; firstFloorBelowPlayer--){

        if(WalkablePiece(indexX, firstFloorBelowPlayer, indexZ) == NOT_WALKABLE){
            break;
        }
    }

    firstFloorBelowPlayer = firstFloorBelowPlayer + PLAYER_HEIGHT;



    if(GRAVITY_ENABLED){
        deltaGravity = DeltaGravity(lastCollisionTime);
        if(firstFloorBelowPlayer >= (y * -1) - deltaGravity){
            y = firstFloorBelowPlayer * -1;
            printf("\t%f\n\n", y);
        }
        else{
            y = y + deltaGravity;
        }
    }

    indexY = (int)y * -1;
    oldIndexY = (int)oldY * -1;


    ///TRY TO DO DOUBLE HEIGHT WALLS HERE???????????????????????????????????????
    if(indexY > oldIndexY + 1){
        printf("Went up more than one block\n");
        y = oldY;
        x = oldX;
        z = oldZ;
    }






    setViewPosition(x, y, z);
    lastCollisionTime = glutGet(GLUT_ELAPSED_TIME);
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
        int currentTime  = glutGet(GLUT_ELAPSED_TIME);
        int deltaTime = currentTime - timeSinceLastUpdate;

        //printf("Delta time is: %d\n", deltaTime);

        /*float x, y, z;

        getViewPosition(&x, &y, &z);
        y = y + 1 * deltaTime / 1000;
        setViewPosition(x, y, z);*/


        timeSinceLastUpdate = glutGet(GLUT_ELAPSED_TIME);
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
       timeSinceLastUpdate = 0;
       lastCollisionTime = 0;

       BuildWorld();
       world[0][2][0] = 1;
       world[0][3][0] = 1;

       world[1][2][0] = 1;
       world[2][2][0] = 1;
       world[3][2][0] = 1;

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
///// Extension functions
/////

void BuildWorld(){
    int offset;
    int i, j, k;

    /* initialize world to empty */
    for(i=0; i<WORLDX; i++)
        for(j=0; j<WORLDY; j++)
            for(k=0; k<WORLDZ; k++)
                world[i][j][k] = 0;

    /* build a red platform */
    for(i=0; i<WORLDX; i++) {
        for(j=0; j<WORLDZ; j++) {
            world[i][0][j] = 3;
        }
    }

    OuterWalls();

    //Randomize walls running along the x axis
    for(i=0; i<WORLDX-2; i+=10){
        for(j=0; j<WORLDZ-2; j+=10){

            world[i][1][j] = 2;

            if(PercentChance(WALL_SPAWN_PERCENT_CHANCE)){
                for(offset = 1; offset < 10; offset++){
                    world[i + offset][1][j] = WALL_COLOUR;
                }
            }

        }
    }

    //Randomize walls running along the y axis
    for(i=0; i<WORLDX-2; i+=10){
        for(j=0; j<WORLDZ-2; j+=10){

            world[i][1][j] = 2;

            if(PercentChance(WALL_SPAWN_PERCENT_CHANCE)){
                for(offset = 1; offset < 10; offset++){
                    world[i][1][j + offset] = WALL_COLOUR;
                }
            }

        }
    }



}

void OuterWalls(){
    int i;

    /* blue box shows xy bounds of the world */
    for(i=0; i<WORLDX-1; i++) {
        world[i][1][0] = WALL_COLOUR;
        world[i][1][WORLDZ-1] = WALL_COLOUR;
    }
    for(i=0; i<WORLDZ-1; i++) {
        world[0][1][i] = WALL_COLOUR;
        world[WORLDX-1][1][i] = WALL_COLOUR;
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
///// Untility Functions
/////
//Generates a random boolean in the form of 0 and 1.
//Parm "percent": is the chance out of 100 that this function will return 1.
int PercentChance(int percent){
    int rnd;
    rnd = rand() % 100 + 1;

    if(percent > rnd){
        return 1;
    }
    return 0;
}

int WalkablePiece(int x, int y, int z){
    return world[x][y][z] == EMPTY_PIECE;
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
