#include <dos.h>
#include <string.h>
#include "polygon.h"
#include "vga.h"

static void DrawPixel(int, int, int);

void DrawLineList(struct LineList *lineListPtr, int colour)
{
    struct Line *linePtr;
    unsigned char *screenPtr;
    int length;
    int x, y;

    // Point to the x start/end descriptor for the first horizontal line
    linePtr = lineListPtr->linesPtr;

    // Initialise the screen pointer
    screenPtr = (char *)(SCREEN_ADDRESS + (lineListPtr->yStart * SCREEN_WIDTH));

    // Draw each line in turn
    length = lineListPtr->length;
    while(length-- > 0) {
        memset(screenPtr + linePtr->xStart, (unsigned char)colour, (linePtr->xEnd - linePtr->xStart + 1));
        screenPtr += SCREEN_WIDTH;
        linePtr++;
    }
}
