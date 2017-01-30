#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#elif _WIN32
#include <windows.h>
#include <GL/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

/* world size and storage array */
#define WORLDX 100
#define WORLDY 50
#define WORLDZ 100
GLubyte  world[WORLDX][WORLDY][WORLDZ];

#define MAX_DISPLAY_LIST 500000

typedef enum _WallState{
    open,
    closed,
    opening,
    closing
} WallState;


typedef struct _Wall{
    WallState state;
    int x, z;
} Wall;

typedef struct _Pillar{
    //Wall *north, *south, *east, *west;
    //Pillar *pillar[4];

    Wall *wall[4];
} Pillar;

typedef struct _GenerationInfo{
    float spawnChanceModifier;
    float spawnChance;

    int creationAttempts;
    int wallsCreated;

} GenerationInfo;
