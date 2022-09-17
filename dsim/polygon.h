struct Point {
    int x;
    int y;
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
