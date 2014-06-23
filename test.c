#include <stdlib.h>
#include <stdio.h>

#define QDCX(v) (context->v[x])
#define QDCX1(v) (context->v[x1])
#define QDCX2(v) (context->v[x2])
#define QDCY(v) (context->v[y])
#define QDCY1(v) (context->v[y1])
#define QDCY2(v) (context->v[y2])
#define QDCXY(v) (context->v[(y  * context->xSize) + x ])
#define QDCXX(v) (context->v[(x2 * context->xSize) + x1])
#define QDCYY(v) (context->v[(y2 * context->xSize) + y1])
#define QDCXY2(v, x, y) (context->v[((y) * context->xSize) + (x)])
#define QDCEachX() for (x = 0; x < context->xSize; ++x)
#define QDCEachX1() for (x1 = x2 + 1; x1 < context->xSize; ++x1)
#define QDCEachX2() for (x2 = 0; x2 < context->xSize; ++x2)
#define QDCEachY() for (y = 0; y < context->ySize; ++y)
#define QDCEachY1() for (y1 = y2 + 1; y1 < context->ySize; ++y1)
#define QDCEachY2() for (y2 = 0; y2 < context->ySize; ++y2)
// #define QDCScan(p, begin, end) for (p = begin; p != end; ++p)
// #define QDCEachX() for ()

typedef struct {
    int xSize;
    int ySize;
    int *count;
    float *sum;
    float *value; // if count, sum / count
    int *xCount;
    int *yCount;
    float *xAve; // if xCount
    float *yAve; // if yCount
    float *xRDelta; // if xCount
    float *yRDelta; // if yCount
    float *xxSim;
    float *yySim;
    float *xxPValue;
    float *yyPValue;
    float *result;
} qdcContext;

float sqr(float v) {
    return v * v;
}

float rSqrt(float v) {
    int i;
    float half;
    float result;

    half = v * 0.5;
    result = v;

    i = *(long *) &result;
    i = 0x5f3759df - (i >> 1); // Evil!
    result = *(float *) &i;

    result = result * (1.5 - (half * result * result));
    result = result * (1.5 - (half * result * result));

    return result;
}

float qdcRevBuf[65536];

void qdcBaseInit() {
    int i;

    qdcRevBuf[0] = 0;
    for (i = 1; i < 65536; ++i) {
        qdcRevBuf[i] = 1 / i;
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
    result->xRDelta  = (float *) calloc(x    , sizeof(float));
    result->yRDelta  = (float *) calloc(y    , sizeof(float));
    result->xxSim    = (float *) calloc(x * x, sizeof(float));
    result->yySim    = (float *) calloc(y * y, sizeof(float));
    result->xxPValue = (float *) calloc(x * x, sizeof(float));
    result->yyPValue = (float *) calloc(y * y, sizeof(float));
    result->result   = (float *) calloc(x * y, sizeof(float));

    return result;
}

void qdcFileLoad(qdcContext *context, FILE *input) { // count sum

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

void qdcRDelta(qdcContext *context) { // xRDelta yRDelta
    int x;
    int y;
    float sum;

    QDCEachY() {
        if (QDCY(yCount)) {
            sum = 0;

            QDCEachX() {
                if (QDCXY(count)) {
                    sum += sqr(QDCXY(value) - QDCY(yAve));
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
                    sum += sqr(QDCXY(value) - QDCX(xAve));
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
    float ave1;
    float ave2;

    QDCEachX2() {
        QDCEachX1() {
            if (QDCX1(xCount) > 0 && QDCX2(xCount) > 0) {
                sum = 0;
                ave1 = QDCX1(xAve);
                ave2 = QDCX2(xAve);

                QDCEachY() {
                    sum += (QDCXY2(value, x1, y) - ave1) * (QDCXY2(value, x2, y) - ave2);
                }

                QDCXX(xxSim) = sum * QDCX1(xRDelta) * QDCX2(xRDelta);
            }
        }
    }

    QDCEachY2() {
        QDCEachY1() {
            if (QDCY1(yCount) > 0 && QDCY2(yCount) > 0) {
                sum = 0;
                ave1 = QDCY1(yAve);
                ave2 = QDCY2(yAve);

                QDCEachX() {
                    sum += (QDCXY2(value, x, y1) - ave1) * (QDCXY2(value, x, y2) - ave2);
                }

                QDCYY(yySim) = sum * QDCY1(yRDelta) * QDCY2(yRDelta);
            }
        }
    }
}

void qdcPValue(qdcContext *context) { // xxPValue yyPValue
}

void qdcResult(qdcContext *context) { // result
}

void qdcFileSave(qdcContext *context, FILE *output) {
}

void qdcFree(qdcContext *context) {
    free(context->count);
    free(context->sum);
    free(context->value);
    free(context->xCount);
    free(context->yCount);
    free(context->xAve);
    free(context->yAve);
    free(context->xRDelta);
    free(context->yRDelta);
    free(context->xxSim);
    free(context->yySim);
    free(context->xxPValue);
    free(context->yyPValue);
    free(context->result);
    free(context);
}

int main() {
    qdcContext *qdc;
    FILE *input;
    FILE *output;

    qdcBaseInit();

    qdc = qdcInit(142, 4500);

    input = fopen("in.txt", "r");
    qdcFileLoad(qdc, input);
    fclose(input);

    qdcValue(qdc);
    qdcAve(qdc);
    qdcRDelta(qdc);
    qdcSim(qdc);
    qdcPValue(qdc);
    qdcResult(qdc);

    output = fopen("out.txt", "w");
    qdcFileSave(qdc, output);
    fclose(output);

    qdcFree(qdc);

    return 0;
}
