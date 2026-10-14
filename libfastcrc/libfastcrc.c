

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <log/log.h>

#include "libfastcrc.h"

#define CRC_TABLE_SIZE 256
#define MISR_ENTRIES   8
#define REC_SIZE       0x12c
#define NAME_SIZE      0x12c
#define PENDING_OFF    0x160




typedef struct {
    int      frameNo;
    int      width;
    int      height;
    int      cropWidth;
    int      cropHeight;
    int      cropX;
    int      cropY;
    int      bitDepthY;
    int      bitDepthC;
    int      count;
    uint32_t dpbLuma[MISR_ENTRIES];
    uint32_t dpbChroma[MISR_ENTRIES];
    uint32_t opbLuma[MISR_ENTRIES];
    uint32_t opbChroma[MISR_ENTRIES];
    int      count2;
    uint32_t dpbLuma2[MISR_ENTRIES];
    uint32_t dpbChroma2[MISR_ENTRIES];
    uint32_t opbLuma2[MISR_ENTRIES];
    uint32_t opbChroma2[MISR_ENTRIES];
} MisrRecord;


typedef struct MisrFrame {
    int       reserved[9];
    int       flags;
    MisrRecord rec;
} MisrFrame;


typedef struct CrcFrame {
    int reserved0;
    int reserved1;
    int width;
    int height;
    int reserved10;
    int stride;
    int cropX;
    int cropY;
    int reserved20;
} CrcFrame;

typedef struct VideoCRCChecker {
    char         name[NAME_SIZE];
    FILE        *fp;
    int          mode;
    const char  *tag;
    union {
        struct {
            uint32_t unused;
            uint32_t poly;
            uint32_t table[CRC_TABLE_SIZE];
        } crc;
        unsigned char misrRec[REC_SIZE];
    } u;
    uint32_t     prevCRC;
    uint32_t     pendingCRC;
    uint32_t     totalCRC;
    uint32_t     checkStarted;
    uint32_t     crcCheckMode;
    uint32_t     writeMode;
    uint32_t     skipOpbMisr;
    uint32_t     duplicateMisr;
    void        *ops[8];
} VideoCRCChecker;



static const uint32_t kCrcPoly = 0x04c11db7u;

static const char *const kCrcFileNames[4] = {
    ".fcrc", "CrcFile", ".misr", "MisrFile",
};

#define ALWAYS_INLINE static inline __attribute__((always_inline))

ALWAYS_INLINE uint32_t crc_step(uint32_t crc, const uint32_t *table,
                                uint32_t byte)
{
    return (crc << 8) ^ table[(byte ^ (crc >> 24)) & 0xffu];
}

static void build_crc_table(VideoCRCChecker *chk)
{
    uint32_t i, k, c;

    chk->u.crc.poly = kCrcPoly;
    for (i = 0; i < CRC_TABLE_SIZE; i++) {
        c = i << 24;
        for (k = 0; k < 8; k++)
            c = (c & 0x80000000u) ? ((c << 1) ^ kCrcPoly) : (c << 1);
        chk->u.crc.table[i] = c;
    }
}


static int depth_to_index(int depth)
{
    if (depth == 8)
        return 0;
    if (depth == 9)
        return 1;
    if (depth == 10)
        return 2;
    return 3;
}

static int index_to_depth(int index)
{
    if (index == 0)
        return 8;
    if (index == 1)
        return 9;
    if (index == 2)
        return 10;
    return 3;
}




__attribute__((visibility("hidden")))
int VideoCRCCheckerCheckMISR(struct VideoCRCChecker *, unsigned char *,
                             struct MisrFrame *, int, uint32_t *);

int VideoCRCCheckerInit(VideoCRCChecker *chk, int writeMode, int mode)
{
    if (chk == NULL)
        return 3;

    memset(chk, 0, sizeof(*chk));

    chk->mode = mode;
    chk->tag = "FCRC";
    chk->pendingCRC = 0;
    chk->totalCRC = 0;
    chk->checkStarted = 0;
    chk->ops[0] = (void *)&VideoCRCCheckerConfigure;
    chk->ops[1] = (void *)&VideoCRCCheckerDestroy;
    chk->ops[2] = (void *)&VideoCRCCheckerSetCRCCheckMode;
    chk->writeMode = (uint32_t)writeMode;

    if (mode == 0) {
        build_crc_table(chk);
        chk->prevCRC = 0;
        chk->pendingCRC = 0;
        chk->ops[3] = (void *)&VideoCRCCheckerGetPendingCRCCount;
        chk->ops[4] = (void *)&VideoCRCCheckerCheckCRCofYUVframe;
        chk->ops[5] = (void *)&VideoCRCCheckerSkipFrameCRCCheck;
        chk->ops[6] = (void *)&VideoCRCCheckerTotalCRCCount;
        chk->ops[7] = (void *)&VideoCRCCheckerGetPrevCRC;
        return 0;
    }
    if (mode != 1)
        return 3;

    chk->ops[3] = (void *)&VideoCRCCheckerGetPendingMISRCount;
    chk->ops[4] = (void *)&VideoCRCCheckerCheckMISR;
    chk->ops[5] = (void *)&VideoCRCCheckerSkipFrameMISRCheck;
    chk->ops[6] = (void *)&VideoCRCCheckerTotalMISRCount;
    chk->ops[7] = (void *)&VideoCRCCheckerGetPrevMISR;
    memset(chk->u.misrRec, 0, sizeof(chk->u.misrRec));
    chk->tag = "MISR";
    return 0;
}

int VideoCRCCheckerConfigure(VideoCRCChecker *chk, const char *dir)
{
    char path[600];
    FILE *fp;

    memset(path, 0, sizeof(path));

    if (dir == NULL) {
        ALOGE("Unable to open CRC file %s", path);
        return 3;
    }
    strlcpy(path, dir, sizeof(path));
    strlcat(path, chk->name, sizeof(path));

    if (chk->writeMode != 0) {
        fp = fopen(path, "w");
        chk->fp = fp;
        if (fp == NULL) {
            ALOGE("Unable to open CRC file %s", path);
            return 3;
        }
        if (chk->mode == 1) {
            static const char hdr[] =
                "Displaying Order Frame No, DPB Luma Misr, DPB Chroma Misr, "
                "OPB Luma Misr, OPB Chroma Misr, width, height, crop_width, "
                "crop_height, crop_x, crop_y, bitdepthY, bitdepthC\n";
            fwrite(hdr, sizeof(hdr) - 1, 1, fp);
        }
        return 0;
    }

    fp = fopen(path, "r");
    chk->fp = fp;
    if (fp == NULL) {
        ALOGE("Unable to open ref CRC file %s (%s) (%s)", path, dir, chk->name);
        return 3;
    }

    if (chk->mode != 0) {
        char scratch[0x100];
        memset(scratch, 0, sizeof(scratch));
        fgets(scratch, sizeof(scratch), fp);
        return 0;
    }

    fseeko(fp, 0, SEEK_END);
    if (ftello(fp) == 0) {
        ALOGE("Zero length CRC file");
        return 7;
    }
    fseeko(fp, 0, SEEK_SET);
    return 0;
}

int VideoCRCCheckerDestroy(VideoCRCChecker *chk)
{
    if (chk->fp != NULL) {
        fclose(chk->fp);
        chk->fp = NULL;
    }
    return 0;
}

void VideoCRCCheckerSetCRCCheckMode(VideoCRCChecker *chk, int mode)
{
    chk->crcCheckMode = (uint32_t)mode;
}

uint32_t VideoCRCCheckerGetPrevCRC(VideoCRCChecker *chk)
{
    return chk->prevCRC;
}

uint32_t VideoCRCCheckerGetPrevMISR(VideoCRCChecker *chk)
{
    uint32_t value;
    memcpy(&value, (const char *)chk + PENDING_OFF, sizeof(value));
    return value;
}

void VideoCRCSetSkipOpbMisr(VideoCRCChecker *chk, int skip)
{
    chk->skipOpbMisr = (uint32_t)skip;
}

int VideoCRCGetCRCFileInfo(VideoCRCChecker *chk, const char **out, int which)
{
    int mode = chk->mode;

    if (mode > 1)
        return 3;
    if (which != 0 && which != 1)
        return 3;
    *out = kCrcFileNames[mode * 2 + which];
    return 0;
}



int VideoCRCCheckerGetPendingCRCCount(VideoCRCChecker *chk)
{
    uint32_t tmp = 0;
    int count = 0;

    if (feof(chk->fp) != 0)
        return 0;
    while (fscanf(chk->fp, "%x", &tmp) >= 1) {
        if (chk->checkStarted == 0)
            count++;
        else if (tmp != chk->prevCRC)
            count++;
        if (feof(chk->fp) != 0)
            break;
    }
    return count;
}

uint32_t VideoCRCCheckerTotalCRCCount(VideoCRCChecker *chk)
{
    uint32_t tmp = 0;
    uint32_t count = 0;
    long pos;

    if (chk->fp == NULL)
        return 0;
    pos = ftello(chk->fp);
    fseeko(chk->fp, 0, SEEK_SET);
    if (feof(chk->fp) == 0) {
        while (fscanf(chk->fp, "%x", &tmp) >= 1) {
            count++;
            if (feof(chk->fp) != 0)
                break;
        }
    }
    fseeko(chk->fp, pos, SEEK_SET);
    chk->totalCRC = count;
    return count;
}

int VideoCRCCheckerSkipFrameCRCCheck(VideoCRCChecker *chk)
{
    uint32_t tmp = 0;

    if (chk->fp == NULL)
        return 0;
    fscanf(chk->fp, "%x", &tmp);
    ALOGE("Skipping one frame CRC");
    ALOGE("0");
    chk->pendingCRC++;
    return 0;
}



#define MISR_SCAN_FMT                                                          \
    " %d%*c %x%*c %x%*c %x%*c %x%*c %d%*c %d%*c %d%*c %d%*c %d%*c %d%*c "      \
    "%d%*c %d%*c"

static int misr_scan(FILE *fp, MisrRecord *r)
{
    return fscanf(fp, MISR_SCAN_FMT, &r->frameNo, &r->dpbLuma[0],
                  &r->dpbChroma[0], &r->opbLuma[0], &r->opbChroma[0],
                  &r->width, &r->height, &r->cropWidth, &r->cropHeight,
                  &r->cropX, &r->cropY, &r->bitDepthY, &r->bitDepthC);
}

static int misr_scan_indexed(FILE *fp, MisrRecord *r, int idx)
{
    return fscanf(fp, MISR_SCAN_FMT, &r->frameNo, &r->dpbLuma[idx],
                  &r->dpbChroma[idx], &r->opbLuma[idx], &r->opbChroma[idx],
                  &r->width, &r->height, &r->cropWidth, &r->cropHeight,
                  &r->cropX, &r->cropY, &r->bitDepthY, &r->bitDepthC);
}


static int misr_scan_indexed2(FILE *fp, MisrRecord *r, int idx)
{
    return fscanf(fp, MISR_SCAN_FMT, &r->frameNo, &r->dpbLuma2[idx],
                  &r->dpbChroma2[idx], &r->opbLuma2[idx], &r->opbChroma2[idx],
                  &r->width, &r->height, &r->cropWidth, &r->cropHeight,
                  &r->cropX, &r->cropY, &r->bitDepthY, &r->bitDepthC);
}

static void misr_write_line(FILE *fp, const MisrRecord *r, int idx)
{
    fprintf(fp,
            "%d, %08x, %08x, %08x, %08x, %d, %d, %d, %d, %d, %d, %d, %d\n",
            r->frameNo, r->dpbLuma[idx], r->dpbChroma[idx], r->opbLuma[idx],
            r->opbChroma[idx], r->width, r->height, r->cropWidth,
            r->cropHeight, r->cropX, r->cropY, r->bitDepthY, r->bitDepthC);
}


static void misr_write_line2(FILE *fp, const MisrRecord *r, int idx)
{
    fprintf(fp,
            "%d, %08x, %08x, %08x, %08x, %d, %d, %d, %d, %d, %d, %d, %d\n",
            r->frameNo, r->dpbLuma2[idx], r->dpbChroma2[idx], r->opbLuma2[idx],
            r->opbChroma2[idx], r->width, r->height, r->cropWidth,
            r->cropHeight, r->cropX, r->cropY, r->bitDepthY, r->bitDepthC);
}

#define MISR_MISMATCH_FMT "!! MISR Mismatch Field %s Ref %x FW %x  !!"


static int VideoCRCCheckerCompareMisrInfo(const MisrRecord *a,
                                          const MisrRecord *b, int which,
                                          int logFlag, int skipOpb)
{
    const uint32_t *dpbA = which ? a->dpbLuma2 : a->dpbLuma;
    const uint32_t *dpbB = which ? b->dpbLuma2 : b->dpbLuma;
    const uint32_t *chrA = which ? a->dpbChroma2 : a->dpbChroma;
    const uint32_t *chrB = which ? b->dpbChroma2 : b->dpbChroma;
    const uint32_t *opbA = which ? a->opbLuma2 : a->opbLuma;
    const uint32_t *opbB = which ? b->opbLuma2 : b->opbLuma;
    const uint32_t *opcA = which ? a->opbChroma2 : a->opbChroma;
    const uint32_t *opcB = which ? b->opbChroma2 : b->opbChroma;
    int count = which ? a->count2 : a->count;
    int i;

    if (count != 0) {
        for (i = 0; i < count; i++) {
            if (dpbA[i] != dpbB[i]) {
                if (logFlag == 0)
                    return 0x10;
                ALOGE(MISR_MISMATCH_FMT, "DPBLuma", dpbB[i], dpbA[i]);
                return 0x10;
            }
        }
        for (i = 0; i < count; i++) {
            if (chrA[i] != chrB[i]) {
                if (logFlag == 0)
                    return 0x10;
                ALOGE(MISR_MISMATCH_FMT, "DPBChroma", chrB[i], chrA[i]);
                return 0x10;
            }
        }
        if (skipOpb == 0) {
            for (i = 0; i < count; i++) {
                if (opbA[i] != opbB[i]) {
                    if (logFlag == 0)
                        return 0x10;
                    ALOGE(MISR_MISMATCH_FMT, "OPBLuma", opbB[i], opbA[i]);
                    return 0x10;
                }
            }
            for (i = 0; i < count; i++) {
                if (opcA[i] != opcB[i]) {
                    if (logFlag == 0)
                        return 0x10;
                    ALOGE(MISR_MISMATCH_FMT, "OPBChroma", opcB[i], opcA[i]);
                    return 0x10;
                }
            }
        }
    }

    if (a->cropWidth != b->cropWidth) {
        if (logFlag == 0)
            return 0x10;
        ALOGE(MISR_MISMATCH_FMT, "CropWidth", b->cropWidth, a->cropWidth);
        return 0x10;
    }
    if (a->cropHeight != b->cropHeight) {
        if (logFlag == 0)
            return 0x10;
        ALOGE(MISR_MISMATCH_FMT, "CropHeight", b->cropHeight, a->cropHeight);
        return 0x10;
    }
    if (a->cropX != b->cropX) {
        if (logFlag == 0)
            return 0x10;
        ALOGE(MISR_MISMATCH_FMT, "CropX", b->cropX, a->cropX);
        return 0x10;
    }
    if (a->cropY != b->cropY) {
        if (logFlag == 0)
            return 0x10;
        ALOGE(MISR_MISMATCH_FMT, "CropY", b->cropY, a->cropY);
        return 0x10;
    }
    if (a->bitDepthY != b->bitDepthY) {
        if (logFlag == 0)
            return 0x10;
        ALOGE(MISR_MISMATCH_FMT, "BitDepthY", b->bitDepthY, a->bitDepthY);
        return 0x10;
    }
    if (a->bitDepthC != b->bitDepthC) {
        if (logFlag == 0)
            return 0x10;
        ALOGE(MISR_MISMATCH_FMT, "BitDepthC", b->bitDepthC, a->bitDepthC);
        return 0x10;
    }
    return 0;
}


static int VideoCRCCheckerSearchMISREntry(VideoCRCChecker *chk,
                                          MisrRecord *out, const MisrRecord *rec,
                                          int flags, int kind)
{
    int flagBit = ((flags & 1) == 0) ? (1 & ~(flags >> 5)) : 0;
    int ret;
    int skip = (int)chk->skipOpbMisr;
    int i;

    memset(out, 0, REC_SIZE);

    if (kind == 0) {
        char scratch[0x100];
        fseeko(chk->fp, 0, SEEK_SET);
        memset(scratch, 0, sizeof(scratch));
        fgets(scratch, sizeof(scratch), chk->fp);
        kind = 1;
    }

    if (kind == 2) {
        if (rec->count == 0) {
            out->bitDepthY = depth_to_index(out->bitDepthY);
            out->bitDepthC = depth_to_index(out->bitDepthC);
        } else {
            for (i = 0; i < rec->count; i++) {
                if (misr_scan_indexed(chk->fp, out, i) < 1)
                    return 5;
            }
            out->bitDepthY = depth_to_index(out->bitDepthY);
            out->bitDepthC = depth_to_index(out->bitDepthC);
        }
    } else {
        if (out->dpbLuma[0] != rec->dpbLuma[0]) {
            while (feof(chk->fp) == 0) {
                int sr = misr_scan(chk->fp, out);
                if (sr < 1)
                    return 5;
                if (out->dpbLuma[0] == rec->dpbLuma[0])
                    break;
            }
        }
        for (i = 1; i < rec->count; i++) {
            if (misr_scan_indexed(chk->fp, out, i) < 1)
                return 5;
        }
        out->bitDepthY = depth_to_index(out->bitDepthY);
        out->bitDepthC = depth_to_index(out->bitDepthC);
    }


    ret = VideoCRCCheckerCompareMisrInfo(rec, out, 0, 1, skip);
    if (flagBit == 0)
        return ret;
    if (ret != 0)
        return ret;


    if (rec->dpbLuma2[0] == 0 && rec->dpbChroma2[0] == 0)
        return 0;
    if (rec->count2 != 0) {
        for (i = 0; i < rec->count2; i++) {
            if (misr_scan_indexed2(chk->fp, out, i) < 1)
                return 5;
        }
        out->bitDepthY = depth_to_index(out->bitDepthY);
        out->bitDepthC = depth_to_index(out->bitDepthC);
    }
    return VideoCRCCheckerCompareMisrInfo(rec, out, 1, 1, skip);
}
int VideoCRCCheckerCheckMISR(VideoCRCChecker *chk, unsigned char *unused,
                             MisrFrame *frame, int unused2, uint32_t *out)
{
    MisrRecord scratch;
    uint32_t dummy = 0;
    int ret = 0;
    int i;
    MisrRecord *rec = &frame->rec;

    (void)unused;
    (void)unused2;
    if (out == NULL)
        out = &dummy;

    memset(&scratch, 0, sizeof(scratch));

    if (chk->fp == NULL)
        goto match;

    if (chk->writeMode != 0) {
        rec->bitDepthY = index_to_depth(rec->bitDepthY);
        rec->bitDepthC = index_to_depth(rec->bitDepthC);
        if (rec->count != 0) {
            for (i = 0; i < rec->count; i++)
                misr_write_line(chk->fp, rec, i);
        }
        if ((frame->flags & 0x21) == 0 && rec->dpbLuma2[0] != 0 &&
            rec->opbLuma2[0] != 0 && rec->count2 != 0) {
            for (i = 0; i < rec->count2; i++)
                misr_write_line2(chk->fp, rec, i);
        }
        goto match;
    }

    ALOGE("Searching in file for %x", rec->dpbLuma[0]);
    if (chk->crcCheckMode != 0) {
        ret = VideoCRCCheckerSearchMISREntry(chk, &scratch, rec, frame->flags,
                                             0);
    } else {
        chk->pendingCRC++;
        ret = VideoCRCCheckerSearchMISREntry(chk, &scratch, rec, frame->flags,
                                             2);
    }
    if (ret == 0)
        goto match;

    ALOGE("MISR Mismatch Found!!");
    if (chk->crcCheckMode != 0)
        goto asterisk;

    if (VideoCRCCheckerSearchMISREntry(chk, &scratch, rec, frame->flags, 1) !=
        0)
        goto asterisk;
    if (VideoCRCCheckerSearchMISREntry(chk, (MisrRecord *)chk->u.misrRec, rec,
                                       frame->flags, 0) != 0)
        goto reorder;
    if (VideoCRCCheckerSearchMISREntry(chk, (MisrRecord *)chk->u.misrRec, rec,
                                       frame->flags, 1) != 0)
        goto reorder;

    ALOGE("\nDuplicate MISR count %d", 2);
    chk->duplicateMisr = 2;
    (void)out;
    return 0xe;

reorder:
    ALOGE("\nDisplay Reorder issue. Current Frm: %d", chk->pendingCRC);
    ret = 8;
asterisk:
    ALOGE("*");
    *out = 0;
    memcpy(chk->u.misrRec, rec, REC_SIZE);
    return ret;

match:
    ALOGE("MISR Match Found!!");
    ALOGE(".");
    *out = (ret == 0) ? 1u : 0u;
    memcpy(chk->u.misrRec, rec, REC_SIZE);
    return ret;
}

int VideoCRCCheckerGetPendingMISRCount(VideoCRCChecker *chk)
{
    MisrRecord rec;
    int count = 0;

    memset(&rec, 0, sizeof(rec));
    if (feof(chk->fp) != 0)
        return 0;
    for (;;) {
        memset(&rec, 0, sizeof(rec));
        if (misr_scan(chk->fp, &rec) < 1)
            break;
        rec.bitDepthY = depth_to_index(rec.bitDepthY);
        rec.bitDepthC = depth_to_index(rec.bitDepthC);
        if (chk->checkStarted == 0)
            count++;
        else if (VideoCRCCheckerCompareMisrInfo(
                     &rec, (const MisrRecord *)chk->u.misrRec, 0, 0,
                     (int)chk->skipOpbMisr) != 0)
            count++;
        if (feof(chk->fp) != 0)
            break;
    }
    return count;
}

uint32_t VideoCRCCheckerTotalMISRCount(VideoCRCChecker *chk)
{
    MisrRecord rec;
    char scratch[0x100];
    long pos;
    uint32_t count = 0;

    memset(&rec, 0, sizeof(rec));
    if (chk->fp == NULL)
        return 0;
    pos = ftell(chk->fp);
    fseeko(chk->fp, 0, SEEK_SET);
    memset(scratch, 0, sizeof(scratch));
    fgets(scratch, sizeof(scratch), chk->fp);
    if (feof(chk->fp) == 0) {
        for (;;) {
            memset(&rec, 0, sizeof(rec));
            if (misr_scan(chk->fp, &rec) < 1)
                break;
            count++;
            if (feof(chk->fp) != 0)
                break;
        }
    }
    fseeko(chk->fp, pos, SEEK_SET);
    chk->totalCRC = count;
    return count;
}

int VideoCRCCheckerSkipFrameMISRCheck(VideoCRCChecker *chk)
{
    if (chk->fp == NULL)
        return 0;
    ALOGE("Skipping one frame MISR Check");
    ALOGE("0");
    chk->pendingCRC++;
    return (int)chk->pendingCRC;
}



int VideoCRCCheckerCheckCRCofYUVframe(VideoCRCChecker *chk,
                                      const unsigned char *buf, CrcFrame *info,
                                      int flag, uint32_t *out)
{
    const uint32_t *table = chk->u.crc.table;
    uint32_t crc = 0;
    uint32_t computed;
    uint32_t ref = 0x00ffffffu;
    uint32_t val = 0xffffffffu;
    uint32_t hits = 0;
    int ret = 0;
    int i, r, c;
    const int width = info->width;
    const int height = info->height;
    const int stride = info->stride;
    const int cropX = info->cropX;
    const int cropY = info->cropY;

    *out = 0;

    ALOGE("CRCresCROP: %u %u %u %u %u %u", (unsigned)cropX, (unsigned)cropY,
          (unsigned)width, (unsigned)height, (unsigned)info->reserved0,
          (unsigned)info->reserved1);

    if (flag != 0) {
        if (cropX != 0 || cropY != 0) {
            ALOGE("CRC calculation: StartX StartY not supported.");
            return 0x000fffff;
        }

        if (height != 0 && width != 0) {
            const unsigned char *p = buf;
            for (r = 0; r < height; r += 7, p += (size_t)7 * stride) {
                const unsigned char *q = p;
                for (c = 0; c < width; c += 7, q += 7)
                    crc = crc_step(crc, table, *q);
            }
        }

        if ((height >> 1) != 0 && (width >> 1) != 0) {
            const unsigned char *base =
                buf + (size_t)info->reserved10 * stride;
            const int rows = (height + 1) >> 1;
            const int cols = (width + 1) >> 1;
            const int rowStep = 2 * ((stride + 1) >> 1);
            crc = ~crc;
            for (r = 0; r < rows; r += 7) {
                const unsigned char *q = base + (size_t)r * rowStep;
                for (c = 0; c < cols; c += 7)
                    crc = crc_step(crc, table, q[2 * c]);
            }
            crc = ~crc;
            for (r = 0; r < rows; r += 7) {
                const unsigned char *q = base + 1 + (size_t)r * rowStep;
                for (c = 0; c < cols; c += 7)
                    crc = crc_step(crc, table, q[2 * c]);
            }
        }
    } else {
        if ((width & 1) != 0 || (height & 1) != 0) {
            ALOGE("CRC calculation for odd resolution for panner format not "
                  "supported.");
            return 0x000fffff;
        }
        if (height != 0 && width != 0) {
            const unsigned char *p = buf + (size_t)cropY * stride + cropX;
            for (r = 0; r < height; r += 7, p += (size_t)7 * stride) {
                const unsigned char *q = p;
                for (c = 0; c < width; c += 7, q += 7)
                    crc = crc_step(crc, table, *q);
            }
        }
        if ((height >> 1) != 0 && (width >> 1) != 0) {
            const int cw = (stride >> 1) * (cropY >> 1) + (cropX >> 1);
            const int rowStep = stride >> 1;
            const unsigned char *base = buf + cw;
            crc = ~crc;
            for (r = 0; r < (height >> 1); r += 7) {
                const unsigned char *q = base + (size_t)r * rowStep;
                for (c = 0; c < (width >> 1); c += 7)
                    crc = crc_step(crc, table, q[c]);
            }
            crc = ~crc;
            for (r = 0; r < (height >> 1); r += 7) {
                const unsigned char *q = base + (size_t)r * rowStep;
                for (c = 0; c < (width >> 1); c += 7)
                    crc = crc_step(crc, table, q[c]);
            }
        }
    }

    computed = ~crc;

    if (chk->fp == NULL)
        return 0;

    if (chk->writeMode != 0) {
        fprintf(chk->fp, "%08X\n", computed);
        chk->pendingCRC++;
        *out = 1;
        return 0;
    }

    if (chk->crcCheckMode != 0) {
        ALOGE("Searching in file for %x", computed);
        fseeko(chk->fp, 0, SEEK_SET);
        if (computed != 0x00ffffffu) {
            while (feof(chk->fp) == 0) {
                if (fscanf(chk->fp, "%x", &ref) < 1)
                    break;
                if (ref == computed)
                    break;
            }
        }

        ALOGE("Searching in file entry found %d, crc: 0x%x file_offset 0x%x "
              "buffer %p",
              (ref == computed) ? 1 : 0, computed, (unsigned)width,
              (const void *)(uintptr_t)ftello(chk->fp));
    } else {
        chk->pendingCRC++;
        ret = (fscanf(chk->fp, "%x", &ref) > 0) ? 0 : 5;
    }

    if (chk->checkStarted == 1 && ref != computed && ref == chk->prevCRC) {
        chk->pendingCRC++;
        while (feof(chk->fp) == 0) {
            if (fscanf(chk->fp, "%x", &ref) < 1)
                break;
            if (ref != chk->prevCRC)
                break;
        }
    }

    if (ref == computed) {
        ALOGE("CRC Match Found!! Calculated CRC:0x%x buffer %p", computed,
              (const void *)buf);
        ALOGE(".");
        *out = 1;
        chk->prevCRC = computed;
        return ret;
    }

    ALOGE("CRC Mismatch Found!! Calculated CRC:0x%x RefCRC:0x%x @ cnt=%d "
          "buffer %p",
          computed, ref, chk->pendingCRC, (const void *)buf);

    if (chk->crcCheckMode != 0 || ret != 0) {
        ALOGE("*");
        *out = 0;
        chk->prevCRC = computed;
        return ret;
    }

    hits = chk->pendingCRC;
    if (val != computed) {
        while (feof(chk->fp) == 0) {
            if (fscanf(chk->fp, "%x", &val) < 1)
                break;
            hits++;
            if (val == computed)
                break;
        }
    }
    if (val != computed) {
        ALOGE("*");
        *out = 0;
        chk->prevCRC = computed;
        return 0;
    }

    {
        uint32_t dup = 0;
        int checked = 0;
        fseeko(chk->fp, 0, SEEK_SET);
        if (feof(chk->fp) == 0) {
            while (fscanf(chk->fp, "%x", &val) >= 1) {
                if (val == chk->prevCRC)
                    dup++;
                if (feof(chk->fp) != 0)
                    break;
            }
            checked = 1;
        }
        if (checked != 0 && dup > 1) {
            ALOGE("\nDuplicate CRC 0x%x, count %d", chk->prevCRC, dup);
            chk->prevCRC = dup;
            return 0xe;
        }
        ALOGE("\nDisple Reorder issue. Current Frm: %d, CRC file Frm:%d "
              "Calculated CRC:0x%x buffer %p",
              chk->pendingCRC, (int)hits, computed, (const void *)buf);
        ret = 8;
        ALOGE("*");
        *out = 0;
        chk->prevCRC = computed;
        return ret;
    }
    (void)i;
}




typedef void *(*fastcrc_copy_fn)(void *, const void *, size_t);
typedef void *(*fastcrc_set_fn)(void *, int, size_t);
static fastcrc_copy_fn volatile fastcrc_libc_memcpy = memcpy;
static fastcrc_set_fn volatile fastcrc_libc_memset = memset;

void __aeabi_memcpy(void *dst, const void *src, size_t n)
{
    fastcrc_libc_memcpy(dst, src, n);
}
__attribute__((alias("__aeabi_memcpy")))
void __aeabi_memcpy4(void *dst, const void *src, size_t n);
__attribute__((alias("__aeabi_memcpy")))
void __aeabi_memcpy8(void *dst, const void *src, size_t n);

void __aeabi_memset(void *dst, size_t n, int c)
{
    fastcrc_libc_memset(dst, c, n);
}
__attribute__((alias("__aeabi_memset")))
void __aeabi_memset4(void *dst, size_t n, int c);
__attribute__((alias("__aeabi_memset")))
void __aeabi_memset8(void *dst, size_t n, int c);

void __aeabi_memclr(void *dst, size_t n)
{
    fastcrc_libc_memset(dst, 0, n);
}
__attribute__((alias("__aeabi_memclr")))
void __aeabi_memclr4(void *dst, size_t n);
__attribute__((alias("__aeabi_memclr")))
void __aeabi_memclr8(void *dst, size_t n);


int CRCVideoInterlaceType;
