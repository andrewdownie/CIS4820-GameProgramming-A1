
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

typedef struct V3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct I3 {
    int x;
    int y;
    int z;
} Int3;

typedef enum _WallState{
    open,
    closed,
    opening,
    closing
} WallState;

typedef enum _Direction{
    north,
    south,
    east,
    west,
    none
} Direction;





typedef struct _Wall{
    float percentClosed;
    WallState state;
    Direction movementDirection;
} Wall;

typedef struct _Node{
    Wall *south;
    Wall *north;
    Wall *east;
    Wall *west;
} Node;
