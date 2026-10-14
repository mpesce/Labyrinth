/*
 * labimage.cpp -- loading texture pictures of any common format.
 *
 * Labyrinth 2.0a2 read textures as GIF (gif.cpp) or PPM (RLLoadTexture).
 * VRML worlds also use JPEG and PNG, which are decoded here with the
 * Windows Imaging Component.  New code.
 */
#include <windows.h>
#include <wincodec.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "labimage.h"
#include "drawwin.h"

static RLTexture loadWIC(const char *file)
{
    static int comReady = 0;
    if (!comReady) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        comReady = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE ? 1 : -1;
    }
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *conv = NULL;
    RLTexture texture = NULL;
    wchar_t wfile[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, file, -1, wfile, MAX_PATH);

    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                                IID_IWICImagingFactory, (void **)&factory)))
        return NULL;
    UINT w = 0, h = 0;
    if (SUCCEEDED(factory->CreateDecoderFromFilename(wfile, NULL, GENERIC_READ,
                                                     WICDecodeMetadataCacheOnDemand, &decoder)) &&
        SUCCEEDED(decoder->GetFrame(0, &frame)) &&
        SUCCEEDED(factory->CreateFormatConverter(&conv)) &&
        SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                                   NULL, 0.0, WICBitmapPaletteTypeCustom)) &&
        SUCCEEDED(conv->GetSize(&w, &h)) && w > 0 && h > 0) {
        std::vector<unsigned char> pixels((size_t)w * h * 4);
        if (SUCCEEDED(conv->CopyPixels(NULL, w * 4, (UINT)pixels.size(), &pixels[0]))) {
            RLImage image;
            memset(&image, 0, sizeof(image));
            image.width = (int)w;
            image.height = (int)h;
            image.aspectx = image.aspecty = 1;
            image.depth = 32;
            image.rgb = 1;
            image.bytes_per_line = (int)w * 4;
            image.buffer1 = &pixels[0];
            image.red_mask = 0x00ff0000;
            image.green_mask = 0x0000ff00;
            image.blue_mask = 0x000000ff;
            image.alpha_mask = 0xff000000;
            texture = RLCreateTexture(&image);
        }
    }
    if (conv) conv->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    factory->Release();
    return texture;
}

RLTexture LabLoadImage(const char *file)
{
    unsigned char magic[4] = { 0, 0, 0, 0 };
    FILE *fp = fopen(file, "rb");
    if (!fp)
        return NULL;
    size_t n = fread(magic, 1, 4, fp);
    fclose(fp);
    if (n >= 3 && memcmp(magic, "GIF", 3) == 0)
        return LabLoadGIF(file);
    if (n >= 2 && magic[0] == 'P' && (magic[1] == '6' || magic[1] == '3'))
        return RLLoadTexture(file);
    return loadWIC(file);
}
