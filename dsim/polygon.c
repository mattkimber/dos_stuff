#include <conio.h>
#include <stdio.h>
#include <dos.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include "polygon.h"
#include "vga.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

char *backBuffer;

extern int FillConvexPolygon(struct PointListHeader *vertexList, int colour, int xOffset, int yOffset);

#define DRAW_POLYGON(pointList,pLength,colour,x,y)     \
    polygon.length = pLength;                          \
    polygon.pointPtr = pointList;                      \
    FillConvexPolygon(&polygon, colour, x, y);

void TransformAndProjectPoly(double transform[4][4], struct Point3 *poly, int polygonLength, int colour)
{
    int i;
    struct Point3 transformedPoly[MAX_POLY_LENGTH];
    struct Point projectedPoly[MAX_POLY_LENGTH];
    struct PointListHeader polygon;

    // Transform to view space, then project to screen
    for(i=0; i<polygonLength; i++) {
        // Transform to view space
        TransformVector(transform, (double *)&poly[i], (double *)&transformedPoly[i]);

        // Project x and y to the screen, rounding to nearest integer.
        // Note: positive y is up in view space, but down in screen space, so is multiplied by -1.
        projectedPoly[i].x = ((int) (transformedPoly[i].x/transformedPoly[i].z*PROJECTION_RATIO*(SCREEN_WIDTH/2.0)+0.5))+SCREEN_WIDTH/2;
        projectedPoly[i].y = ((int) (transformedPoly[i].y/transformedPoly[i].z*-1.0*PROJECTION_RATIO*(SCREEN_WIDTH/2.0)+0.5))+SCREEN_HEIGHT/2;
    }

    DRAW_POLYGON(projectedPoly, polygonLength, colour, 0, 0);
}

void TransformAndProjectPoints(double transform[4][4], struct Object *objectToTransform) {
    int i, numPoints = objectToTransform->numVertices;
    struct Point3 *points = objectToTransform->vertexList;
    struct Point3 *transformedPoints = objectToTransform->transformedVertexList;
    struct Point3 *projectedPoints = objectToTransform->projectedVertexList;
    struct Point *screenPoints = objectToTransform->screenVertexList;

    for(i=0; i<numPoints; i++, points++, transformedPoints++, projectedPoints++, screenPoints++) {
        // Transform to view space
        TransformVector(transform, (double *)points, (double *)transformedPoints);

        // Project points
        projectedPoints->x = transformedPoints->x / transformedPoints->z * PROJECTION_RATIO * (SCREEN_WIDTH / 2.0);
        projectedPoints->y = transformedPoints->y / transformedPoints->z * PROJECTION_RATIO * (SCREEN_WIDTH / 2.0);
        projectedPoints->z = transformedPoints->z;

        // Convert to screen space with negated Y and 0,0 on the screen centre
        screenPoints->x = ((int) floor(projectedPoints->x + 0.5)) + SCREEN_WIDTH / 2;
        screenPoints->y = (-((int) floor(projectedPoints->y + 0.5))) + SCREEN_HEIGHT / 2;

    }
}

void DrawVisibleFaces(struct Object *objectToTransform) {
    int i, j, numFaces = objectToTransform->numFaces, numVertices;
    int *vertexNumbersPtr;
    struct Face *facePtr = objectToTransform->faces;
    struct Point * screenPoints = objectToTransform->screenVertexList;
    long v1, v2, w1, w2;
    struct Point vertices[MAX_POLY_LENGTH];
    struct PointListHeader polygon;

    // Draw each face in turn
    for(i=0; i<numFaces; i++, facePtr++) {
        numVertices = facePtr->numVertices;

        // copy the vertices over
        for(j=0, vertexNumbersPtr=facePtr->vertices; j < numVertices; j++) {
            vertices[j] = screenPoints[*vertexNumbersPtr++];
        }

        // Calculate if this is a back face or not
        v1 = vertices[1].x - vertices[0].x;
        w1 = vertices[numVertices-1].x - vertices[0].x;
        v2 = vertices[1].y - vertices[0].y;
        w2 = vertices[numVertices-1].y - vertices[0].y;

        if((v1*w2 - v2*w1)>0) {
            DRAW_POLYGON(vertices, numVertices, facePtr->colour, 0, 0);
        }
    }
}

void main()
{
    int done = 0;
    double workingTransform[4][4];
    double xvel, yvel, zvel;
 
    static struct Point3 testPoly[] = {{-30,-15,0,1},{0,15,0,1},{10,-5,0,1}};

    static struct Point3 cubeVertices[] = {
        {15,15,15,1},
        {15,15,-15,1},
        {15,-15,15,1},
        {15,-15,-15,1},
        {-15,15,15,1},
        {-15,15,-15,1},
        {-15,-15,15,1},
        {-15,-15,-15,1}
    };

    static struct Point3 transformedCubeVertices[sizeof(cubeVertices)/sizeof(struct Point3)];
    static struct Point3 projectedCubeVertices[sizeof(cubeVertices)/sizeof(struct Point3)];
    static struct Point screenCubeVertices[sizeof(cubeVertices)/sizeof(struct Point3)];

    static int face1[] = {1,3,2,0};
    static int face2[] = {5,7,3,1};
    static int face3[] = {4,5,1,0};
    static int face4[] = {3,7,6,2};
    static int face5[] = {5,4,6,7};
    static int face6[] = {0,2,6,4};

    static struct Face cubeFaces[] = {
        {face1,4,28},{face2,4,27},{face3,4,26},{face4,4,25},{face5,4,24},{face6,4,23}
    };

    static struct Object cube = {
        sizeof(cubeVertices)/sizeof(struct Point3),
        cubeVertices, transformedCubeVertices, projectedCubeVertices, screenCubeVertices,
        sizeof(cubeFaces)/sizeof(struct Face), cubeFaces
    };
 
    #define TEST_POLY_LENGTH (sizeof(testPoly)/sizeof(struct Point3))

    // Static transforms
    static double polyWorldTransform[4][4] = {
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, -140.0},
        {0.0, 0.0, 0.0, 1.0}
    };

    static double worldViewTransform[4][4] = {
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0}
    };

    double rotation = M_PI / 60.0; // initial rotation = 3 degrees
    union REGS regs;
    struct PointListHeader polygon;

    // Create the backbuffer
    backBuffer = (char *)malloc(SCREEN_HEIGHT*SCREEN_WIDTH);

    if(backBuffer == NULL) {
        printf("Could not allocate memory for back buffer\n");
        return;
    }

    // Set VGA mode
    regs.w.ax = SCREEN_MODE;
    int386(VGA_INTERRUPT, &regs, &regs);

    do {
        // Modify the transform matrix with the current rotation
        polyWorldTransform[0][0] = polyWorldTransform[2][2] = cos(rotation);
        polyWorldTransform[2][0] = -(polyWorldTransform[0][2] = sin(rotation));

        // Concatenate poly and world transforms to the working transform
        ConcatenateTransforms(worldViewTransform, polyWorldTransform, workingTransform);

        // Transform and project the cube
        TransformAndProjectPoints(workingTransform, &cube);

        // Clear the screen
        memset(backBuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT);

        // Draw the cube
        DrawVisibleFaces(&cube);

        // Copy back buffer to screen
        memcpy((void *)SCREEN_ADDRESS, (void *)backBuffer, SCREEN_WIDTH * SCREEN_HEIGHT);

        // Rotate by 1 degree, wrapping to 2*pi as necessary
        if ((rotation += (M_PI/180.0)) >= (M_PI * 2)) rotation -= M_PI * 2;

        // Handle the translations
        polyWorldTransform[2][3] += zvel;
        if (polyWorldTransform[2][3] > -40.0) {
            polyWorldTransform[2][3] = -40.0;
        }
        polyWorldTransform[0][3] += xvel;
        polyWorldTransform[1][3] += yvel;

        xvel = xvel / 1.05;
        yvel = yvel / 1.05;
        zvel = zvel / 1.05;

        if(kbhit()) {
            switch(getch()) {
                case 0x1B: // Esc exits
                    done = 1;
                    break;
                case 'A': case 'a': // move closer if not already too close
                    zvel += 1.0;
                    break;
                case 'Z': case 'z': // move away
                    zvel -= 1.0;
                    break;
                case 0: // extended code
                    switch (getch()) {
                        case 0x4B: // left
                            xvel -= 1.0;
                            break;
                        case 0x4D: // right
                            xvel += 1.0;
                            break;
                        case 0x48: // up
                            yvel += 1.0;
                            break;
                        case 0x50: // down
                            yvel -= 1.0;
                            break;
                    }
                    break;
                default:
                    getch();
                    break;
            }
        }
    } while(!done);

    // Return to text mode
    regs.w.ax = TEXT_MODE;
    int386(VGA_INTERRUPT, &regs, &regs);
}

