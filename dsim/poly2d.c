#include <conio.h>
#include <stdio.h>
#include <dos.h>
#include <i86.h>
#include <string.h>
#include <malloc.h>
#include "polygon.h"
#include "vga.h"

#define DRAW_POLYGON(pointList,colour,x,y)                      \
    polygon.length = sizeof(pointList) / sizeof(struct Point);  \
    polygon.pointPtr = pointList;                               \
    FillConvexPolygon(&polygon, colour, x, y);

void main(void);
extern int FillConvexPolygon(struct PointListHeader *, int, int, int);

void main()
{
    int i, j;
    struct PointListHeader polygon;

    static struct Point screenRectangle[] = {{0,0},{320,0},{320,200},{0,200}};
    static struct Point convexShape[] = {{0,0},{121,0},{320,0},{200,51},{301,51},{250,51},{319,143},
                                        {320,200},{22,200},{0,200},{50,180},{20,160},{50,140},
                                        {20,120},{50,100},{20,80},{50,60},{20,40},{50,20}};
    static struct Point hexagon[] = {{90,-50},{0,-90},{-90,-50},{-90,50},{0,90},{90,50}};
    static struct Point triangle1[] = {{30,0},{15,20},{0,0}};
    static struct Point triangle2[] = {{30,20},{15,0},{0,20}};
    static struct Point triangle3[] = {{0,20},{20,10},{0,0}};
    static struct Point triangle4[] = {{20,20},{20,0},{0,10}};
    union REGS regs;

    // Create the backbuffer
    backBuffer = (char *)malloc(SCREEN_HEIGHT*SCREEN_WIDTH);

    if(backBuffer == NULL) {
        printf("Could not allocate memory for back buffer\n");
        return;
    }

    // Set VGA mode
    regs.w.ax = SCREEN_MODE;
    int386(VGA_INTERRUPT, &regs, &regs);

    // Clear to cyan
    DRAW_POLYGON(screenRectangle, 3, 0, 0);
    // Copy back buffer to screen
    memcpy((void *)SCREEN_ADDRESS, (void *)backBuffer, SCREEN_WIDTH * SCREEN_HEIGHT);

    getch();

    // Draw an irregular shape
    DRAW_POLYGON(convexShape, 6, 0, 0);
    // Copy back buffer to screen
    memcpy((void *)SCREEN_ADDRESS, (void *)backBuffer, SCREEN_WIDTH * SCREEN_HEIGHT);
    getch();

    // Draw adjacent triangles
    for(j=0; j<=80; j+=20) {
        for(i=0; i<290; i+=30) {
            DRAW_POLYGON(triangle1, 16+j, i, j);
            DRAW_POLYGON(triangle2, 32+j, i+15, j);
        }
    }

    
    for(j=100; j<=170; j+=20) {
        for(i=0; i<290; i+=20) {
            DRAW_POLYGON(triangle3, 48+i, i, j);
        }
        for(i=0; i<290; i+=20) {
            DRAW_POLYGON(triangle4, 64+i, i, j+10);
        }
    }

    // Copy back buffer to screen
    memcpy((void *)SCREEN_ADDRESS, (void *)backBuffer, SCREEN_WIDTH * SCREEN_HEIGHT);
    getch();

    // Draw a series of conectric hexagons
    for(i=0;i<16;i++) {
        DRAW_POLYGON(hexagon, i, 160, 100);
        for(j=0;j<sizeof(hexagon)/sizeof(struct Point); j++) {
            if(hexagon[j].x != 0) {
                hexagon[j].x -= hexagon[j].x >= 0 ? 3 : -3;
                hexagon[j].y -= hexagon[j].y >= 0 ? 2 : -2;
            } else {
                hexagon[j].y -= hexagon[j].y >= 0 ? 3 : -3;
            }
        }
    }

    // Copy back buffer to screen
    memcpy((void *)SCREEN_ADDRESS, (void *)backBuffer, SCREEN_WIDTH * SCREEN_HEIGHT);

    getch();

    // Return to text mode
    regs.w.ax = TEXT_MODE;
    int386(VGA_INTERRUPT, &regs, &regs);
}
