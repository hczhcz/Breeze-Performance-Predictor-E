#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define QDCMEMFILE
#define QDCEVIL
#define QDCXFIRST
#define QDCTRANSFORM 2

#ifdef QDCEVIL
  #include <xmmintrin.h>

  #define QDCEVIL_RSQRT
  #define QDCEVIL_FASTREAD
  #define QDCEVIL_WEAKCHECK
  #define QDCEVIL_ALIGN
  #define QDCEVIL_SSE
#else
#endif

#if QDCTRANSFORM == 1
  #define QDCBefore(x) ((x) * rSqrt(x))
  #define QDCAfter(x) sqr(x)
#elif QDCTRANSFORM == 2
  #define QDCBefore(x) (((x) + 1) * rSqrt((x) + 1))
  #define QDCAfter(x) (sqr(x) - 1)
#elif QDCTRANSFORM == 3
  #define QDCBefore(x) (((x) > 1) ? (x) : (x) * rSqrt(x))
  #define QDCAfter(x) (((x) > 1) ? (x) : sqr(x))
#elif QDCTRANSFORM == 4
  #define QDCBefore(x) log((x) + 1)
  #define QDCAfter(x) (exp(x) + 1)
#endif

#define QDCX(v) (context->v[x])
#define QDCX1(v) (context->v[x1])
#define QDCX2(v) (context->v[x2])
#define QDCY(v) (context->v[y])
#define QDCY1(v) (context->v[y1])
#define QDCY2(v) (context->v[y2])
#define QDCXY(v) (context->v[(y  * context->xSizeA) + x ])
#define QDCYX(v) (context->v[(x  * context->ySizeA) + y ])
#define QDCXX(v) (context->v[(x2 * context->xSizeA) + x1])
#define QDCYY(v) (context->v[(y2 * context->ySizeA) + y1])
#define QDCXXr(v) (context->v[(x1 * context->xSizeA) + x2])
#define QDCYYr(v) (context->v[(y1 * context->ySizeA) + y2])
#define QDCXYc(v, x, y) (context->v[((y) * context->xSizeA) + (x)])
#define QDCYXc(v, x, y) (context->v[((x) * context->ySizeA) + (y)])
#define QDCEachX() for (x = 0; x < context->xSize; ++x)
#define QDCEachX1() for (x1 = 0; x1 < context->xSize; ++x1)
#define QDCEachX2() for (x2 = 0; x2 < context->xSize; ++x2)
#define QDCEachX1x() for (x1 = x2; x1 < context->xSize; ++x1)
#define QDCEachY() for (y = 0; y < context->ySize; ++y)
#define QDCEachY1() for (y1 = 0; y1 < context->ySize; ++y1)
#define QDCEachY2() for (y2 = 0; y2 < context->ySize; ++y2)
#define QDCEachY1x() for (y1 = y2; y1 < context->ySize; ++y1)

#define QDCMessage(x) printf("[%10ld] " x "\n", clock())

typedef long qdcint;
typedef float qdcfloat;

typedef struct {
    qdcint xSize;
    qdcint ySize;
    qdcint xSizeA; // Align
    qdcint ySizeA; // Align
    qdcfloat lambda;

    /* X  Y  */ qdcint *count;
    /* X  Y  */ qdcfloat *sum;
    /* X  Y  */ qdcfloat *value; // if count, sum / count
    /* X  Y  */ qdcfloat *trans; // if count // if not def QDCTRANSFORM, trans = value
    /* X     */ qdcint *xCount;
    /*    Y  */ qdcint *yCount;
    /* X     */ qdcfloat *xAve; // if xCount
    /*    Y  */ qdcfloat *yAve; // if yCount
    /* Y  X  */ qdcfloat *xAbove; // if count xCount
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
        new = fgetc(input) - '0';
        if (new >= 0/* && new <= 9*/) {
            result = result * 10 + new;
        } else {
            break;
        }
    }

    return result;
}

void fgetdi(FILE *input) {
    qdcint new;

    while (1) {
        new = fgetc(input) - '0';
        if (new >= 0/* && new <= 9*/) {
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
        new = fgetc(input) - '0';
        if (new >= 0/* && new <= 9*/) {
            result = result * 10 + new;
        } else {
            break;
        }
    }

    while (1) {
        new = fgetc(input) - '0';
        if (new >= 0/* && new <= 9*/) {
            result = result * 10 + new;
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
#ifdef QDCEVIL_RSQRT
// #if 0
    // method from Quake 3
    // for float only !!!
    int i;
    float half;
    float result;

    half = v * 0.5f;
    result = v;

    i = *(int *) &result;
    i = 0x5f375a86 - (i >> 1); // Evil!
    result = *(float *) &i;

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

qdcContext *qdcInit(qdcint x, qdcint y, qdcfloat lambda) {
    qdcContext *result;

    result = (qdcContext *) malloc(sizeof(qdcContext));
    result->xSize = x;
    result->ySize = y;
#ifdef QDCEVIL_ALIGN
    x = (x + 3) & ~3;
    y = (y + 3) & ~3;
    // printf("Print this line make it faster. I do not know why :(\n");
#endif
    result->xSizeA = x;
    result->ySizeA = y;
    result->lambda = lambda;
    result->count    = (qdcint   *) calloc(x * y, sizeof(qdcint));
    result->sum      = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
    result->value    = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
#ifdef QDCTRANSFORM
    result->trans    = (qdcfloat *) calloc(x * y, sizeof(qdcfloat));
#else
    result->trans    = result->value;
#endif
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

    clearerr(input);

    // format: x y i value
    // "%ld %ld %*ld %f\n"
#ifdef QDCEVIL_FASTREAD
    // for well-formatted data only
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
#ifdef QDCTRANSFORM
                QDCXY(trans) = QDCBefore(QDCXY(value));
#endif
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
                sum += QDCXY(trans);
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
                sum += QDCXY(trans);
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
                    QDCYX(yAbove) = QDCXY(trans) - QDCY(yAve);
                } else {
                    // QDCYX(yAbove) = 0; // for qdcSim()
                }
            }
        }
    }

    QDCEachX() {
        if (QDCX(xCount)) {
            QDCEachY() {
                if (QDCXY(count)) {
                    QDCXY(xAbove) = QDCXY(trans) - QDCX(xAve);
                } else {
                    // QDCXY(xAbove) = 0; // for qdcSim()
                }
            }
        }
    }
}

void qdcRDelta(qdcContext *context) { // xRDelta yRDelta
    qdcint x;
    qdcint y;
    qdcfloat sum;

    // TODO: is this (swap xAbove and yAbove) right ?
    QDCEachY() {
        if (QDCY(yCount)) {
            sum = 0;

            QDCEachX() {
                if (QDCXY(count)) {
                    sum += sqr(QDCXY(xAbove));
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
                    sum += sqr(QDCYX(yAbove));
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
#ifdef QDCEVIL_WEAKCHECK
                        if (1) {
#else
                        if (QDCXYc(count, x, y1) && QDCXYc(count, x, y2)) {
#endif
                            sum += QDCXYc(xAbove, x, y1) * QDCXYc(xAbove, x, y2);
                        }
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
#ifdef QDCEVIL_WEAKCHECK
                        if (1) {
#else
                        if (QDCXYc(count, x1, y) && QDCXYc(count, x2, y)) {
#endif
                            sum += QDCYXc(yAbove, x1, y) * QDCYXc(yAbove, x2, y);
                        }
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
//#ifdef QDCEVIL_SSE
    __m128 sum_sse;
    __m128 bsum_sse;
//#else
    qdcfloat sum;
    qdcfloat bsum;
//#endif

    QDCEachY2() {
        if (QDCY2(yCount)) {
            QDCEachX() {
                sum = 0;
                bsum = 0;

                QDCEachY1() {
#ifdef QDCEVIL_WEAKCHECK
                    if (*(int *) &QDCYY(yySim) >= 0) { // check sign bit only
#else
                    if (QDCYY(yySim) >= 0) {
#endif
                        sum += QDCYY(yySim) * QDCYXc(yAbove, x, y1);
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

    QDCEachX2() {
        if (QDCX2(xCount)) {
            QDCEachY() {
                sum = 0;
                bsum = 0;

                QDCEachX1() {
#ifdef QDCEVIL_WEAKCHECK
                    if (*(int *) &QDCXX(xxSim) >= 0) { // check sign bit only
#else
                    if (QDCXX(xxSim) >= 0) {
#endif
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
    qdcfloat result;

    QDCEachY() {
        QDCEachX() {
            if (QDCX(xCount) && QDCY(yCount)) {
                result = context->lambda * QDCXY(xPValue)
                       + (1 - context->lambda) * QDCXY(yPValue);
            } else if (QDCX(xCount)) {
                result = QDCXY(xPValue);
            } else if (QDCY(yCount)) {
                result = QDCXY(yPValue);
            } else {
                // no data
            }
#ifdef QDCTRANSFORM
            QDCXY(result) = QDCAfter(result);
#else
            QDCXY(result) = result;
#endif
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
            fprintf(output, "%ld %ld %f %f\n", x, y, QDCXY(value), QDCXY(result));
        }
    }
#else
    QDCEachY() {
        QDCEachX() {
            fprintf(output, "%ld %ld %f %f\n", y, x, QDCXY(value), QDCXY(result));
        }
    }
#endif
}

qdcfloat qdcDelta(qdcContext *context) {
    qdcint x;
    qdcint y;
    qdcfloat result = 0;

    QDCEachY() {
        QDCEachX() {
            if (QDCXY(count)) {
                result += sqr(QDCXY(value) - QDCXY(result));
            }
        }
    }

    return result;
}

qdcfloat qdcRelDelta(qdcContext *context) {
    qdcint x;
    qdcint y;
    qdcfloat result = 0;

    QDCEachY() {
        QDCEachX() {
            if (QDCXY(count)) {
                result += sqr(QDCXY(value) - QDCXY(result)) / (
                    sqr(QDCXY(value)) + sqr(QDCXY(result))
                );
            }
        }
    }

    return result;
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

    QDCMessage("Init system");
    qdcBaseInit();

    QDCMessage("Allocate memory");
#ifdef QDCXFIRST
    qdc = qdcInit(142, 4500, 0.5);
#else
    qdc = qdcInit(4500, 142, 0.5);
#endif
    QDCMessage("Fill zero");
    qdcClear(qdc);

    QDCMessage("Load input");
#ifdef QDCMEMFILE
    input = fopen("/dev/shm/in.txt", "r");
#else
    input = fopen("in.txt", "r");
#endif
    qdcFileLoad(qdc, input);
    fclose(input);

    QDCMessage("Parse 1: value");
    qdcValue(qdc);
    QDCMessage("Parse 1: average");
    qdcAve(qdc);
    QDCMessage("Parse 1: above");
    qdcAbove(qdc);
    QDCMessage("Parse 2: rev delta");
    qdcRDelta(qdc);
    QDCMessage("Parse 2: sim");
    qdcSim(qdc);
    // TODO: filter sim(k), if small, set to 0
    QDCMessage("Parse 3: p value");
    qdcPValue(qdc);
    QDCMessage("Parse 3: result");
    qdcResult(qdc);

    QDCMessage("Save output");
    output = fopen("out.txt", "w");
    qdcFileSave(qdc, output);
    fclose(output);

    QDCMessage("Calculate delta");
    printf("Delta = %f\n", qdcDelta(qdc));

    QDCMessage("Calculate rel delta");
    printf("RelDelta = %f\n", qdcRelDelta(qdc));

    QDCMessage("Free memory");
    qdcFree(qdc);

    return 0;
}
