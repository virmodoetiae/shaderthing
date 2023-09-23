#include "vpch.h"
#include "vgraphics/vmisc/vgifencoder.h"

namespace vir
{

// Protected functions -------------------------------------------------------//

void GifEncoder::encodeIndexedFrame(int delay, bool flipVertically)
{
    // Graphics control extension
    fputc(0x21, file_);
    fputc(0xf9, file_);
    fputc(0x04, file_);
    fputc(0x05, file_); // Leave old frame in place, this frame has transparency
    fputc(delay & 0xff, file_);
    fputc((delay >> 8) & 0xff, file_);
    fputc(0, file_); // Transparent color index
    fputc(0, file_);

    // Image descriptor block
    fputc(0x2c, file_); 

    // Write image corner w.r.t. canvas, i.e., 0,0
    fputc(0 & 0xff, file_);
    fputc((0 >> 8) & 0xff, file_);
    fputc(0 & 0xff, file_);
    fputc((0 >> 8) & 0xff, file_);

    // Write image width, height
    fputc(width_ & 0xff, file_);
    fputc((width_ >> 8) & 0xff, file_);
    fputc(height_ & 0xff, file_);
    fputc((height_ >> 8) & 0xff, file_);

    // Local color table, 2^paletteBitDepth_ entries
    fputc(0x80 + paletteBitDepth_-1, file_);
    
    // Write last palette color used for transparency
    fputc(0, file_);
    fputc(0, file_);
    fputc(0, file_);
    // Write palette data
    for(int i=0; i<paletteSize_; i++)
    {
        fputc((int)palette_[3*i], file_);
        fputc((int)palette_[3*i+1], file_);
        fputc((int)palette_[3*i+2], file_);
    }
    
    const int minCodeSize = paletteBitDepth_;
    const uint32_t clearCode = 1 << paletteBitDepth_;
    fputc(paletteBitDepth_, file_);
    
    GifLZWNode* codetree = (GifLZWNode*)malloc(sizeof(GifLZWNode)*4096);
    memset(codetree, 0, sizeof(GifLZWNode)*4096);
    int32_t curCode = -1;
    uint32_t codeSize = (uint32_t)minCodeSize + 1;
    uint32_t maxCode = clearCode+1;
    GifBitStatus stat;
    stat.byte = 0;
    stat.bitIndex = 0;
    stat.chunkIndex = 0;

    auto GifWriteBit = [](GifBitStatus* stat, uint32_t bit)
    {
        bit = bit & 1;
        bit = bit << stat->bitIndex;
        stat->byte |= bit;
        ++stat->bitIndex;
        if( stat->bitIndex > 7 )
        {
            stat->chunk[stat->chunkIndex++] = stat->byte;
            stat->bitIndex = 0;
            stat->byte = 0;
        }
    };

    auto GifWriteChunk = [](FILE* file, GifBitStatus* stat)
    {
        fputc((int)stat->chunkIndex, file);
        fwrite(stat->chunk, 1, stat->chunkIndex, file);
        stat->bitIndex = 0;
        stat->byte = 0;
        stat->chunkIndex = 0;
    };

    auto GifWriteCode = [GifWriteBit, GifWriteChunk]
    (
        FILE* file, 
        GifBitStatus* stat, 
        uint32_t code, 
        uint32_t length
    )
    {
        for( uint32_t ii=0; ii<length; ++ii )
        {
            GifWriteBit(stat, code);
            code = code >> 1;
            if( stat->chunkIndex == 255 )
                GifWriteChunk(file, stat);
        }
    };

    // Start fresh LZW dictionary
    GifWriteCode(file_, &stat, clearCode, codeSize);
    for(uint32_t y=0; y<height_; ++y)
    {
        for(uint32_t x=0; x<width_; ++x)
        {
            uint8_t nextValue = indexedTexture_
            [
                flipVertically ? (height_-1-y)*width_+x : y*width_+x
            ];
            if (nextValue == paletteSize_)
                nextValue = 0;
            else
                ++nextValue;
            if( curCode < 0 )
                curCode = nextValue;
            else if( codetree[curCode].next[nextValue] )
                curCode = codetree[curCode].next[nextValue];
            else
            {
                GifWriteCode(file_, &stat, (uint32_t)curCode, codeSize);
                codetree[curCode].next[nextValue] = (uint16_t)++maxCode;
                if( maxCode >= (1ul << codeSize) )
                    codeSize++;
                if( maxCode == 4095 )
                {
                    GifWriteCode(file_, &stat, clearCode, codeSize);
                    memset(codetree, 0, sizeof(GifLZWNode)*4096);
                    codeSize = (uint32_t)(minCodeSize + 1);
                    maxCode = clearCode+1;
                }
                curCode = nextValue;
            }
        }
    }
    GifWriteCode(file_, &stat, (uint32_t)curCode, codeSize);
    GifWriteCode(file_, &stat, clearCode, codeSize);
    GifWriteCode(file_, &stat, clearCode + 1, (uint32_t)minCodeSize + 1);
    while( stat.bitIndex ) GifWriteBit(&stat, 0);
    if( stat.chunkIndex ) GifWriteChunk(file_, &stat);

    // Image block terminator
    fputc(0, file_); 
    free(codetree);
}

// Public functions ----------------------------------------------------------//

GifEncoder::GifEncoder():
    file_(nullptr),
    firstFrame_(false),
    width_(0),
    height_(0),
    paletteSize_(0)
{
    quantizer_ = vir::KMeansQuantizer::create();
}

GifEncoder::~GifEncoder()
{
    delete quantizer_;
}

bool GifEncoder::openFile
(
    const std::string& filepath, 
    uint32_t width, 
    uint32_t height, 
    uint32_t paletteBitDepth
)
{
    if (paletteBitDepth < 2 || width*height == 0)
        return false;

    width_ = width;
    height_ = height;
    firstFrame_ = true;

    // I need to reserve one color as the 'transparent' color for delta 
    // encoding, that is why I have that -1. The quantizer automatically
    // appends that extra 'transparent' color at the end of the palette
    // list when computeDelta=true
    paletteBitDepth_ = paletteBitDepth;
    paletteSize_ = (1<<paletteBitDepth)-1;

    #if defined(_MSC_VER) && (_MSC_VER >= 1400)
	    file_ = 0;
        fopen_s(&file_, filepath.c_str(), "wb");
    #else
        file_ = fopen(filepath.c_str(), "wb");
    #endif
    if(!file_) 
        return false;

    fputs("GIF89a", file_);

    // Screen descriptor
    fputc(width & 0xff, file_);
    fputc((width >> 8) & 0xff, file_);
    fputc(height & 0xff, file_);
    fputc((height >> 8) & 0xff, file_);

    fputc(0xf0, file_);  // Global color table of 2 colors (dummy)
    fputc(0, file_);     // Background color
    fputc(0, file_);     // Square pixel aspect ratio

    // Data of the dummy global color table (r,g,b of both dummy colors)
    fputc(0, file_);
    fputc(0, file_);
    fputc(0, file_);
    fputc(0, file_);
    fputc(0, file_);
    fputc(0, file_);

    // Animation header
    fputc(0x21, file_); // Extension
    fputc(0xff, file_); // Application specific
    fputc(11, file_); // Length 11
    fputs("NETSCAPE2.0", file_); // ...
    fputc(3, file_); // 3 bytes of NETSCAPE2.0 data
    fputc(1, file_); // Looping info
    fputc(0, file_); // Loop infinitely (byte 0)
    fputc(0, file_); // Loop infinitely (byte 1)
    fputc(0, file_); // Block terminator

    return true;
}

void GifEncoder::encodeFrame
(
    TextureBuffer2D* frame,
    int delay,
    uint32_t ditherLevel,
    float ditherThreshold,
    bool flipVertically,
    bool updatePalette
)
{
    if (file_ == nullptr)
        return;
    quantizer_->quantize
    (
        frame, 
        paletteSize_, 
        nullptr,
        ditherLevel, 
        firstFrame_,
        updatePalette,
        0,
        ditherThreshold,
        -1,
        true,
        true,
        true
    );
    quantizer_->getIndexedTexture(indexedTexture_, firstFrame_);
    quantizer_->getPalette(palette_, firstFrame_);
    if (firstFrame_)
        firstFrame_ = false;
    encodeIndexedFrame(delay, flipVertically);
}

void GifEncoder::encodeFrame
(
    Framebuffer* frame,
    int delay,
    uint32_t ditherLevel,
    float ditherThreshold,
    bool flipVertically,
    bool updatePalette
)
{
    if (file_ == nullptr)
        return;
    quantizer_->quantize
    (
        frame, 
        paletteSize_, 
        nullptr,
        ditherLevel, 
        firstFrame_,
        updatePalette,
        0,
        ditherThreshold,
        -1,
        true,
        true,
        false //
    );
    quantizer_->getIndexedTexture(indexedTexture_, firstFrame_);
    quantizer_->getPalette(palette_, firstFrame_);
    if (firstFrame_)
        firstFrame_ = false;
    encodeIndexedFrame(delay, flipVertically);
}

bool GifEncoder::closeFile()
{
    if (file_==nullptr)
        return false;
    fputc(0x3b, file_); 
    fclose(file_);
    file_ = nullptr;
    firstFrame_ = false;
    delete quantizer_;
    delete[] indexedTexture_;
    delete[] palette_;
    return true;
}

}