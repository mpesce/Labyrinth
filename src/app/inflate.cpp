/*
 * inflate.cpp -- a small gzip/deflate decoder (RFC 1951/1952).
 *
 * Many VRML worlds of 1995-96 were served gzip-compressed (.wrl.gz).  This is
 * a compact, self-contained decoder in the style of Mark Adler's puff.c:
 * slow but simple, and good enough for world files.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "inflate.h"

namespace {

struct Bits {
    const unsigned char *in;
    size_t len, pos;
    unsigned bitbuf;
    int bitcnt;
    std::vector<unsigned char> *out;
    bool error;
};

int getbits(Bits &s, int need)
{
    long val = s.bitbuf;
    while (s.bitcnt < need) {
        if (s.pos >= s.len) {
            s.error = true;
            return 0;
        }
        val |= (long)s.in[s.pos++] << s.bitcnt;
        s.bitcnt += 8;
    }
    s.bitbuf = (unsigned)(val >> need);
    s.bitcnt -= need;
    return (int)(val & ((1L << need) - 1));
}

const int MAXBITS = 15;

struct Huffman {
    short count[MAXBITS + 1];
    short symbol[288];
};

int decode(Bits &s, const Huffman &h)
{
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= MAXBITS; len++) {
        code |= getbits(s, 1);
        if (s.error)
            return -1;
        int count = h.count[len];
        if (code - count < first)
            return h.symbol[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    s.error = true;
    return -1;
}

int construct(Huffman &h, const short *length, int n)
{
    short offs[MAXBITS + 1];
    memset(h.count, 0, sizeof(h.count));
    for (int i = 0; i < n; i++)
        h.count[length[i]]++;
    if (h.count[0] == n)
        return 0;
    int left = 1;
    for (int len = 1; len <= MAXBITS; len++) {
        left <<= 1;
        left -= h.count[len];
        if (left < 0)
            return left;
    }
    offs[1] = 0;
    for (int len = 1; len < MAXBITS; len++)
        offs[len + 1] = offs[len] + h.count[len];
    for (int i = 0; i < n; i++)
        if (length[i] != 0)
            h.symbol[offs[length[i]]++] = (short)i;
    return left;
}

const short lbase[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                          35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
const short lext[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                         3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
const short dbase[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                          257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
                          8193, 12289, 16385, 24577 };
const short dext[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                         7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

bool codes(Bits &s, const Huffman &lencode, const Huffman &distcode)
{
    for (;;) {
        int symbol = decode(s, lencode);
        if (symbol < 0)
            return false;
        if (symbol < 256) {
            s.out->push_back((unsigned char)symbol);
        } else if (symbol == 256) {
            return true;
        } else {
            symbol -= 257;
            if (symbol >= 29)
                return false;
            int len = lbase[symbol] + getbits(s, lext[symbol]);
            symbol = decode(s, distcode);
            if (symbol < 0 || symbol >= 30)
                return false;
            size_t dist = dbase[symbol] + getbits(s, dext[symbol]);
            if (s.error || dist > s.out->size())
                return false;
            size_t from = s.out->size() - dist;
            for (int i = 0; i < len; i++)
                s.out->push_back((*s.out)[from + i]);
        }
    }
}

bool stored(Bits &s)
{
    s.bitbuf = 0;
    s.bitcnt = 0;
    if (s.pos + 4 > s.len)
        return false;
    unsigned len = s.in[s.pos] | (s.in[s.pos + 1] << 8);
    unsigned nlen = s.in[s.pos + 2] | (s.in[s.pos + 3] << 8);
    s.pos += 4;
    if (len != (~nlen & 0xffff) || s.pos + len > s.len)
        return false;
    s.out->insert(s.out->end(), s.in + s.pos, s.in + s.pos + len);
    s.pos += len;
    return true;
}

bool fixed(Bits &s)
{
    static Huffman lencode, distcode;
    static bool built = false;
    if (!built) {
        short lengths[288];
        int i;
        for (i = 0; i < 144; i++) lengths[i] = 8;
        for (; i < 256; i++) lengths[i] = 9;
        for (; i < 280; i++) lengths[i] = 7;
        for (; i < 288; i++) lengths[i] = 8;
        construct(lencode, lengths, 288);
        for (i = 0; i < 30; i++) lengths[i] = 5;
        construct(distcode, lengths, 30);
        built = true;
    }
    return codes(s, lencode, distcode);
}

bool dynamic(Bits &s)
{
    static const short order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
    short lengths[320];
    Huffman lencode, distcode;
    int nlen = getbits(s, 5) + 257;
    int ndist = getbits(s, 5) + 1;
    int ncode = getbits(s, 4) + 4;
    if (s.error || nlen > 286 || ndist > 30)
        return false;
    int index;
    for (index = 0; index < ncode; index++)
        lengths[order[index]] = (short)getbits(s, 3);
    for (; index < 19; index++)
        lengths[order[index]] = 0;
    if (construct(lencode, lengths, 19) != 0)
        return false;
    index = 0;
    while (index < nlen + ndist) {
        int symbol = decode(s, lencode);
        if (symbol < 0)
            return false;
        if (symbol < 16) {
            lengths[index++] = (short)symbol;
        } else {
            int len = 0;
            if (symbol == 16) {
                if (index == 0)
                    return false;
                len = lengths[index - 1];
                symbol = 3 + getbits(s, 2);
            } else if (symbol == 17) {
                symbol = 3 + getbits(s, 3);
            } else {
                symbol = 11 + getbits(s, 7);
            }
            if (index + symbol > nlen + ndist)
                return false;
            while (symbol--)
                lengths[index++] = (short)len;
        }
    }
    if (lengths[256] == 0)
        return false;
    int err = construct(lencode, lengths, nlen);
    if (err < 0 || (err > 0 && nlen - lencode.count[0] != 1))
        return false;
    err = construct(distcode, lengths + nlen, ndist);
    if (err < 0 || (err > 0 && ndist - distcode.count[0] != 1))
        return false;
    return codes(s, lencode, distcode);
}

} // namespace

bool LabInflate(const unsigned char *in, size_t len, std::vector<unsigned char> &out)
{
    Bits s;
    s.in = in;
    s.len = len;
    s.pos = 0;
    s.bitbuf = 0;
    s.bitcnt = 0;
    s.out = &out;
    s.error = false;
    int last;
    do {
        last = getbits(s, 1);
        int type = getbits(s, 2);
        if (s.error)
            return false;
        bool ok;
        switch (type) {
        case 0: ok = stored(s); break;
        case 1: ok = fixed(s); break;
        case 2: ok = dynamic(s); break;
        default: ok = false; break;
        }
        if (!ok || s.error)
            return false;
    } while (!last);
    return true;
}

bool LabGunzip(const unsigned char *in, size_t len, std::vector<unsigned char> &out)
{
    if (len < 18 || in[0] != 0x1f || in[1] != 0x8b || in[2] != 8)
        return false;
    int flags = in[3];
    size_t pos = 10;
    if (flags & 4) {                        /* FEXTRA */
        if (pos + 2 > len) return false;
        pos += 2 + (in[pos] | (in[pos + 1] << 8));
    }
    if (flags & 8)                          /* FNAME */
        while (pos < len && in[pos++]) ;
    if (flags & 16)                         /* FCOMMENT */
        while (pos < len && in[pos++]) ;
    if (flags & 2)                          /* FHCRC */
        pos += 2;
    if (pos >= len)
        return false;
    return LabInflate(in + pos, len - pos, out);
}

/* If 'file' is gzip-compressed, write the expanded data next to it and
 * return that file's name; otherwise return 'file' unchanged. */
const char *LabGunzipFile(const char *file)
{
    static char expanded[1024];
    FILE *fp = fopen(file, "rb");
    if (!fp)
        return file;
    unsigned char magic[2] = { 0, 0 };
    size_t n = fread(magic, 1, 2, fp);
    if (n != 2 || magic[0] != 0x1f || magic[1] != 0x8b) {
        fclose(fp);
        return file;
    }
    std::vector<unsigned char> in, out;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size <= 0) {
        fclose(fp);
        return file;
    }
    in.resize((size_t)size);
    n = fread(&in[0], 1, in.size(), fp);
    fclose(fp);
    if (n != in.size() || !LabGunzip(&in[0], in.size(), out))
        return file;
    /* the expanded copy goes in %TEMP%\Labyrinth, beside the other local
     * copies, rather than next to the original */
    char dir[MAX_PATH];
    GetTempPathA(MAX_PATH, dir);
    strncat(dir, "Labyrinth\\", sizeof(dir) - strlen(dir) - 1);
    CreateDirectoryA(dir, NULL);
    const char *base = file;
    for (const char *p = file; *p; p++)
        if (*p == '\\' || *p == '/' || *p == ':')
            base = p + 1;
    _snprintf(expanded, sizeof(expanded) - 1, "%s%s.unz", dir, base);
    fp = fopen(expanded, "wb");
    if (!fp)
        return file;
    if (!out.empty())
        fwrite(&out[0], 1, out.size(), fp);
    fclose(fp);
    return expanded;
}
