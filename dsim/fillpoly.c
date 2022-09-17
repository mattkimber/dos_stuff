#include <stdio.h>
#include <math.h>
#include <malloc.h> // think this is correct for OpenWatcom
#include "polygon.h"

#define IDX_FORWARD(index) index = (index + 1) % vertexList->length;
#define IDX_BACKWARD(index) index = (index - 1 + vertexList->length) % vertexList->length;

#define IDX_MOVE(index, direction)                                      \
    if (direction > 0) index = (index + 1) % vertexList->length;        \
    else index = (index - 1 + vertexList->length) % vertexList->length;

extern void DrawLineList(struct LineList *, int);
static void ScanEdge(int, int, int, int, int, int, struct Line **);

int FillConvexPolygon(struct PointListHeader *vertexList, int colour, int xOffset, int yOffset)
{
    int i, minIndexL, maxIndex, minIndexR, skipFirst, temp;
    int minPointY, maxPointY, topIsFlat, leftEdgeDirection;
    int nextIndex, currentIndex, previousIndex;
    int deltaXN, deltaYN, deltaXP, deltaYP;
    struct LineList workingLineList;
    struct Line *edgePointPtr;
    struct Point *vertexPtr;

    // Set up vertexPtr to point to the vertex list
    vertexPtr = vertexList->pointPtr;

    // Find the top and bottom of the polygon
    if (vertexList->length == 0) return(1); // reject polygons with no vertices

    maxPointY = minPointY = vertexPtr[minIndexL = maxIndex = 0].y;
    for(i = 1; i < vertexList->length; i++) {
        if(vertexPtr[i].y < minPointY) {
            minPointY = vertexPtr[minIndexL = i].y; // new top
        } else if (vertexPtr[i].y > maxPointY) {
            maxPointY = vertexPtr[maxIndex = i].y; // new bottom
        }
    }

    if(minPointY == maxPointY) return(1); // zero-height polygon = degenerate

    // Scan in ascending order to find the last top-edge point#
    minIndexR = minIndexL;
    while(vertexPtr[minIndexR].y == minPointY) {
        IDX_FORWARD(minIndexR)
    }
    IDX_BACKWARD(minIndexR); // Back up to the last top-edge point

    // Now scan in descending order to find the first top-edge point
    while (vertexPtr[minIndexL].y == minPointY) {
        IDX_BACKWARD(minIndexL);
    }
    IDX_FORWARD(minIndexL); // Back up to the first top-edge point

    // Identify which direction through the vertex list is the left edge
    leftEdgeDirection = -1; // left edge runs down through the vertex list
    if ((topIsFlat = (vertexPtr[minIndexL].x != vertexPtr[minIndexR].x) ? 1 : 0) == 1) {
        // If the top is flat, we can just see which end is leftmost
        if (vertexPtr[minIndexL].x > vertexPtr[minIndexR].x) {
            leftEdgeDirection = 1;  // left edge runs up through vertex list
            temp = minIndexL;       // swap indices so minIndexL points to left edge
            minIndexL = minIndexR;
            minIndexR = temp;
        }
    } else {
        // Point to downward end of first line
        nextIndex = minIndexR;
        IDX_FORWARD(nextIndex);
        previousIndex = minIndexL;
        IDX_BACKWARD(previousIndex);

        // Calculate x/y lengths and use them to compare slopes
        deltaXN = vertexPtr[nextIndex].x - vertexPtr[minIndexL].x;
        deltaYN = vertexPtr[nextIndex].y - vertexPtr[minIndexL].y;
        deltaXP = vertexPtr[previousIndex].x - vertexPtr[minIndexL].x;
        deltaYP = vertexPtr[previousIndex].y - vertexPtr[minIndexL].y;

        if(((long)deltaXN * deltaYP - (long)deltaYN * deltaXP) < 0L) {
            leftEdgeDirection = 1;  // left edge runs up through vertex list
            temp = minIndexL;       // swap indices so minIndexL points to left edge
            minIndexL = minIndexR;
            minIndexR = temp;
        }
    }

    // Set the number of scan lines in the polygon
    // - Skip bottom edge
    // - Skip top edge if the top isn't flat
    // - Set the top scan line to draw (second line, unless top is flat)

    // Handle degenerate case (nothing to draw)
    if((workingLineList.length = maxPointY - minPointY - 1 + topIsFlat) <= 0) return(1);

    workingLineList.yStart = yOffset + minPointY + 1 - topIsFlat;

    // Allocate memory to store the line list
    if ((workingLineList.linesPtr = (struct Line *)(malloc(sizeof(struct Line) * workingLineList.length))) == NULL) {
        return(0); // couldn't allocate memory for line list
    }

    // Scan the left edge and store the boundary points in the list
    edgePointPtr = workingLineList.linesPtr;

    // Start from top of the left edge
    previousIndex = currentIndex = minIndexL;

    // Skip the first point of the first line unless top is flat - if not flat,
    // vertex is exactly on the right edge of the polygon and shouldn't be drawn
    skipFirst = topIsFlat ? 0 : 1;

    // Scan convert each line in the left edge from top to bottom
    do {
        IDX_MOVE(currentIndex, leftEdgeDirection);
        ScanEdge(vertexPtr[previousIndex].x + xOffset,
            vertexPtr[previousIndex].y,
            vertexPtr[currentIndex].x + xOffset,
            vertexPtr[currentIndex].y,
            1, skipFirst, &edgePointPtr);

        previousIndex = currentIndex;
        skipFirst = 0;  // skip first should always be 0 after first iteration

    } while (currentIndex != maxIndex);

    // Scan the right edge and store the boundary points in the list
    edgePointPtr = workingLineList.linesPtr;

    // Start from top of the left edge
    previousIndex = currentIndex = minIndexR;

    // Skip the first point of the first line unless top is flat - if not flat,
    // vertex is exactly on the right edge of the polygon and shouldn't be drawn
    skipFirst = topIsFlat ? 0 : 1;

    // Scan convert each line in the right edge from top to bottom, adjusting
    // coordinates by 1
    do {
        IDX_MOVE(currentIndex, -leftEdgeDirection);
        ScanEdge(vertexPtr[previousIndex].x + xOffset - 1,
            vertexPtr[previousIndex].y,
            vertexPtr[currentIndex].x + xOffset - 1,
            vertexPtr[currentIndex].y,
            0, skipFirst, &edgePointPtr);

        previousIndex = currentIndex;
        skipFirst = 0;  // skip first should always be 0 after first iteration

    } while (currentIndex != maxIndex);

    // Draw the scan-converted lines
    DrawLineList(&workingLineList, colour);

    // Free up the memory used for the working line list
    free(workingLineList.linesPtr);
    return(1);

}

// Scan converts edge from (x1,y1)->(x2,y2), not including the point at (x2,y2)
static void ScanEdge(int x1, int y1, int x2, int y2, int setXStart, int skipFirst, struct Line **edgePointPtr)
{
    int y, deltaX, deltaY;
    double inverseSlope;
    struct Line *workingEdgePointPtr;

    // Calculate x/y lengths of the line and inverse slope
    deltaX = x2 - x1;
    if ((deltaY = y2 - y1) <= 0) return; // this edge is 0 length

    inverseSlope = (double)deltaX / (double)deltaY;

    // Store the x coordinate of the pixel closest to but not left of line for each Y coord
    workingEdgePointPtr = *edgePointPtr; // avoid double deref
    for (y = y1 + skipFirst; y < y2; y++, workingEdgePointPtr++) {
        // Store x coord in appropriate edge list
        if (setXStart == 1) {
            workingEdgePointPtr->xStart = x1 + (int)(ceil((y-y1) * inverseSlope));
        } else {
            workingEdgePointPtr->xEnd = x1 + (int)(ceil((y-y1)*inverseSlope));
        }
    }
    *edgePointPtr = workingEdgePointPtr; // advance calling function's pointer
}
