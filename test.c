#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define QDCMEM
#define QDCEVIL
// #define QDCXFIRST

#define QDCX(v) (context->v[x])
#define QDCX1(v) (context->v[x1])
#define QDCX2(v) (context->v[x2])
#define QDCY(v) (context->v[y])
#define QDCY1(v) (context->v[y1])
#define QDCY2(v) (context->v[y2])
#define QDCXY(v) (context->v[(y  * context->xSize) + x ])
#define QDCXX(v) (context->v[(x2 * context->xSize) + x1])
#define QDCYY(v) (context->v[(y2 * context->ySize) + y1])
#define QDCXXr(v) (context->v[(x1 * context->xSize) + x2])
#define QDCYYr(v) (context->v[(y1 * context->ySize) + y2])
#define QDCXYc(v, x, y) (context->v[((y) * context->xSize) + (x)])
#define QDCEachX() for (x = 0; x < context->xSize; ++x)
#define QDCEachX1() for (x1 = 0; x1 < context->xSize; ++x1)
#define QDCEachX2() for (x2 = 0; x2 < context->xSize; ++x2)
#define QDCEachX1x() for (x1 = x2; x1 < context->xSize; ++x1)
#define QDCEachY() for (y = 0; y < context->ySize; ++y)
#define QDCEachY1() for (y1 = 0; y1 < context->ySize; ++y1)
#define QDCEachY2() for (y2 = 0; y2 < context->ySize; ++y2)
#define QDCEachY1x() for (y1 = y2; y1 < context->ySize; ++y1)
// #define QDCScan(p, begin, end) for (p = begin; p != end; ++p)
// #define QDCEachX() for ()

typedef struct {
    int xSize;
    int ySize;
    /* X  Y  */ int *count;
    /* X  Y  */ float *sum;
    /* X  Y  */ float *value; // if count, sum / count
    /* X     */ int *xCount;
    /*    Y  */ int *yCount;
    /* X     */ float *xAve; // if xCount
    /*    Y  */ float *yAve; // if yCount
    /* X  Y  */ float *xAbove; // if count xCount
    /* X  Y  */ float *yAbove; // if count yCount
    /* X     */ float *xRDelta; // if xCount
    /*    Y  */ float *yRDelta; // if yCount
    /* X1 X2 */ float *xxSim; // if xCount1 xCount2
    /* Y1 Y2 */ float *yySim; // if yCount1 yCount2
    /* X  Y  */ float *xPValue; // if xCount yCount
    /* X  Y  */ float *yPValue; // if xCount yCount
    /* X  Y  */ float *result;
} qdcContext;

float sqr(float v) {
    return v * v;
}

float rSqrt(float v) {
#ifdef QDCEVIL
    int i;
    float half;
    float result;

    half = v * 0.5;
    result = v;

    i = *(long *) &result;
    i = 0x5f375a86 - (i >> 1); // Evil!
    result = *(float *) &i;

    result = result * (1.5 - (half * result * result));
    result = result * (1.5 - (half * result * result));

    return result;
#else
    return 1 / sqrt(v);
#endif
}

float qdcRevBuf[65536];

void qdcBaseInit() {
    int i;

    qdcRevBuf[0] = 0;
    for (i = 1; i < 65536; ++i) {
        qdcRevBuf[i] = 1.0 / (float) i;
    }
}

qdcContext *qdcInit(int x, int y) {
    qdcContext *result;

    result = (qdcContext *) malloc(sizeof(qdcContext));
    result->xSize = x;
    result->ySize = y;
    result->count    = (int   *) calloc(x * y, sizeof(int));
    result->sum      = (float *) calloc(x * y, sizeof(float));
    result->value    = (float *) calloc(x * y, sizeof(float));
    result->xCount   = (int   *) calloc(x    , sizeof(int));
    result->yCount   = (int   *) calloc(y    , sizeof(int));
    result->xAve     = (float *) calloc(x    , sizeof(float));
    result->yAve     = (float *) calloc(y    , sizeof(float));
    result->xAbove   = (float *) calloc(x * y, sizeof(float));
    result->yAbove   = (float *) calloc(x * y, sizeof(float));
    result->xRDelta  = (float *) calloc(x    , sizeof(float));
    result->yRDelta  = (float *) calloc(y    , sizeof(float));
    result->xxSim    = (float *) calloc(x * x, sizeof(float));
    result->yySim    = (float *) calloc(y * y, sizeof(float));
    result->xPValue  = (float *) calloc(x * y, sizeof(float));
    result->yPValue  = (float *) calloc(x * y, sizeof(float));
    result->result   = (float *) calloc(x * y, sizeof(float));

    return result;
}

void qdcClear(qdcContext *context) { // count sum = 0
    int x;
    int y;

    QDCEachY() {
        QDCEachX() {
            QDCXY(sum) = 0;
            QDCXY(count) = 0;
        }
    }
}

void qdcFileLoad(qdcContext *context, FILE *input) { // count sum
    int x;
    int y;
    float value;

    // format: x y i value
    // "%d %d %*d %f\n"
/*#ifdef QDCEVIL
    char buf[32];
  #ifdef QDCXFIRST
    while (fgets(buf, 32, input), sscanf(buf, "%d%d%*s%f", &x, &y, &value) == 3) {
  #else
    while (fgets(buf, 32, input), sscanf(buf, "%d%d%*s%f", &y, &x, &value) == 3) {
  #endif
#else*/
  #ifdef QDCXFIRST
    while (fscanf(input, "%d%d%*s%f", &x, &y, &value) == 3) {
  #else
    while (fscanf(input, "%d%d%*s%f", &y, &x, &value) == 3) {
  #endif
//#endif
        QDCXY(sum) += value;
        QDCXY(count)++;
    }
}

void qdcValue(qdcContext *context) { // value
    int x;
    int y;

    QDCEachY() {
        QDCEachX() {
            if (QDCXY(count)) {
                QDCXY(value) = QDCXY(sum) * qdcRevBuf[QDCXY(count)];
            }
        }
    }
}

void qdcAve(qdcContext *context) { // xCount yCount xAve yAve
    int x;
    int y;
    int count;
    float sum;

    QDCEachY() {
        count = 0;
        sum = 0;

        QDCEachX() {
            if (QDCXY(count)) {
                count++;
                sum += QDCXY(value);
            }
        }

        QDCY(yCount) = count;
        if (QDCY(yCount)) {
            QDCY(yAve) = sum * qdcRevBuf[count];
        }
    }

    QDCEachX() {
        count = 0;
        sum = 0;

        QDCEachY() {
            if (QDCXY(count)) {
                count++;
                sum += QDCXY(value);
            }
        }

        QDCX(xCount) = count;
        if (QDCX(xCount)) {
            QDCX(xAve) = sum * qdcRevBuf[count];
        }
    }
}

void qdcAbove(qdcContext *context) { // xAbove yAbove
    int x;
    int y;

    QDCEachY() {
        if (QDCY(yCount)) {
            QDCEachX() {
                if (QDCXY(count)) {
                    QDCXY(xAbove) = QDCXY(value) - QDCY(yAve);
                } else {
                    QDCXY(xAbove) = 0; // for qdcSim()
                }
            }
        }
    }

    QDCEachX() {
        if (QDCX(xCount)) {
            QDCEachY() {
                if (QDCXY(count)) {
                    QDCXY(yAbove) = QDCXY(value) - QDCX(xAve);
                } else {
                    QDCXY(xAbove) = 0; // for qdcSim()
                }
            }
        }
    }
}

void qdcRDelta(qdcContext *context) { // xRDelta yRDelta
    int x;
    int y;
    float sum;

    QDCEachY() {
        if (QDCY(yCount)) {
            sum = 0;

            QDCEachX() {
                if (QDCXY(count)) {
                    sum += sqr(QDCXY(yAbove));
                }
            }

            QDCY(yRDelta) = rSqrt(sum);
        }
    }

    QDCEachX() {
        if (QDCX(xCount)) {
            sum = 0;

            QDCEachY() {
                if (QDCXY(count)) {
                    sum += sqr(QDCXY(xAbove));
                }
            }

            QDCX(xRDelta) = rSqrt(sum);
        }
    }
}

void qdcSim(qdcContext *context) { // xxSim yySim
    int x;
    int y;
    int x1;
    int x2;
    int y1;
    int y2;
    float sum;
    float result;

    QDCEachY2() {
        if (QDCY2(yCount)) {
            QDCEachY1x() {
                if (QDCY1(yCount)) {
                    sum = 0;

                    QDCEachX() {
                        // if (QDCXYc(count, x, y1) && QDCXYc(count, x, y2)) {
                            sum += QDCXYc(yAbove, x, y1) * QDCXYc(yAbove, x, y2);
                        // }
                    }

                    result = sum * QDCY1(yRDelta) * QDCY2(yRDelta);
                    QDCYY(yySim) = result;
                    QDCYYr(yySim) = result;
                }
            }
        }
    }

    QDCEachX2() {
        if (QDCX2(xCount)) {
            QDCEachX1x() {
                if (QDCX1(xCount)) {
                    sum = 0;

                    QDCEachY() {
                        // if (QDCXYc(count, x1, y) && QDCXYc(count, x2, y)) {
                            sum += QDCXYc(xAbove, x1, y) * QDCXYc(xAbove, x2, y);
                        // }
                    }

                    result = sum * QDCX1(xRDelta) * QDCX2(xRDelta);
                    QDCXX(xxSim) = result;
                    QDCXXr(xxSim) = result;
                }
            }
        }
    }
}

void qdcPValue(qdcContext *context) { // xPValue yPValue
    int x;
    int y;
    int x1;
    int x2;
    int y1;
    int y2;
    float sum;
    float bsum;

    QDCEachX() {
        QDCEachY2() {
            if (QDCY2(yCount)) {
                sum = 0;
                bsum = 0;

                QDCEachY1() {
                    if (QDCY1(yCount) && QDCYY(yySim) >= 0) {
                        sum += QDCYY(yySim) * QDCXYc(yAbove, x, y1);
                        bsum += QDCYY(yySim);
                    }
                }

                if (bsum > 0) {
                    QDCXYc(yPValue, x, y2) = QDCY2(yAve) + sum / bsum;
                } else {
                    QDCXYc(yPValue, x, y2) = QDCY2(yAve);
                }
            }
        }
    }

    QDCEachY() {
        QDCEachX2() {
            if (QDCX2(xCount)) {
                sum = 0;
                bsum = 0;

                QDCEachX1() {
                    if (QDCX1(xCount) && QDCXX(xxSim) >= 0) {
                        sum += QDCXX(xxSim) * QDCXYc(xAbove, x1, y);
                        bsum += QDCXX(xxSim);
                    }
                }

                if (bsum > 0) {
                    QDCXYc(xPValue, x2, y) = QDCX2(xAve) + sum / bsum;
                } else {
                    QDCXYc(xPValue, x2, y) = QDCX2(xAve);
                }
            }
        }
    }
}

void qdcResult(qdcContext *context) { // result
    int x;
    int y;

    QDCEachY() {
        QDCEachX() {
            if (QDCX(xCount) && QDCY(yCount)) {
                // take both
                // TODO

                QDCXY(result) = 0.5 * QDCXY(xPValue) + 0.5 * QDCXY(yPValue);
            } else if (QDCX(xCount)) {
                QDCXY(result) = QDCXY(xPValue);
            } else if (QDCY(yCount)) {
                QDCXY(result) = QDCXY(yPValue);
            } else {
                // no data
            }
        }
    }
}

void qdcFileSave(qdcContext *context, FILE *output) {
    int x;
    int y;

    // format: x y value result
#ifdef QDCXFIRST
    QDCEachX() {
        QDCEachY() {
            fprintf(output, "%d %d %f %f\n", x, y, QDCXY(value), QDCXY(result));
        }
    }
#else
    QDCEachY() {
        QDCEachX() {
            fprintf(output, "%d %d %f %f\n", y, x, QDCXY(value), QDCXY(result));
        }
    }
#endif
}

void qdcFree(qdcContext *context) {
    free(context->count);
    free(context->sum);
    free(context->value);
    free(context->xCount);
    free(context->yCount);
    free(context->xAve);
    free(context->yAve);
    free(context->xAbove);
    free(context->yAbove);
    free(context->xRDelta);
    free(context->yRDelta);
    free(context->xxSim);
    free(context->yySim);
    free(context->xPValue);
    free(context->yPValue);
    free(context->result);
    free(context);
}

int main() {
    qdcContext *qdc;
    FILE *input;
    FILE *output;

    /* info */ printf("Init system\n");
    qdcBaseInit();

    /* info */ printf("Allocate memory\n");
#ifdef QDCXFIRST
    qdc = qdcInit(142, 4500);
#else
    qdc = qdcInit(4500, 142);
#endif
    /* info */ printf("Fill zero\n");
    qdcClear(qdc);

    /* info */ printf("Load input\n");
#ifdef QDCMEM
    input = fopen("/dev/shm/in.txt", "r");
#else
    input = fopen("in.txt", "r");
#endif
    qdcFileLoad(qdc, input);
    fclose(input);

    /* info */ printf("Parse 1: value\n");
    qdcValue(qdc);
    /* info */ printf("Parse 1: average\n");
    qdcAve(qdc);
    /* info */ printf("Parse 1: above\n");
    qdcAbove(qdc);
    /* info */ printf("Parse 2: rev delta\n");
    qdcRDelta(qdc);
    /* info */ printf("Parse 2: sim\n");
    qdcSim(qdc);
    // TODO: filter sim(k), if small, set to 0
    /* info */ printf("Parse 3: p value\n");
    qdcPValue(qdc);
    /* info */ printf("Parse 3: result\n");
    qdcResult(qdc);

    /* info */ printf("Save output\n");
    output = fopen("out.txt", "w");
    qdcFileSave(qdc, output);
    fclose(output);

    /* info */ printf("Free memory\n");
    qdcFree(qdc);

    return 0;
}
