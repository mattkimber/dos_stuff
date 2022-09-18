#define MAX_POLY_LENGTH 4
#define PROJECTION_RATIO -2.0

struct Point {
    int x;
    int y;
};

struct Point3 {
    double x;
    double y;
    double z;
    double w;
};

struct PointListHeader {
    int length;
    struct Point *pointPtr;
};

struct Line {
    int xStart;
    int xEnd;
};

struct LineList {
    int length;
    int yStart;
    struct Line *linesPtr;
};

struct Face {
    int * vertices;
    int numVertices;
    int colour;
};

struct Object {
    int numVertices;
    struct Point3 *vertexList;
    struct Point3 *transformedVertexList;
    struct Point3 *projectedVertexList;
    struct Point *screenVertexList;
    int numFaces;
    struct Face *faces;
};

// Function definitions
extern void TransformVector(double transform[4][4], double *source, double *dest);
extern void ConcatenateTransforms(double a[4][4], double b[4][4], double dest[4][4]);
