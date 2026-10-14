/*
 * gif.cpp -- read a GIF file into a Reality Lab texture.
 *
 * Labyrinth used David Koblas's GIF reader (from giftoppm, "Copyright 1990,
 * 1991, 1993, David Koblas.  Permission to use, copy, modify, and distribute
 * this software and its documentation for any purpose and without fee is
 * hereby granted..."), changed to deliver an 8-bit palettised RLImage.
 * Reconstructed from LABYRINTH.EXE 0x411920-0x4128e0.
 *
 * Change from the original: errors abandon the picture (the texture comes
 * back NULL) instead of calling exit(1) and taking Labyrinth down.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include "drawwin.h"

#define MAXCOLORMAPSIZE 256
#define MAX_LWZ_BITS 12
#define INTERLACE 0x40
#define LOCALCOLORMAP 0x80
#define BitSet(byte, bit) (((byte) & (bit)) == (bit))
#define ReadOK(file, buffer, len) (fread(buffer, len, 1, file) != 0)
#define LM_to_uint(a, b) (((b) << 8) | (a))

static struct {
    unsigned int Width;
    unsigned int Height;
    unsigned char ColorMap[3][MAXCOLORMAPSIZE];
    unsigned int BitPixel;
    unsigned int ColorResolution;
    unsigned int Background;
    unsigned int AspectRatio;
} GifScreen;                                                    /* 4abcc0 */

static struct {
    int transparent;
    int delayTime;
    int inputFlag;
    int disposal;
} Gif89 = { -1, -1, -1, 0 };                                    /* 4b7988 */

static int verbose;                                             /* 4abfdc */
static int showComment;                                         /* 4abfd8 */
static int ZeroDataBlock;                                       /* 4b7998 */
static jmp_buf gifError;

static void pm_error(const char *fmt, ...)                      /* 411e50 */
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    longjmp(gifError, 1);
}

static void pm_message(const char *fmt, ...)                    /* 411e90 */
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

static int ReadColorMap(FILE *fd, int number, unsigned char buffer[3][MAXCOLORMAPSIZE]) /* 411ec0 */
{
    unsigned char rgb[3];
    for (int i = 0; i < number; ++i) {
        if (!ReadOK(fd, rgb, sizeof(rgb)))
            pm_error("bad colormap");
        buffer[0][i] = rgb[0];
        buffer[1][i] = rgb[1];
        buffer[2][i] = rgb[2];
    }
    return 0;
}

static int GetDataBlock(FILE *fd, unsigned char *buf)           /* 412180 */
{
    unsigned char count;
    if (!ReadOK(fd, &count, 1)) {
        pm_message("error in getting DataBlock size");
        return -1;
    }
    ZeroDataBlock = count == 0;
    if (count != 0 && !ReadOK(fd, buf, count)) {
        pm_message("error in reading DataBlock");
        return -1;
    }
    return count;
}

static int DoExtension(FILE *fd, int label)                     /* 411f30 */
{
    static unsigned char buf[256];
    const char *str;

    switch (label) {
    case 0x01:
        str = "Plain Text Extension";
        break;
    case 0xff:
        str = "Application Extension";
        break;
    case 0xfe:
        while (GetDataBlock(fd, buf) > 0)
            if (showComment)
                pm_message("gif comment: %s", buf);
        return 0;
    case 0xf9:
        GetDataBlock(fd, buf);
        Gif89.disposal = (buf[0] >> 2) & 0x7;
        Gif89.inputFlag = (buf[0] >> 1) & 0x1;
        Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
        if ((buf[0] & 0x1) != 0)
            Gif89.transparent = buf[3];
        while (GetDataBlock(fd, buf) > 0)
            ;
        return 0;
    default:
        str = (char *)buf;
        sprintf((char *)buf, "UNKNOWN (0x%02x)", label);
        break;
    }
    pm_message("got a '%s' extension - please report this to koblas@mips.com", str);
    while (GetDataBlock(fd, buf) > 0)
        ;
    return 0;
}

static int GetCode(FILE *fd, int code_size, int flag)           /* 412210 */
{
    static unsigned char buf[280];
    static int curbit, lastbit, done, last_byte;
    int i, j, ret;
    unsigned char count;

    if (flag) {
        curbit = 0;
        lastbit = 0;
        done = 0;
        return 0;
    }
    if ((curbit + code_size) >= lastbit) {
        if (done) {
            if (curbit >= lastbit)
                pm_error("ran off the end of my bits");
            return -1;
        }
        buf[0] = buf[last_byte - 2];
        buf[1] = buf[last_byte - 1];
        if ((count = (unsigned char)GetDataBlock(fd, &buf[2])) == 0)
            done = 1;
        last_byte = 2 + count;
        curbit = (curbit - lastbit) + 16;
        lastbit = (2 + count) * 8;
    }
    ret = 0;
    for (i = curbit, j = 0; j < code_size; ++i, ++j)
        ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;
    curbit += code_size;
    return ret;
}

static int LZWReadByte(FILE *fd, int flag, int input_code_size) /* 412350 */
{
    static int fresh = 0;
    int code, incode;
    static int code_size, set_code_size;
    static int max_code, max_code_size;
    static int firstcode, oldcode;
    static int clear_code, end_code;
    static int table[2][(1 << MAX_LWZ_BITS)];
    static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;
    int i;

    if (flag) {
        set_code_size = input_code_size;
        code_size = set_code_size + 1;
        clear_code = 1 << set_code_size;
        end_code = clear_code + 1;
        max_code_size = 2 * clear_code;
        max_code = clear_code + 2;
        GetCode(fd, 0, 1);
        fresh = 1;
        for (i = 0; i < clear_code; ++i) {
            table[0][i] = 0;
            table[1][i] = i;
        }
        for (; i < (1 << MAX_LWZ_BITS); ++i)
            table[0][i] = table[1][0] = 0;
        sp = stack;
        return 0;
    } else if (fresh) {
        fresh = 0;
        do {
            firstcode = oldcode = GetCode(fd, code_size, 0);
        } while (firstcode == clear_code);
        return firstcode;
    }

    if (sp > stack)
        return *--sp;

    while ((code = GetCode(fd, code_size, 0)) >= 0) {
        if (code == clear_code) {
            for (i = 0; i < clear_code; ++i) {
                table[0][i] = 0;
                table[1][i] = i;
            }
            for (; i < (1 << MAX_LWZ_BITS); ++i)
                table[0][i] = table[1][i] = 0;
            code_size = set_code_size + 1;
            max_code_size = 2 * clear_code;
            max_code = clear_code + 2;
            sp = stack;
            firstcode = oldcode = GetCode(fd, code_size, 0);
            return firstcode;
        } else if (code == end_code) {
            int count;
            unsigned char buf[260];
            if (ZeroDataBlock)
                return -2;
            while ((count = GetDataBlock(fd, buf)) > 0)
                ;
            if (count != 0)
                pm_message("missing EOD in data stream (common occurence)");
            return -2;
        }

        incode = code;
        if (code >= max_code) {
            *sp++ = firstcode;
            code = oldcode;
        }
        while (code >= clear_code) {
            *sp++ = table[1][code];
            if (code == table[0][code])
                pm_error("circular table entry BIG ERROR");
            code = table[0][code];
            if (sp >= stack + (sizeof(stack) / sizeof(stack[0])))
                pm_error("circular table entry BIG ERROR");
        }
        *sp++ = firstcode = table[1][code];

        if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
            table[0][code] = oldcode;
            table[1][code] = firstcode;
            ++max_code;
            if ((max_code >= max_code_size) && (max_code_size < (1 << MAX_LWZ_BITS))) {
                max_code_size *= 2;
                ++code_size;
            }
        }
        oldcode = incode;
        if (sp > stack)
            return *--sp;
    }
    return code;
}

/* Decode one image into the RLImage's pixel buffer. */
static void ReadImage(FILE *fd, RLImage *image, int len, int height,
                      int interlace, int ignore)                /* 4126d0 */
{
    unsigned char c;
    int v;
    int xpos = 0, ypos = 0, pass = 0;
    unsigned char *pixels = (unsigned char *)image->buffer1;

    if (!ReadOK(fd, &c, 1))
        pm_error("EOF / read error on image data");
    if (LZWReadByte(fd, 1, c) < 0)
        pm_error("error reading image");

    if (ignore) {
        if (verbose)
            pm_message("skipping image...");
        while (LZWReadByte(fd, 0, c) >= 0)
            ;
        return;
    }
    if (verbose)
        pm_message("reading %d by %d%s GIF image", len, height, interlace ? " interlaced" : "");

    while ((v = LZWReadByte(fd, 0, c)) >= 0) {
        if (xpos < image->width && ypos < image->height)
            pixels[ypos * image->bytes_per_line + xpos] = (unsigned char)v;
        ++xpos;
        if (xpos == len) {
            xpos = 0;
            if (interlace) {
                switch (pass) {
                case 0:
                case 1:
                    ypos += 8;
                    break;
                case 2:
                    ypos += 4;
                    break;
                case 3:
                    ypos += 2;
                    break;
                }
                if (ypos >= height) {
                    ++pass;
                    switch (pass) {
                    case 1: ypos = 4; break;
                    case 2: ypos = 2; break;
                    case 3: ypos = 1; break;
                    default: goto fini;
                    }
                }
            } else {
                ++ypos;
            }
        }
        if (ypos >= height)
            break;
    }
fini:
    if (LZWReadByte(fd, 0, c) >= 0)
        pm_message("too much input data, ignoring extra...");
    if (verbose)
        pm_message("writing output");
}

static void setPalette(RLImage *image, unsigned char map[3][MAXCOLORMAPSIZE], int n)
{
    for (int i = 0; i < n; i++) {
        image->palette[i].red = map[0][i];
        image->palette[i].green = map[1][i];
        image->palette[i].blue = map[2][i];
        image->palette[i].flags = 1;
    }
    image->palette_size = n;
}

static RLTexture ReadGIF(FILE *fd, int imageNumber)             /* 411960 */
{
    unsigned char buf[16];
    unsigned char c;
    unsigned char localColorMap[3][MAXCOLORMAPSIZE];
    char version[4];
    int imageCount = 0;
    RLImage *image = 0;

    if (setjmp(gifError)) {
        if (image)
            RLFree(image);
        return NULL;
    }

    if (!ReadOK(fd, buf, 6))
        pm_error("error reading magic number");
    if (strncmp((char *)buf, "GIF", 3) != 0)
        pm_error("not a GIF file");
    strncpy(version, (char *)buf + 3, 3);
    version[3] = '\0';
    if (strcmp(version, "87a") != 0 && strcmp(version, "89a") != 0)
        pm_error("bad version number, not '87a' or '89a'");

    if (!ReadOK(fd, buf, 7))
        pm_error("failed to read screen descriptor");
    GifScreen.Width = LM_to_uint(buf[0], buf[1]);
    GifScreen.Height = LM_to_uint(buf[2], buf[3]);
    GifScreen.BitPixel = 2 << (buf[4] & 0x07);
    GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
    GifScreen.Background = buf[5];
    GifScreen.AspectRatio = buf[6];

    /* The image header, the palette and the pixels in one block */
    int bpl = (GifScreen.Width + 3) & ~3;
    if (RLMalloc((void **)&image, sizeof(RLImage) + 256 * sizeof(RLPaletteEntry)
                 + bpl * GifScreen.Height) != RLNoError)
        return NULL;
    memset(image, 0, sizeof(RLImage));
    image->width = GifScreen.Width;
    image->height = GifScreen.Height;
    image->aspectx = image->aspecty = 1;
    image->depth = 8;
    image->rgb = 0;
    image->bytes_per_line = bpl;
    image->buffer1 = (char *)image + sizeof(RLImage) + 256 * sizeof(RLPaletteEntry);
    image->palette_size = 256;
    image->palette = (RLPaletteEntry *)((char *)image + sizeof(RLImage));
    memset(image->palette, 0, 256 * sizeof(RLPaletteEntry));
    memset(image->buffer1, 0, bpl * GifScreen.Height);

    if (BitSet(buf[4], LOCALCOLORMAP)) {        /* global colour map */
        if (ReadColorMap(fd, GifScreen.BitPixel, GifScreen.ColorMap))
            pm_error("error reading global colormap");
        setPalette(image, GifScreen.ColorMap, GifScreen.BitPixel);
    }

    for (;;) {
        if (!ReadOK(fd, &c, 1))
            pm_error("EOF / read error on image data");
        if (c == ';')                           /* GIF terminator */
            break;
        if (c == '!') {                         /* extension */
            if (!ReadOK(fd, &c, 1))
                pm_error("OF / read error on extention function code");
            DoExtension(fd, c);
            continue;
        }
        if (c != ',') {                         /* not a valid start character */
            pm_message("bogus character 0x%02x, ignoring", (int)c);
            continue;
        }
        ++imageCount;
        if (!ReadOK(fd, buf, 9))
            pm_error("couldn't read left/top/width/height");
        if (BitSet(buf[8], LOCALCOLORMAP)) {
            int n = 1 << ((buf[8] & 0x07) + 1);
            if (ReadColorMap(fd, n, localColorMap))
                pm_error("error reading local colormap");
            /* (the original copied GifScreen.BitPixel entries here) */
            setPalette(image, localColorMap, n);
        }
        ReadImage(fd, image, LM_to_uint(buf[4], buf[5]), LM_to_uint(buf[6], buf[7]),
                  BitSet(buf[8], INTERLACE), imageCount != imageNumber);
    }
    if (imageCount < imageNumber)
        pm_error("only %d image%s found in file", imageCount, imageCount > 1 ? "s" : "");

    RLTexture texture = RLCreateTexture(image);
    RLTextureSetColours(texture, image->palette_size);
    RLObjectSetAppData(texture, image);
    return texture;
}

RLTexture LabLoadGIF(const char *file)                          /* 411920 */
{
    FILE *fp = fopen(file, "rb");
    if (!fp)
        return NULL;
    RLTexture t = ReadGIF(fp, 1);
    fclose(fp);
    return t;
}
