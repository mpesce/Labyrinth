/*
 * inflate.h -- gzip/deflate decoding (see inflate.cpp).
 */
#ifndef INFLATE_H
#define INFLATE_H

#include <vector>
#include <stddef.h>

bool LabInflate(const unsigned char *in, size_t len, std::vector<unsigned char> &out);
bool LabGunzip(const unsigned char *in, size_t len, std::vector<unsigned char> &out);
const char *LabGunzipFile(const char *file);

#endif /* INFLATE_H */
