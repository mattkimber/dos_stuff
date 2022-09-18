#include <dos.h>
#include <string.h>
#include "polygon.h"
#include "vga.h"

void DrawLineList(struct LineList *lineListPtr, int colour)
{
    struct Line *linePtr;
    unsigned char *screenPtr;
    int length;

    // Point to the x start/end descriptor for the first horizontal line
    linePtr = lineListPtr->linesPtr;

    // Initialise the screen pointer
    screenPtr = backBuffer + (lineListPtr->yStart * SCREEN_WIDTH);

    // Draw each line in turn
    length = lineListPtr->length;

    // y clipping
    if(lineListPtr->yStart + length >= SCREEN_HEIGHT) {
        length = SCREEN_HEIGHT - lineListPtr->yStart;
    }

    if(lineListPtr->yStart < 0) {
        length -= -(lineListPtr->yStart);
        linePtr += -(lineListPtr->yStart);
        screenPtr += (SCREEN_WIDTH * -(lineListPtr->yStart));
    }

    while(length-- > 0) {
        if(linePtr->xEnd > 0 && linePtr->xStart < SCREEN_WIDTH) {
            if(linePtr->xStart < 0) linePtr->xStart = 0;
            if(linePtr->xEnd >= SCREEN_WIDTH) linePtr->xEnd = SCREEN_WIDTH - 1;
            memset(screenPtr + linePtr->xStart, (unsigned char)colour, (linePtr->xEnd - linePtr->xStart + 1));
        }
        
        screenPtr += SCREEN_WIDTH;
        linePtr++;
    }
}
