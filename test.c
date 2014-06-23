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

typedef long qdcint;
typedef float qdcfloat;

typedef struct {
    qdcint xSize;
    qdcint ySize;
    /* X  Y  */ qdcint *count;
    /* X  Y  */ qdcfloat *sum;
    /* X  Y  */ qdcfloat *value; // if count, sum / count
    /* X     */ qdcint *xCount;
    /*    Y  */ qdcint *yCount;
    /* X     */ qdcfloat *xAve; // if xCount
    /*    Y  */ qdcfloat *yAve; // if yCount
    /* X  Y  */ qdcfloat *xAbove; // if count xCount
    /* X  Y  */ qdcfloat *yAbove; // if count yCount
    /* X     */ qdcfloat *xRDelta; // if xCount
    /*    Y  */ qdcfloat *yRDelta; // if yCount
    /* X1 X2 */ qdcfloat *xxSim; // if xCount1 xCount2
    /* Y1 Y2 */ qdcfloat *yySim; // if yCount1 yCount2
    /* X  Y  */ qdcfloat *xPValue; // if xCount yCount
    /* X  Y  */ qdcfloat *yPValue; // if xCount yCount
    /* X  Y  */ qdcfloat *result;
} qdcContext;

qdcint fgetd(FILE *input) {
    qdcint result = 0;
    qdcint new;

    while (1) {
        new = fgetc(input);
        if (new >= '0' && new <= '9') {
            result = result * 10 + (new - '0');
        } else {
            break;
        }
    }

    return result;
}

void fgetdi(FILE *input) {
    qdcint new;

    while (1) {
        new = fgetc(input);
        if (new >= '0' && new <= '9') {
            // nothing
        } else {
            break;
        }
    }
}

qdcfloat fgetf(FILE *input) {
    const qdcfloat rev10[16] = {
        1,     1e-1,  1e-2,  1e-3,
        1e-4,  1e-5,  1e-6,  1e-7,
        1e-8,  1e-9,  1e-10, 1e-11,
        1e-12, 1e-13, 1e-14, 1e-15,
    };
    qdcint result = 0;
    qdcint count = 0;
    qdcint new;

    while (1) {
        new = fgetc(input);
        if (new >= '0' && new <= '9') {
            result = result * 10 + (new - '0');
        } else {
            break;
        }
    }

    while (1) {
        new = fgetc(input);
        if (new >= '0' && new <= '9') {
            result = result * 10 + (new - '0');
            count++;
        } else {
            break;
        }
    }

    return rev10[count] * result;
}

qdcfloat sqr(qdcfloat v) {
    return v * v;
}

qdcfloat rSqrt(qdcfloat v) {
// #ifdef QDCEVIL
#if 0
    qdcint i;
    float half;
    float result;

    half = v * 0.5f;
    result = v;

    i = *(long *) &result;
    i = 0x5f375a86 - (i >> 1); // Evil!
    result = *(qdcfloat *) &i;

    result = result * (1.5f - (half * result * result));
    result = result * (1.5f - (half * result * result));

    return result;
#else
    return 1 / sqrt(v);
#endif
}

qdcfloat qdcRevBuf[65536];

void qdcBaseInit() {
    qdcint i;

    qdcRevBuf[0] = 0;
    for (i = 1; i < 65536; ++i) {
        qdcRevBuf[i] = (qdcfloat) 1.0 / (qdcfloat) i;
    }
}

qdcContext *qdcInit(qdcint x, qdcint y) {
    qdcContext *result;

    result = (qdcContext *) malloc(sizeof(qdcContext));
    result->xSize = x;
    result->ySize = y;
    result->count    = (qdcint   *) calloc(x * y, sizeof(qdcint));
    result->sum      = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
    result->value    = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
    result->xCount   = (qdcint   *) calloc(x    , sizeof(qdcint));
    result->yCount   = (qdcint   *) calloc(y    , sizeof(qdcint));
    result->xAve     = (qdcfloat *) calloc(x    , sizeof(qdcfloat));
    result->yAve     = (qdcfloat *) calloc(y    , sizeof(qdcfloat));
    result->xAbove   = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
    result->yAbove   = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
    result->xRDelta  = (qdcfloat *) calloc(x    , sizeof(qdcfloat));
    result->yRDelta  = (qdcfloat *) calloc(y    , sizeof(qdcfloat));
    result->xxSim    = (qdcfloat *) calloc(x * x, sizeof(qdcfloat));
    result->yySim    = (qdcfloat *) calloc(y * y, sizeof(qdcfloat));
    result->xPValue  = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
    result->yPValue  = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
    result->result   = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));

    return result;
}

void qdcClear(qdcContext *context) { // count sum = 0
    qdcint x;
    qdcint y;

    QDCEachY() {
        QDCEachX() {
            QDCXY(sum) = 0;
            QDCXY(count) = 0;
        }
    }
}

void qdcFileLoad(qdcContext *context, FILE *input) { // count sum
    qdcint x;
    qdcint y;
    qdcfloat value;

    // format: x y i value
    // "%ld %ld %*ld %lf\n"
#ifdef QDCEVIL
    x = 0;
    y = 0;
    QDCXY(count)--; // last line is null

    while (!feof(input)) {
  #ifdef QDCXFIRST
        x = fgetd(input);
        y = fgetd(input);
  #else
        y = fgetd(input);
        x = fgetd(input);
  #endif
        fgetdi(input);
        value = fgetf(input);
#else
  #ifdef QDCXFIRST
    while (fscanf(input, "%ld%ld%*s%f", &x, &y, &value) == 3) {
  #else
    while (fscanf(input, "%ld%ld%*s%f", &y, &x, &value) == 3) {
  #endif
#endif
        QDCXY(sum) += value;
        QDCXY(count)++;
    }
}

void qdcValue(qdcContext *context) { // value
    qdcint x;
    qdcint y;

    QDCEachY() {
        QDCEachX() {
            if (QDCXY(count)) {
                QDCXY(value) = QDCXY(sum) * qdcRevBuf[QDCXY(count)];
            }
        }
    }
}

void qdcAve(qdcContext *context) { // xCount yCount xAve yAve
    qdcint x;
    qdcint y;
    qdcint count;
    qdcfloat sum;

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
    qdcint x;
    qdcint y;

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
    qdcint x;
    qdcint y;
    qdcfloat sum;

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
    qdcint x;
    qdcint y;
    qdcint x1;
    qdcint x2;
    qdcint y1;
    qdcint y2;
    qdcfloat sum;
    qdcfloat result;

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
    qdcint x;
    qdcint y;
    qdcint x1;
    qdcint x2;
    qdcint y1;
    qdcint y2;
    qdcfloat sum;
    qdcfloat bsum;

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
    qdcint x;
    qdcint y;

    QDCEachY() {
        QDCEachX() {
            if (QDCX(xCount) && QDCY(yCount)) {
                // take both
                // TODO

                QDCXY(result) = (qdcfloat) 0.5 * QDCXY(xPValue) + (qdcfloat) 0.5 * QDCXY(yPValue);
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
    qdcint x;
    qdcint y;

    // format: x y value result
#ifdef QDCXFIRST
    QDCEachX() {
        QDCEachY() {
            fprintf(output, "%ld %ld %lf %lf\n", x, y, QDCXY(value), QDCXY(result));
        }
    }
#else
    QDCEachY() {
        QDCEachX() {
            fprintf(output, "%ld %ld %lf %lf\n", y, x, QDCXY(value), QDCXY(result));
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
