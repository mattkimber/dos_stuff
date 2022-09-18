void TransformVector(double transform[4][4], double *source, double *dest)
{
    int i, j;

    for(i=0;i<4;i++) {
        dest[i] = 0;
        for(j=0;j<4;j++) {
            dest[i] += transform[i][j] * source[j];
        }
    }
}

void ConcatenateTransforms(double a[4][4], double b[4][4], double dest[4][4])
{
    int i, j, k;

    for(i=0;i<4;i++) {
        for(j=0;j<4;j++) {
            dest[i][j] = 0;
            for(k=0;k<4;k++) {
                dest[i][j] += a[i][k] * b [k][j];
            }
        }
    }
}
