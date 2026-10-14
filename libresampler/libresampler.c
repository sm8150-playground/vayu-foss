/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdint.h>


#define EXPORT __attribute__((visibility("default"), noinline))

#ifndef NULL
#define NULL ((void *)0)
#endif




typedef __SIZE_TYPE__ size_t;
extern void *malloc(size_t n);
extern void free(void *p);
extern void *memcpy(void *d, const void *s, size_t n);
extern void *memmove(void *d, const void *s, size_t n);
extern void *memset(void *d, int c, size_t n);
extern float expf(float x);
extern double pow(double x, double y);
extern double sinh(double x);
extern void sincosf(float x, float *sin_out, float *cos_out);
extern int __android_log_print(int prio, const char *tag, const char *fmt, ...);

#define LOG_TAG "Mi_Resampler"
#define LOG_PRIO_ERROR 6



static inline float fmin_nan(float a, float b) { return (a < b) ? a : b; }



static inline float vselgt(float a, float b, float t, float f) {
    return (a > b) ? t : f;
}





typedef struct {
    float b0, b1, b2, a1, a2;
    float d1, d2;
    float x1, x2;
    int32_t b0f, b1f, b2f, a1f, a2f;
    int32_t fx1;
    int32_t fx2;
    int32_t m40;
    int32_t m44;
    int32_t m48;
    int32_t m4c;
    int32_t m50;
    int32_t m54;
    int32_t type;
    float fs;
    float fc;
    float db;
    float q;
} CMusicBiquad;

EXPORT CMusicBiquad *Biquad_New(void) {
    CMusicBiquad *bq = (CMusicBiquad *)malloc(sizeof(CMusicBiquad));

    if (bq == NULL)
        return NULL;
    memset(bq, 0, sizeof(*bq));
    bq->fc = 48000.0f;
    return bq;
}

EXPORT void Biquad_ResetHistory(CMusicBiquad *bq) {
    bq->d1 = 0.0f;
    bq->d2 = 0.0f;
    bq->x1 = 0.0f;
    bq->x2 = 0.0f;
    bq->fx1 = 0;
    bq->fx2 = 0;
    bq->m40 = 0;
    bq->m44 = 0;
    bq->m48 = 0;
    bq->m4c = 0;
    bq->m50 = 0;
    bq->m54 = 0;
}

EXPORT float Biquad_Process(float x, CMusicBiquad *bq) {
    float x1 = bq->x1;
    float b1 = bq->b1;
    float b0 = bq->b0;
    float x2 = bq->x2;
    float b2 = bq->b2;
    float a1 = bq->a1;
    float d1 = bq->d1;
    float d2 = bq->d2;
    float a2 = bq->a2;
    float y;

    bq->x1 = x;
    y = b1 * x1;
    bq->x2 = x1;
    bq->d2 = d1;





    y = b0 * x + y;
    y = b2 * x2 + y;
    y = -a1 * d1 + y;
    y = -a2 * d2 + y;
    bq->d1 = y;
    return y;
}


EXPORT float Biquad_Process_ScaleB0PreMult(float x, CMusicBiquad *bq) {
    float x1 = bq->x1;
    float b1 = bq->b1;
    float x2 = bq->x2;
    float b2 = bq->b2;
    float a1 = bq->a1;
    float d1 = bq->d1;
    float y = x;
    float d2 = bq->d2;
    float a2 = bq->a2;

    bq->x1 = x;
    y = b1 * x1 + y;
    bq->x2 = x1;
    bq->d2 = d1;
    y = b2 * x2 + y;
    y = -a1 * d1 + y;
    y = -a2 * d2 + y;
    bq->d1 = y;
    return y;
}



EXPORT void Biquad_SetCoefsDirectly(const float *coefs, CMusicBiquad *bq) {
    const float scale = 16777216.0f;

    bq->fx1 = 0;
    bq->fx2 = 0;
    bq->m40 = 0;
    bq->m44 = 0;
    bq->m48 = 0;
    bq->m4c = 0;
    bq->m50 = 0;
    bq->m54 = 0;
    bq->d1 = 0.0f;
    bq->d2 = 0.0f;
    bq->x1 = 0.0f;
    bq->x2 = 0.0f;

    bq->b0 = coefs[0];
    bq->b1 = coefs[1];
    bq->b2 = coefs[2];
    bq->a1 = coefs[4];
    bq->a2 = coefs[5];

    bq->b0f = (int32_t)(coefs[0] * scale);
    bq->b1f = (int32_t)(coefs[1] * scale);
    bq->b2f = (int32_t)(coefs[2] * scale);
    bq->a1f = (int32_t)(coefs[4] * scale);
    bq->a2f = (int32_t)(coefs[5] * scale);
}


EXPORT int32_t Biquad_Process_Fix(int32_t x0, int32_t x1, CMusicBiquad *bq) {
    uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, lr;
    uint32_t c;
    uint64_t p;

    (void)r11;

    r12 = (uint32_t)bq->m4c;
    bq->m4c = x1;
    r10 = (uint32_t)bq->m54;
    r3 = (uint32_t)bq->b0f;
    r4 = (uint32_t)bq->b1f;
    r5 = (uint32_t)bq->b2f;
    lr = (uint32_t)bq->a1f;
    bq->m54 = (int32_t)r12;

    r0 = (uint32_t)x0;
    r1 = (uint32_t)x1;


    p = (uint64_t)r3 * r0;
    r8 = (uint32_t)p;
    r7 = (uint32_t)(p >> 32);
    r1 = r3 * r1 + r7;
    r7 = (uint32_t)bq->m48;
    r3 = (uint32_t)((int32_t)r3 >> 31);
    bq->m48 = (int32_t)r0;
    r9 = r3 * r0 + r1;


    r2 = (uint32_t)bq->m50;
    r0 = (uint32_t)((int32_t)r4 >> 31);
    p = (uint64_t)r7 * r4;
    r3 = (uint32_t)p;
    r6 = (uint32_t)(p >> 32);
    bq->m50 = (int32_t)r7;
    r0 = r7 * r0 + r6;
    c = ((uint64_t)r3 + r8) >> 32;
    r3 = r3 + r8;
    r0 = r12 * r4 + r0;


    p = (uint64_t)r2 * r5;
    r11 = (uint32_t)p;
    r4 = (uint32_t)(p >> 32);
    r0 = r0 + r9 + c;
    r6 = (uint32_t)((int32_t)r5 >> 31);
    r1 = r2 * r6 + r4;
    c = ((uint64_t)r3 + r11) >> 32;
    r8 = r3 + r11;
    r4 = (uint32_t)bq->m40;
    r1 = r10 * r5 + r1;
    r5 = (uint32_t)((int32_t)lr >> 31);
    r12 = r0 + r1 + c;


    r1 = (uint32_t)bq->fx1;
    p = (uint64_t)r1 * lr;
    r9 = (uint32_t)p;
    r6 = (uint32_t)(p >> 32);
    bq->m40 = (int32_t)r1;
    r1 = r1 * r5 + r6;
    r5 = (uint32_t)bq->fx2;
    r6 = (uint32_t)bq->m44;
    bq->m44 = (int32_t)r5;
    r1 = r5 * lr + r1;


    r5 = (uint32_t)bq->a2f;
    p = (uint64_t)r4 * r5;
    r0 = (uint32_t)p;
    r3 = (uint32_t)(p >> 32);
    r7 = (uint32_t)((int32_t)r5 >> 31);
    r3 = r4 * r7 + r3;
    c = ((uint64_t)r0 + r9) >> 32;
    r0 = r0 + r9;
    r3 = r6 * r5 + r3;
    c = ((uint64_t)r1 + r3 + c) >> 32;
    r1 = r1 + r3 + c;


    c = (r8 < r0) ? 1u : 0u;
    r0 = r8 - r0;
    r1 = r12 - r1 - c;

    r0 = (r0 >> 24) | (r1 << 8);
    r1 = (uint32_t)((int32_t)r1 >> 24);
    bq->fx1 = (int32_t)r0;
    bq->fx2 = (int32_t)r1;
    return (int32_t)r0;
}


EXPORT void Biquad_CalcCoeff(int type, float a1, float a2, float a3, float a4,
                             CMusicBiquad *bq) {
    const double two_pi = 6.283185307179586232;
    const double ln2_2 = 0.346573590279972654;
    double w, sharg;
    float wf;
    float s_sin, s_cos, alpha, a, gain_term, shterm;
    float b0, b1, b2, c1, c2, den;

    if (bq == NULL)
        return;




    w = (double)a1 * two_pi / (double)a2;
    wf = (float)w;
    sincosf(wf, &s_sin, &s_cos);
    sharg = (double)a4 * ln2_2 * (double)wf / (double)s_sin;
    shterm = (float)(sinh(sharg) * (double)s_sin);
    a = (float)pow(10.0, (double)(a3 / 40.0f));
    gain_term = __builtin_sqrtf(a) / a4;
    alpha = (float)((double)s_sin / (2.0 * (double)a4));

    bq->fs = a1;
    bq->fc = a2;
    bq->db = a3;
    bq->q = a4;
    bq->type = type;

    if ((unsigned)type > 6)
        return;

    switch (type) {
    case 0:
        den = 1.0f + alpha;
        b0 = (1.0f - s_cos) * 0.5f;
        b1 = 1.0f - s_cos;
        b2 = b0;
        c1 = -2.0f * s_cos;
        c2 = 1.0f - alpha;
        break;
    case 1:
        den = 1.0f + alpha;
        b0 = (1.0f + s_cos) * 0.5f;
        b1 = -(1.0f + s_cos);
        b2 = b0;
        c1 = -2.0f * s_cos;
        c2 = 1.0f - alpha;
        break;
    case 2:
        den = 1.0f + alpha;
        b0 = alpha;
        b1 = 0.0f;
        b2 = -alpha;
        c1 = -2.0f * s_cos;
        c2 = 1.0f - alpha;
        break;
    case 3:
        den = 1.0f + alpha;
        b0 = 1.0f;
        b1 = -2.0f * s_cos;
        b2 = 1.0f;
        c1 = -2.0f * s_cos;
        c2 = 1.0f - alpha;
        break;
    case 4: {
        float t = shterm / a;
        den = 1.0f + t;
        b0 = 1.0f + a * shterm;
        b1 = -2.0f * s_cos;
        b2 = 1.0f - a * shterm;
        c1 = -2.0f * s_cos;
        c2 = 1.0f - t;
        break;
    }
    case 5: {
        float k = s_sin * gain_term;
        float ap1 = a + 1.0f, am1 = a - 1.0f;
        float t = s_cos * am1;
        float u = s_cos * ap1;

        den = ap1 + t + k;
        b0 = a * (ap1 - t + k);
        b1 = (a + a) * (am1 - u);
        b2 = a * (ap1 - t - k);
        c1 = -2.0f * (am1 + u);
        c2 = ap1 + t - k;
        break;
    }
    default: {
        float k = s_sin * gain_term;
        float ap1 = a + 1.0f, am1 = a - 1.0f;
        float t = s_cos * am1;
        float u = s_cos * ap1;

        den = ap1 - t + k;
        b0 = a * (ap1 + t + k);
        b1 = -2.0f * a * (am1 + u);
        b2 = a * (ap1 + t - k);
        c1 = 2.0f * (am1 - u);
        c2 = ap1 - t - k;
        break;
    }
    }

    bq->b2 = b2 / den;
    bq->b1 = b1 / den;
    bq->b0 = b0 / den;
    bq->a1 = c1 / den;
    bq->a2 = c2 / den;
}





typedef struct {
    CMusicBiquad **filters;
    float *scales;
    int count_a;
    int count_b;
    int pad;
} CMusicIirSos;

EXPORT float IirSos_Process(float x, CMusicIirSos *s) {
    const float *scales = s->scales;
    int count = s->count_a;
    int i;

    for (i = 0; i < count; i++) {
        x = x * scales[i];
        x = Biquad_Process(x, s->filters[i]);
    }
    return x * scales[count];
}

EXPORT void IirSos_Process_Buffer(float *buf, int n, CMusicIirSos *s) {
    int i, k;

    for (i = 0; i < n; i++) {
        const float *scales = s->scales;
        int count = s->count_a;
        float x = buf[i];

        for (k = 0; k < count; k++) {
            x = x * scales[k];
            x = Biquad_Process(x, s->filters[k]);
        }
        buf[i] = x * scales[count];
    }
}



EXPORT void IirSos_Process_Buffer_ScaleB0PreMult(float *buf, int n, CMusicIirSos *s) {
    int i, k;

    for (i = 0; i < n; i++) {
        const float *scales = s->scales;
        int count = s->count_a;
        float x = buf[i];

        for (k = 0; k < count; k++) {
            x = x * scales[k];
            x = Biquad_Process_ScaleB0PreMult(x, s->filters[k]);
        }
        buf[i] = x * scales[count];
    }
}

EXPORT CMusicIirSos *IirSos_New(int a, int b) {
    CMusicIirSos *s = (CMusicIirSos *)malloc(sizeof(CMusicIirSos));
    CMusicBiquad **filters;
    int i;

    if (s == NULL)
        return NULL;
    s->count_a = a;
    s->count_b = b;
    s->pad = 0;

    filters = (CMusicBiquad **)malloc((size_t)b * sizeof(CMusicBiquad *));
    s->filters = filters;
    for (i = 0; i < b; i++) {
        filters[i] = Biquad_New();
        if (filters[i] == NULL) {
            int k;

            for (k = 0; k < i; k++) {
                free(filters[k]);
                filters[k] = NULL;
            }


            free(s);
            return NULL;
        }
    }

    s->scales = (float *)malloc(((size_t)b + 1) * sizeof(float));
    if (filters == NULL || s->scales != NULL)
        return s;


    for (i = 0; i < b; i++) {
        if (filters[i] != NULL) {
            free(filters[i]);
            filters[i] = NULL;
        }
    }
    free(filters);
    s->filters = NULL;
    return s;
}

EXPORT void SosIir_Delete_Filters(CMusicIirSos *s)
    __asm__("_Z21SosIir_Delete_FiltersP20CMusicIirSos_State_t");
EXPORT void SosIir_Delete_Filters(CMusicIirSos *s) {
    int i;

    if (s == NULL || s->filters == NULL)
        return;
    for (i = 0; i < s->count_b; i++) {
        if (s->filters[i] != NULL) {
            free(s->filters[i]);
            s->filters[i] = NULL;
        }
    }
    free(s->filters);
    s->filters = NULL;
}

EXPORT void SosIir_Delete_Scales(CMusicIirSos *s)
    __asm__("_Z20SosIir_Delete_ScalesP20CMusicIirSos_State_t");
EXPORT void SosIir_Delete_Scales(CMusicIirSos *s) {
    if (s == NULL || s->scales == NULL)
        return;
    free(s->scales);
    s->scales = NULL;
}





EXPORT void IirSos_Delete(CMusicIirSos *s) {
    int i;

    if (s == NULL)
        return;
    if (s->filters != NULL) {
        for (i = 0; i < s->count_b; i++) {
            if (s->filters[i] != NULL) {
                free(s->filters[i]);
                s->filters[i] = NULL;
            }
        }
        free(s->filters);
        s->filters = NULL;
    }
    if (s->scales != NULL)
        free(s->scales);
    free(s);
}

EXPORT void IirSos_SetFilterCoefs(int index, const float *coefs, CMusicIirSos *s) {
    CMusicBiquad *bq;

    if (s == NULL || s->filters == NULL)
        return;
    bq = s->filters[index];
    if (bq == NULL)
        return;
    Biquad_SetCoefsDirectly(coefs, bq);
}

EXPORT void IirSos_SetFilterScales(const float *src, CMusicIirSos *s) {
    if (s == NULL)
        return;
    memcpy(s->scales, src, ((size_t)s->count_a + 1) * sizeof(float));
}

EXPORT void IirSos_ResetHistory(CMusicIirSos *s) {
    const float zero[6] = { 0, 0, 0, 0, 0, 0 };
    int i;

    if (s == NULL)
        return;
    for (i = 0; i < s->count_b; i++) {
        Biquad_ResetHistory(s->filters[i]);
        Biquad_SetCoefsDirectly(zero, s->filters[i]);
    }
    memset(s->scales, 0, ((size_t)s->count_b + 1) * sizeof(float));
}





typedef struct {
    int32_t value;
    float rem;
} CMusicIntFloat;

EXPORT void CMusicIntFloat_Init(CMusicIntFloat *f) {
    f->value = 0;
    f->rem = 0.0f;
}

EXPORT int32_t CMusicIntFloat_Add(float x, CMusicIntFloat *f) {
    float sum = f->rem + x;
    int32_t ip = (int32_t)sum;

    f->rem = sum - (float)ip;
    f->value += ip;
    return f->value;
}





typedef struct {
    int32_t index;
    int32_t len;
    float *delay;
    float attack;
    float release;
    float env;
    float gain;
    int32_t index2;
    float *scales;
    int32_t m24;
    int32_t ramp_len;
    float ramp_time;
} CMusicLimiter;

EXPORT CMusicLimiter *CMusicLimiter_Init(float attack, float release, int len,
                                         float ramp_time, float fs) {
    CMusicLimiter *s = (CMusicLimiter *)malloc(sizeof(CMusicLimiter));
    int i;

    s->attack = attack;
    s->release = release;
    s->env = 0.0f;
    s->index = 0;
    s->len = len;
    s->gain = 1.0f;

    s->delay = (float *)malloc((size_t)len * sizeof(float));
    memset(s->delay, 0, (size_t)len * sizeof(float));

    s->index2 = 0;
    s->m24 = 0;
    s->scales = (float *)malloc((size_t)len * sizeof(float));
    for (i = 0; i < len; i++)
        s->scales[i] = 1.0f;

    s->ramp_len = (int32_t)(ramp_time * fs);
    s->ramp_time = ramp_time;
    return s;
}

EXPORT void CMusicLimiter_Delete(CMusicLimiter *s) {
    if (s == NULL)
        return;
    if (s->delay != NULL) {
        free(s->delay);
        s->delay = NULL;
    }
    if (s->scales != NULL)
        free(s->scales);
    free(s);
}

EXPORT void CMusicLimiter_Reset(CMusicLimiter *s) {
    int i;

    if (s == NULL)
        return;
    if (s->delay == NULL || s->scales == NULL)
        return;
    memset(s->delay, 0, (size_t)s->len * sizeof(float));
    for (i = 0; i < s->len; i++)
        s->scales[i] = 1.0f;
}


EXPORT void CMusicLimiter_ProcessBuffer(float *buf, int n, float level, CMusicLimiter *s) {
    int i;

    if (n < 1)
        return;

    for (i = 0; i < n; i++) {
        float ax = __builtin_fabsf(buf[i]);
        float env = s->env;
        float coef = vselgt(ax, env, s->attack, s->release);
        float target;
        float out;

        {
            float t = env * (1.0f - coef);

            env = ax * coef + t;
        }
        s->env = env;

        target = fmin_nan(level / env, 1.0f);
        coef = vselgt(s->gain, target, s->attack, s->release);
        {
            float t = s->gain * (1.0f - coef);

            s->gain = target * coef + t;
        }

        s->delay[s->index] = buf[i];
        s->index += 1;
        if (s->index >= s->len)
            s->index = 0;

        out = s->gain * s->delay[s->index];
        buf[i] = out;
    }
}

EXPORT float CMusicLimiter_ProcessSample(float x, float level, CMusicLimiter *s) {
    uint32_t absbits = *(uint32_t *)&x & 0x7fffffffu;
    float ax = *(float *)&absbits;
    float env = s->env;
    float coef = vselgt(ax, env, s->attack, s->release);
    float target;
    int32_t idx;

    {
        float t = env * (1.0f - coef);

        env = ax * coef + t;
    }
    s->env = env;

    target = fmin_nan(level / env, 1.0f);
    coef = vselgt(s->gain, target, s->attack, s->release);
    {
        float t = s->gain * (1.0f - coef);

        s->gain = target * coef + t;
    }

    idx = s->index;
    s->delay[idx] = x;
    idx += 1;
    if (idx >= s->len)
        idx = 0;
    s->index = idx;

    return s->gain * s->delay[idx];
}


EXPORT void CMusicLimiter_ProcessBufferHardLimit(float *buf, int n, float level,
                                                 CMusicLimiter *s) {
    int i;

    if (n < 1)
        return;

    for (i = 0; i < n; i++) {
        float ax = __builtin_fabsf(buf[i]);
        int32_t j = s->index2;
        int32_t idx = j - 1;
        float target, sc;

        s->env = ax;
        if (j <= 0)
            idx += s->len;
        s->m24 = idx;

        target = fmin_nan(level / ax, 1.0f);
        sc = s->scales[idx];

        if (target < sc) {


            float step = (target - s->gain) / (float)s->len;
            int k;

            for (k = 0; k < s->len; k++) {
                int32_t back = s->index2 - k;
                int32_t w;
                float val;

                if (back < 0)
                    back += s->len;
                w = back;
                val = -step * (float)k + target;
                if (w == s->index2 || val < s->scales[w])
                    s->scales[w] = val;
            }
        } else if (target > sc) {
            s->scales[s->index2] = sc + (1.0f - sc) / (float)s->ramp_len;
        } else {
            s->scales[s->index2] = sc;
        }

        j = s->index2 + 1;
        if (j >= s->len)
            j = 0;
        s->index2 = j;
        s->gain = s->scales[j];

        s->delay[s->index] = buf[i];
        s->index += 1;
        if (s->index >= s->len)
            s->index = 0;
        buf[i] = s->gain * s->delay[s->index];
    }
}





typedef struct {
    float *buf_a;
    float *buf_b;
    float f08;
    float f0c;
    int32_t pos;
    float frac;
    int32_t inpos;
    float ifrac;
    float inv_ratio;
    float ratio;
    float inv_ratio2;
    int32_t len2;
    float *out;
    float o34;
    float o38;
    int32_t out_len;
    int32_t out_size;
    float *hist;
    int32_t flag;
    int32_t out_len2;
} CMusicResampler;


#define K_SUM23A (-0.24618500471115112f)
#define K_SUM14A (0.24614027142524719f)
#define K_DIF32A (-0.36030924320220947f)
#define K_DIF41A (0.10174985975027084f)
#define K_DIF41B (0.17577925324440002f)
#define K_DIF32B (0.48068025708198547f)
#define K_SUM14B (0.041314020752906799f)
#define K_SUM23B (0.45868971943855286f)


EXPORT float CMusicResampler_Interp(float a0, float a1, float a2, float a3, float a4)
    __asm__("_Z22CMusicResampler_Interpfffff");
EXPORT float CMusicResampler_Interp(float a0, float a1, float a2, float a3, float a4) {
    float sum23 = a2 + a3;
    float dif32 = a3 - a2;
    float sum14 = a1 + a4;
    float dif41 = a4 - a1;
    float t = a0 - 0.5f;
    float p1 = sum23 * K_SUM23A;
    float p2 = dif32 * K_DIF32A;
    float p3 = dif41 * K_DIF41B;
    float p4 = sum14 * K_SUM14B;

    p1 = sum14 * K_SUM14A + p1;
    p2 = dif41 * K_DIF41A + p2;
    p3 = dif32 * K_DIF32B + p3;
    p4 = sum23 * K_SUM23B + p4;
    p1 = t * p2 + p1;
    p3 = t * p1 + p3;
    return t * p3 + p4;
}

EXPORT CMusicResampler *CMusicResampler_Init(float out_rate, float in_rate) {
    CMusicResampler *s = (CMusicResampler *)malloc(sizeof(CMusicResampler));
    float ratio = in_rate / out_rate;
    float inv = 1.0f / ratio;
    int32_t size = (int32_t)__builtin_ceilf(ratio + ratio);

    s->ratio = ratio;
    s->inv_ratio = inv;
    s->len2 = size;
    s->out = NULL;

    s->buf_a = (float *)malloc((size_t)size * sizeof(float));
    memset(s->buf_a, 0, (size_t)size * sizeof(float));
    s->buf_b = (float *)malloc((size_t)size * sizeof(float));
    memset(s->buf_b, 0, (size_t)size * sizeof(float));

    s->inv_ratio2 = inv;
    s->f08 = 0.0f;
    s->f0c = 0.0f;
    s->pos = 0;
    s->frac = 0.0f;
    s->inpos = 0;
    s->ifrac = 0.0f;
    s->o34 = 0.0f;
    s->o38 = 0.0f;
    s->out_len = 0;
    s->out_size = 0;
    s->hist = NULL;
    s->flag = 0;
    s->out_len2 = 0;
    return s;
}

EXPORT void CMusicResampler_Delete(CMusicResampler *s) {
    if (s == NULL)
        return;
    if (s->buf_a != NULL) {
        free(s->buf_a);
        s->buf_a = NULL;
    }
    if (s->buf_b != NULL) {
        free(s->buf_b);
        s->buf_b = NULL;
    }
    if (s->out != NULL) {
        free(s->out);
        s->out = NULL;
    }
    if (s->hist != NULL)
        free(s->hist);
    free(s);
}

EXPORT void CMusicResampler_Reset(CMusicResampler *s) {
    memset(s->buf_a, 0, (size_t)s->len2 * sizeof(float));
    s->flag = 0;
    s->f08 = 0.0f;
    s->f0c = 0.0f;
    s->pos = 0;
    s->frac = 0.0f;
    s->inpos = 0;
    s->ifrac = 0.0f;
}


static void cmr_fill(float *buf, int n, float unused, CMusicResampler *s) {
    int32_t out_len = (int32_t)__builtin_ceilf(s->inv_ratio2 * (float)n);
    int32_t size = out_len + 2;
    int32_t len2 = s->len2;
    float *tmp;
    int32_t i;
    int32_t save_pos;
    float save_frac;
    float frac;

    (void)unused;

    s->out_len = out_len;
    s->out_size = size;
    if (s->out == NULL)
        s->out = (float *)malloc((size_t)size * sizeof(float));
    memset(s->out, 0, (size_t)size * sizeof(float));
    if (s->hist == NULL)
        s->hist = (float *)malloc((size_t)s->out_size * sizeof(float));

    tmp = (float *)malloc((size_t)n * sizeof(float));
    memcpy(tmp, s->buf_a, (size_t)len2 * sizeof(float));
    memcpy(tmp + len2, buf, (size_t)(n - len2) * sizeof(float));
    memcpy(s->buf_a, buf + (n - len2), (size_t)len2 * sizeof(float));

    i = 0;
    for (;;) {
        frac = s->frac;
        if (frac == 0.0f) {
            s->out[i] = tmp[(uint32_t)s->pos % (uint32_t)n];
            frac = s->frac;
        }
        frac = s->ratio + frac;
        {
            int32_t ip = (int32_t)frac;
            uint32_t p;

            s->frac = frac - (float)ip;
            p = (uint32_t)s->pos + (uint32_t)ip;
            if (p < (uint32_t)n)
                s->pos = (int32_t)p;
            else
                s->pos = (int32_t)(p - (uint32_t)n);
        }
        i += 1;
        if (i >= s->out_len)
            break;
    }



    save_pos = s->pos;
    save_frac = s->frac;
    frac = s->frac;

    if (frac == 0.0f) {
        s->out[s->out_len] = s->buf_a[(uint32_t)s->pos % (uint32_t)len2];
        frac = s->frac;
    }
    frac = s->ratio + frac;
    {
        int32_t ip = (int32_t)frac;
        uint32_t p;

        frac = frac - (float)ip;
        s->frac = frac;
        p = (uint32_t)s->pos + (uint32_t)ip;
        s->pos = (p < (uint32_t)n) ? (int32_t)p : (int32_t)(p - (uint32_t)n);
    }





    if (frac == 0.0f) {
        s->out[s->out_len + 1] = s->buf_a[(uint32_t)s->pos % (uint32_t)len2];
        frac = s->frac;
    }
    frac = s->ratio + frac;
    {
        int32_t ip = (int32_t)frac;
        uint32_t p;

        frac = frac - (float)ip;
        s->frac = frac;
        p = (uint32_t)s->pos + (uint32_t)ip;
        s->pos = (p < (uint32_t)n) ? (int32_t)p : (int32_t)(p - (uint32_t)n);
    }

    s->pos = save_pos;
    s->frac = save_frac;
    s->f08 = tmp[n - 1];

    if (s->flag == 0) {
        s->out_len2 = s->out_size;
    } else {
        s->out_len2 = s->out_len;
        memmove(s->out + 2, s->out, (size_t)s->out_len * sizeof(float));
    }
    memcpy(s->hist, s->out, (size_t)s->out_size * sizeof(float));

    free(tmp);
}

EXPORT void CMusicResampler_DownSample(float *buf, int n, float unused, CMusicResampler *s) {
    cmr_fill(buf, n, unused, s);
}

EXPORT void CMusicResampler_UpSample1(float *buf, int n, float unused, CMusicResampler *s) {
    cmr_fill(buf, n, unused, s);
}


EXPORT void CMusicResampler_UpSample(float *out, int n, CMusicResampler *s) {
    float c = 0.0f;
    float d = 0.0f;
    int i;

    if (s->flag == 0) {
        s->flag = 1;
    } else {
        memmove(s->out + 2, s->out, (size_t)s->out_len * sizeof(float));
        s->out[0] = s->o34;
        s->out[1] = s->o38;
    }

    for (i = 0; i < n; i++) {
        int32_t idx = (int32_t)((uint32_t)s->inpos % (uint32_t)s->out_len);
        float b = s->out[idx];
        float a = (idx <= 0) ? s->f0c : s->out[idx - 1];
        float frac = s->ifrac;

        if (frac == 0.0f) {
            out[i] = b;
        } else {
            float t, sum23, dif32, sum14, dif41, p1, p2, p3, p4;

            if (idx + 2 < s->out_size) {
                c = s->out[idx + 1];
                d = s->out[idx + 2];
            }

            t = frac - 0.5f;
            sum23 = b + c;
            dif32 = c - b;
            sum14 = a + d;
            dif41 = d - a;

            p1 = sum23 * K_SUM23A;
            p2 = dif32 * K_DIF32A;
            p3 = dif41 * K_DIF41B;
            p4 = sum14 * K_SUM14B;

            p1 = sum14 * K_SUM14A + p1;
            p2 = dif41 * K_DIF41A + p2;
            p3 = dif32 * K_DIF32B + p3;
            p4 = sum23 * K_SUM23B + p4;
            p1 = t * p2 + p1;
            p3 = t * p1 + p3;
            out[i] = t * p3 + p4;
        }

        {
            float v = s->ifrac + s->inv_ratio;
            int32_t ip = (int32_t)v;

            s->ifrac = v - (float)ip;
            s->inpos += ip;
            if (s->inpos >= s->out_len)
                s->inpos -= s->out_len;
        }
    }

    s->f0c = s->out[s->out_len - 1];
    s->o34 = s->out[s->out_size - 2];
    s->o38 = s->out[s->out_size - 1];
    if (s->out != NULL) {
        free(s->out);
        s->out = NULL;
    }
    if (s->hist != NULL) {
        free(s->hist);
        s->hist = NULL;
    }
}





typedef struct {
    int32_t enable;
    CMusicResampler *rs;
    CMusicIirSos *iir;
    CMusicLimiter *lim;
} Mi_Resampler;



static const double kLpfCoefs[8][6] = {
    { 1.0, -0.99274218381261503, 1.0, 1.0, -1.1095418149015399, 0.93121102491488295 },
    { 1.0, -0.93253755244810199, 1.0, 1.0, -0.99614920299213805, 0.80055649095412396 },
    { 1.0, -0.80000792577823698, 1.0, 1.0, -0.84750210513518198, 0.66917119326622898 },
    { 1.0, -0.56766125607330198, 1.0, 1.0, -0.66096678877591497, 0.52942608962638704 },
    { 1.0, -0.18788425326316899, 1.0, 1.0, -0.44012459750411098, 0.37966986944827302 },
    { 1.0, 0.400031703741137, 1.0, 1.0, -0.203022606468063, 0.22795275231286999 },
    { 1.0, 1.1928592812111101, 1.0, 1.0, 0.0097762596691694896, 0.096110527880399296 },
    { 1.0, 1.88794144304485, 1.0, 1.0, 0.139750414081905, 0.016865834036990799 },
};


static const float kLpfScales[9] = {
    0.81574863195419312f, 0.75356966257095337f, 0.68472874164581299f,
    0.60632258653640747f, 0.51847970485687256f, 0.42704859375953674f,
    0.34636253118515015f, 0.29748809337615967f, 1.0f,
};

EXPORT void Mi_Resampler_LPF_Init(Mi_Resampler *self)
    __asm__("_ZN12Mi_Resampler29CMiSoundFX_Resampler_LPF_InitEv");

static void Mi_Resampler_Init(Mi_Resampler *self, float out_rate, float in_rate) {
    float inv_time, e_attack, e_release;
    int len;

    self->enable = 1;
    self->rs = CMusicResampler_Init(out_rate, in_rate);
    Mi_Resampler_LPF_Init(self);

    inv_time = out_rate * 0.000199999995f;
    e_attack = expf((float)(-2.2 / (double)inv_time));
    inv_time = out_rate * 0.400000006f;
    e_release = expf((float)(-2.2 / (double)inv_time));
    len = (int)((double)out_rate * 0.005 + 1.0);
    self->lim = CMusicLimiter_Init(1.0f - e_attack, 1.0f - e_release, len,
                                   0.1f, out_rate);
}

EXPORT void Mi_Resampler_Ctor(Mi_Resampler *self, float out_rate, float in_rate)
    __asm__("_ZN12Mi_ResamplerC1Eff");
EXPORT void Mi_Resampler_Ctor(Mi_Resampler *self, float out_rate, float in_rate) {
    Mi_Resampler_Init(self, out_rate, in_rate);
}

EXPORT void Mi_Resampler_Ctor2(Mi_Resampler *self, float out_rate, float in_rate)
    __asm__("_ZN12Mi_ResamplerC2Eff");
EXPORT void Mi_Resampler_Ctor2(Mi_Resampler *self, float out_rate, float in_rate) {
    Mi_Resampler_Init(self, out_rate, in_rate);
}

EXPORT void Mi_Resampler_LPF_Init(Mi_Resampler *self)
    __asm__("_ZN12Mi_Resampler29CMiSoundFX_Resampler_LPF_InitEv");

EXPORT void Mi_Resampler_LPF_Init(Mi_Resampler *self) {
    int i;

    self->iir = IirSos_New(8, 8);
    for (i = 0; i < self->iir->count_a; i++) {
        float coefs[6];
        int k;

        for (k = 0; k < 6; k++)
            coefs[k] = (float)kLpfCoefs[i][k];
        IirSos_SetFilterCoefs(i, coefs, self->iir);
    }
    IirSos_SetFilterScales(kLpfScales, self->iir);
}

EXPORT void Mi_Resampler_Dtor(Mi_Resampler *self) __asm__("_ZN12Mi_ResamplerD1Ev");
EXPORT void Mi_Resampler_Dtor(Mi_Resampler *self) {
    if (self->iir != NULL) {
        IirSos_Delete(self->iir);
        self->iir = NULL;
    }
    if (self->rs != NULL) {
        CMusicResampler_Delete(self->rs);
        self->rs = NULL;
    }
    CMusicLimiter_Delete(self->lim);
}

EXPORT void Mi_Resampler_Dtor2(Mi_Resampler *self) __asm__("_ZN12Mi_ResamplerD2Ev");
EXPORT void Mi_Resampler_Dtor2(Mi_Resampler *self) {
    Mi_Resampler_Dtor(self);
}

EXPORT void Mi_Resampler_setEnable(Mi_Resampler *self, int enable)
    __asm__("_ZN12Mi_Resampler9setEnableEi");
EXPORT void Mi_Resampler_setEnable(Mi_Resampler *self, int enable) {
    self->enable = enable;
}

EXPORT unsigned Mi_Resampler_getLenOut(Mi_Resampler *self)
    __asm__("_ZN12Mi_Resampler9getLenOutEv");
EXPORT unsigned Mi_Resampler_getLenOut(Mi_Resampler *self) {
    return (unsigned)self->rs->out_len;
}

EXPORT unsigned Mi_Resampler_getLenOutN(Mi_Resampler *self, unsigned n)
    __asm__("_ZN12Mi_Resampler9getLenOutEj");
EXPORT unsigned Mi_Resampler_getLenOutN(Mi_Resampler *self, unsigned n) {
    return (unsigned)(self->rs->inv_ratio2 * (float)n);
}


EXPORT void Mi_Resampler_process(Mi_Resampler *self, float *out, float *tmp,
                                 unsigned *len_out, unsigned n)
    __asm__("_ZN12Mi_Resampler7processEPfS0_Pjj");
EXPORT void Mi_Resampler_process(Mi_Resampler *self, float *out, float *tmp,
                                 unsigned *len_out, unsigned n) {
    unsigned produced, i;

    if (self->enable == 0)
        return;

    IirSos_Process_Buffer(tmp, (int)n, self->iir);
    CMusicResampler_DownSample(tmp, (int)n, 1.0f, self->rs);

    produced = (unsigned)self->rs->out_len;
    *len_out = produced;
    memcpy(out, self->rs->out, (size_t)produced * sizeof(float));

    for (i = 0; i < produced; i++)
        out[i] = out[i] + out[i];

    CMusicLimiter_ProcessBufferHardLimit(out, (int)produced, 0.99f, self->lim);
}





typedef struct {
    int32_t enable;
    int32_t channels;
    int32_t f08;
    float f0c;
    float f10;
    int32_t bw_in;
    int32_t bw_out;
    Mi_Resampler **chan;
    float scale_in;
    float scale_out;
} Mi_Resampler_Fx;




static void Mi_Resampler_Fx_Ctor(Mi_Resampler_Fx *self, int channels, float f0c,
                                 float bw_in, int32_t f08, float f10,
                                 float bw_out) {
    int i;

    self->f0c = f0c;
    self->channels = channels;
    self->bw_in = (int)bw_in;
    self->enable = 1;
    self->bw_out = (int)bw_out;
    self->f10 = f10;
    self->f08 = f08;

    self->chan = (Mi_Resampler **)malloc((size_t)(unsigned)channels *
                                         sizeof(Mi_Resampler *));
    for (i = 0; i < channels; i++) {
        Mi_Resampler *c = (Mi_Resampler *)malloc(sizeof(Mi_Resampler));

        Mi_Resampler_Ctor(c, self->f10, self->f0c);
        self->chan[i] = c;
    }

    self->scale_in = 4.6566128730773926e-10f;
    self->scale_out = 2147483648.0f;
}

EXPORT void Mi_Resampler_Fx_C1(Mi_Resampler_Fx *self, int channels, float f0c,
                               float bw_in, int32_t f08, float f10,
                               float bw_out)
    __asm__("_ZN15Mi_Resampler_FxC1Eiffiff");
EXPORT void Mi_Resampler_Fx_C1(Mi_Resampler_Fx *self, int channels, float f0c,
                               float bw_in, int32_t f08, float f10,
                               float bw_out) {
    Mi_Resampler_Fx_Ctor(self, channels, f0c, bw_in, f08, f10, bw_out);
}

EXPORT void Mi_Resampler_Fx_C2(Mi_Resampler_Fx *self, int channels, float f0c,
                               float bw_in, int32_t f08, float f10,
                               float bw_out)
    __asm__("_ZN15Mi_Resampler_FxC2Eiffiff");
EXPORT void Mi_Resampler_Fx_C2(Mi_Resampler_Fx *self, int channels, float f0c,
                               float bw_in, int32_t f08, float f10,
                               float bw_out) {
    Mi_Resampler_Fx_Ctor(self, channels, f0c, bw_in, f08, f10, bw_out);
}

static void Mi_Resampler_Fx_Dtor(Mi_Resampler_Fx *self) {
    int i;

    for (i = 0; i < self->channels; i++) {
        if (self->chan[i] != NULL) {
            Mi_Resampler_Dtor(self->chan[i]);
            free(self->chan[i]);
        }
    }
    if (self->chan != NULL)
        free(self->chan);
}

EXPORT void Mi_Resampler_Fx_D1(Mi_Resampler_Fx *self)
    __asm__("_ZN15Mi_Resampler_FxD1Ev");
EXPORT void Mi_Resampler_Fx_D1(Mi_Resampler_Fx *self) {
    Mi_Resampler_Fx_Dtor(self);
}

EXPORT void Mi_Resampler_Fx_D2(Mi_Resampler_Fx *self)
    __asm__("_ZN15Mi_Resampler_FxD2Ev");
EXPORT void Mi_Resampler_Fx_D2(Mi_Resampler_Fx *self) {
    Mi_Resampler_Fx_Dtor(self);
}

EXPORT void Mi_Resampler_Fx_setEnable(Mi_Resampler_Fx *self, int enable)
    __asm__("_ZN15Mi_Resampler_Fx9setEnableEi");
EXPORT void Mi_Resampler_Fx_setEnable(Mi_Resampler_Fx *self, int enable) {
    int i;

    self->enable = enable;
    for (i = 0; i < self->channels; i++)
        Mi_Resampler_setEnable(self->chan[i], self->enable);
}

EXPORT int Mi_Resampler_Fx_getFrameLen(Mi_Resampler_Fx *self)
    __asm__("_ZN15Mi_Resampler_Fx11getFrameLenEv");
EXPORT int Mi_Resampler_Fx_getFrameLen(Mi_Resampler_Fx *self) {
    Mi_Resampler *c = self->chan[0];

    return (c != NULL) ? (int)Mi_Resampler_getLenOut(c) : 0;
}



EXPORT void Mi_Resampler_Fx_CharToInt(Mi_Resampler_Fx *self, const char *src,
                                      int count, int32_t *dst)
    __asm__("_ZN15Mi_Resampler_Fx9CharToIntEPciPi");
EXPORT void Mi_Resampler_Fx_CharToInt(Mi_Resampler_Fx *self, const char *src,
                                      int count, int32_t *dst) {
    int i;

    __android_log_print(LOG_PRIO_ERROR, LOG_TAG,
                        "CharToInt size is %d bw_in is %d", count, self->bw_in);

    if (self->bw_in == 16) {
        for (i = 0; i < count; i++)
            dst[i] = (int32_t)((uint32_t)(uint16_t)(
                ((const uint16_t *)src)[i]) << 16);
    } else if (self->bw_in == 24) {
        for (i = 0; i < count; i++) {
            uint32_t v;

            memcpy(&v, src + 3 * (size_t)i, 4);
            dst[i] = (int32_t)((v << 8) & 0xffff0000u);
        }
    }
}


EXPORT void Mi_Resampler_Fx_IntToChar(Mi_Resampler_Fx *self, const int32_t *src,
                                      int count, char *dst)
    __asm__("_ZN15Mi_Resampler_Fx9IntToCharEPiiPc");
EXPORT void Mi_Resampler_Fx_IntToChar(Mi_Resampler_Fx *self, const int32_t *src,
                                      int count, char *dst) {
    int i;

    __android_log_print(LOG_PRIO_ERROR, LOG_TAG,
                        "IntToChar size is %d bw_out is %d", count,
                        self->bw_out);

    if (count < 1 || self->bw_out != 16)
        return;
    for (i = 0; i < count; i++)
        ((uint16_t *)dst)[i] = (uint16_t)((uint32_t)src[i] >> 16);
}


EXPORT void Mi_Resampler_Fx_process(Mi_Resampler_Fx *self, int **out, int **in,
                                    unsigned *frame_len, unsigned n)
    __asm__("_ZN15Mi_Resampler_Fx7processEPPiS1_Pjj");
EXPORT void Mi_Resampler_Fx_process(Mi_Resampler_Fx *self, int **out, int **in,
                                    unsigned *frame_len, unsigned n) {
    unsigned i;

    __android_log_print(LOG_PRIO_ERROR, LOG_TAG, "process");

    if (self->enable == 0)
        return;
    if (self->channels <= 0)
        return;

    for (i = 0; i < (unsigned)self->channels; i++) {
        Mi_Resampler *c = self->chan[i];
        unsigned produced;
        unsigned k;
        unsigned predicted = Mi_Resampler_getLenOutN(c, n);
        float *fbuf = (float *)malloc(n * sizeof(float));
        int32_t *ibuf = (int32_t *)malloc((size_t)predicted * sizeof(int32_t));

        for (k = 0; k < n; k++)
            fbuf[k] = (float)in[i][k] * self->scale_in;

        Mi_Resampler_process(c, (float *)ibuf, fbuf, frame_len, n);
        produced = frame_len[0];

        for (k = 0; k < produced; k++)
            out[i][k] = (int32_t)(((float *)ibuf)[k] * self->scale_out);

        free(fbuf);
        free(ibuf);
    }
}





EXPORT void *mi_resampler_init(int channels, float in_rate, float bw_in,
                               int32_t f08, float out_rate, float bw_out) {
    Mi_Resampler_Fx *self =
        (Mi_Resampler_Fx *)malloc(sizeof(Mi_Resampler_Fx));



    Mi_Resampler_Fx_C1(self, channels, in_rate, bw_in, f08, out_rate, bw_out);
    return self;
}

EXPORT void mi_resampler_delete(void *handle) {
    if (handle == NULL)
        return;

    Mi_Resampler_Fx_D1((Mi_Resampler_Fx *)handle);
    free(handle);
}





EXPORT void mi_resampler_process(int **out, int **in, unsigned *frame_len,
                                 unsigned n, void *handle) {
    Mi_Resampler_Fx_process((Mi_Resampler_Fx *)handle, out, in, frame_len, n);
}

EXPORT void mi_resampler_CharToInt(char *src, int count, int *dst,
                                   void *handle) {
    Mi_Resampler_Fx_CharToInt((Mi_Resampler_Fx *)handle, src, count,
                              (int32_t *)dst);
}

EXPORT void mi_resampler_IntToChar(int *src, int count, char *dst,
                                   void *handle) {
    Mi_Resampler_Fx_IntToChar((Mi_Resampler_Fx *)handle, (const int32_t *)src,
                              count, dst);
}
