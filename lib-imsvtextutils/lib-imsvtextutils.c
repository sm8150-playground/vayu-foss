



typedef struct { unsigned long __opaque[7]; } pthread_rwlock_t;

extern pthread_rwlock_t log_rw_mutex;
extern volatile unsigned short logPriorityValue;
extern volatile unsigned char bInit_Success;
extern volatile unsigned short adbLogPropertyValue;

extern void msg_send_2(const void *msg, int pid, int tid);

extern int pthread_rwlock_rdlock(pthread_rwlock_t *lock);
extern int pthread_rwlock_unlock(pthread_rwlock_t *lock);
extern int getpid(void);
extern int gettid(void);
extern int __android_log_print(int prio, const char *tag, const char *fmt, ...);

#define EXPORT __attribute__((visibility("default")))



EXPORT const char *VertexShader =
    "#version 300 es \n"
    "uniform float rad;\n"
    "out vec2 texCord;\n"
    "void main() {    \n"
    "mat4 RotationMatrix = mat4(cos(rad), sin(rad), 0.0, 0.0,\n"
    "-sin(rad), cos(rad), 0.0, 0.0, \n"
    "0.0, 0.0, 1.0, 0.0,             \n"
    "0.0, 0.0, 0.0, 1.0);             \n"
    "vec4 vTmpPos; \n"
    "vTmpPos.x = float(gl_VertexID % 2) * 4.0 - 1.0;\n"
    "vTmpPos.y = float(gl_VertexID / 2) * 4.0 - 1.0;\n"
    "vTmpPos.z = 0.0;\n"
    "vTmpPos.w = 1.0;\n"
    "gl_Position = RotationMatrix * vTmpPos;\n"
    "texCord.y = float(gl_VertexID / 2) * 2.0;\n"
    "texCord.x = float(gl_VertexID % 2) * 2.0;\n"
    "} \n";

EXPORT const char *FragmentShader =
    "#version 300 es \n"
    "#extension GL_EXT_YUV_target : require\n"
    " precision highp float; \n"
    " precision highp int; \n"
    " uniform highp __samplerExternal2DY2YEXT u_sTexture; \n"
    " layout (yuv) out vec4 o_vFragColor;\n"
    " in vec2 texCord; // varying texture coordinate\n"
    " void main()\n"
    " {\n"
    "    o_vFragColor = texture( u_sTexture, texCord );\n"
    "} \n";



struct svt_msg_const {
    unsigned int id;
    unsigned int field;
    const char *fmt;
    const char *file;
};

static const struct svt_msg_const setup_shader_msg = {
    0x0033015du,
    4u,
    "[SIMSVT] %d %d \"setup shader\"",
    "vendor/qcom/proprietary/commonsys/ims/vtextutils/src/qpImsVTGpuUtil.cpp"
};

#define SVT_TAG "SIMSVT"




static __attribute__((noinline)) int rotateFrame_body(void)
{
    return 0;
}


static __attribute__((noinline)) void setupEGL_body(void)
{
}


static __attribute__((noinline)) void destroyEGL_body(void)
{
}


static __attribute__((noinline)) void setupShader_body(void)
{
    int pid;
    int tid;

    if ((logPriorityValue & 0x10u) == 0u)
        return;

    if (bInit_Success != 0u) {
        pthread_rwlock_rdlock(&log_rw_mutex);
        pid = getpid();
        tid = gettid();
        msg_send_2(&setup_shader_msg, pid, tid);
        pthread_rwlock_unlock(&log_rw_mutex);
    }

    if (adbLogPropertyValue != 0u)
        __android_log_print(6, SVT_TAG, "setup shader");
}

EXPORT int rotateFrame(void)
{
    return rotateFrame_body();
}


#define CFI_JUMP_TABLE __attribute__((section(".text.cfijt")))

EXPORT CFI_JUMP_TABLE void setupEGL(void)
{
    setupEGL_body();
}

EXPORT CFI_JUMP_TABLE void setupShader(void)
{
    setupShader_body();
}

EXPORT CFI_JUMP_TABLE void destroyEGL(void)
{
    destroyEGL_body();
}



static __attribute__((noinline)) void cfi_check_fail(const void *diag)
{
    if (diag == 0)
        goto trap;
    if (*(const volatile unsigned char *)diag < 5u)
        goto trap;
    return;
trap:
    __asm__ __volatile__("brk #0x5502");
}


extern void cfi_addr_rotateFrame(void)
    __attribute__((alias("rotateFrame"), visibility("hidden")));
extern void cfi_addr_setupEGL(void)
    __attribute__((alias("setupEGL"), visibility("hidden")));

EXPORT void __cfi_check(unsigned long type_id, const void *addr, const void *diag)
{
    unsigned long diff;

    if (type_id == 0x82FB293FAE840CCCUL) {
        if (addr == (const void *)&cfi_addr_rotateFrame)
            return;
    } else if (type_id == 0x7E04A0FB7AD8BCD5UL) {
        diff = (unsigned long)addr - (unsigned long)&cfi_addr_setupEGL;
        diff = (diff >> 3) | (diff << 61);
        if (diff < 3UL)
            return;
    }
    cfi_check_fail(diag);
}
