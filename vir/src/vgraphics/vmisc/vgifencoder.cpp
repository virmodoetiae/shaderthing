#include "vpch.h"
#include "vgraphics/vmisc/vgifencoder.h"

namespace vir
{

// Protected functions -------------------------------------------------------//

void GifEncoder::encodeIndexedFrame
(
    int delay, 
    bool flipVertically
)
{
    // Graphics control extension
    fputc(0x21, file_);
    fputc(0xf9, file_);
    fputc(0x04, file_);
    switch(indexMode_)
    {
        case Quantizer::Settings::IndexMode::Default :
            fputc(0b00001000, file_);   // Old frame reset to no color,
                                        // no transparency
            break;
        case Quantizer::Settings::IndexMode::Alpha :
            fputc(0b00001001, file_);   // Old frame reset to no color, 
                                        // enable transparency
            break;
        case Quantizer::Settings::IndexMode::Delta :
            fputc(0b00000101, file_);   // Old frame left in place, 
                                        // enable transparency
            break;
    }
    fputc(delay & 0xff, file_);
    fputc((delay >> 8) & 0xff, file_);
    //if (indexMode_ != Quantizer::Settings::IndexMode::Default)
    fputc(0, file_); // Transparent color index (Apparently I need this 
                     // regardless of the transparency color flag, in
                     // spite of the GIF89a specs, or the GIF, while 
                     // renderable, cannot be properly read by either
                     // stbi or online GIF editing tools...)
    fputc(0, file_); // Block terminator

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
    
    // Write dummy color which will be used for transparency only
    if (indexMode_ != Quantizer::Settings::IndexMode::Default)
    {
        fputc(0, file_);
        fputc(0, file_);
        fputc(0, file_);
    }
    // Write palette data
    for(int i=0; i<paletteSize_; i++)
    {
        fputc((int)palette_[3*i], file_);
        fputc((int)palette_[3*i+1], file_);
        fputc((int)palette_[3*i+2], file_);
    }

    // Pad palette if necessary. This only happens if 
    // bitDepth < 8 and indexMode != Default. Then the extra color used for 
    // transparency (the first we wrote) causes our palette to require one more
    // color, but since the GIF format only works with palette sizes in power of
    // 2, we need to add a bunch of dummy colors to the palette to make it work
    int ps = paletteSize_;
    while (ps < ((1<<paletteBitDepth_)-1))
    {
        fputc(0, file_);
        fputc(0, file_);
        fputc(0, file_);
        ++ps;
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
            if (indexMode_ != Quantizer::Settings::IndexMode::Default)
            {
                if (nextValue == paletteSize_)
                    nextValue = 0;
                else
                    ++nextValue;
            }
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

GifEncoder::GifEncoder(Quantizer::Settings::IndexMode indexMode):
    file_(nullptr),
    firstFrame_(false),
    width_(0),
    height_(0),
    paletteSize_(0),
    indexMode_(indexMode)
{
    quantizer_ = Quantizer::create();
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

    paletteBitDepth_ = paletteBitDepth;
    paletteSize_ = (1<<paletteBitDepth);
    // I need to reserve one color as the 'transparent' color for delta or alpha
    // encoding only if my paletteSize is already at max capacity, i.e. 256 (due 
    // to the GIF format limitations)
    if (indexMode_ != Quantizer::Settings::IndexMode::Default)
    {
        paletteBitDepth_ = std::min(paletteBitDepth_+1, 8u);
        if (paletteSize_ == 256)
            paletteSize_ -= 1;
    }

    #if defined(_MSC_VER) && (_MSC_VER >= 1400)
	    file_ = 0;
        fopen_s(&file_, filepath.c_str(), "wb");
    #else
        file_ = fopen(filepath.c_str(), "wb");
    #endif
    if(!file_) 
        return false;

    // Here we gooo, have a read at 
    // https://www.w3.org/Graphics/GIF/spec-gif89a.txt

    fputs("GIF89a", file_);

    // Screen descriptor
    fputc(width & 0xff, file_);
    fputc((width >> 8) & 0xff, file_);
    fputc(height & 0xff, file_);
    fputc((height >> 8) & 0xff, file_);
    
    //   Reverted back to including a dummy global color table even if not used
    //   at all, mainly for compatibility with some GIF decoders
    // - first bit is global color table yet/no (here is yes)
    // - bits 2,3,4 are the the number of bits -1 per primary color of the 
    //   incoming frames, here set to 111 = 7, i.e. 7+1=8 bit depth
    // - bit 5 signals whether the global color table is sorted by frequency 
    //   or not (here is no)
    // - bits 6,7,8 should be equal to log2(size global color table-1), set to
    //   0 if no global color table. In this case, the global color table has
    //   2 dummy colors, hence the last three bytes represent log2(2-1) = 0
    fputc(0b11110000, file_);
    fputc(0, file_);     // Background color index (forced to have this even if
                         // table-on-off bit (bit 1) were the were to be 0)
    fputc(0, file_);     // Square pixel aspect ratio
    for (int i=0; i<6; i++) // Write r,g,b, components of the two dummy colors
        fputc(0, file_);

    // Animation header
    fputc(0x21, file_); // Extension
    fputc(0xff, file_); // Application specific
    fputc(11, file_); // Length 11
    fputs("NETSCAPE2.0", file_);
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
    const EncodingOptions& options
)
{
    if (file_ == nullptr)
        return;
    Quantizer::Settings quantizerOptions = {};
    quantizerOptions.ditherMode = options.ditherMode;
    quantizerOptions.ditherThreshold = options.ditherThreshold;
    quantizerOptions.indexMode = indexMode_;
    quantizerOptions.reseedPalette = firstFrame_;
    quantizerOptions.recalculatePalette = options.updatePalette;
    quantizerOptions.relTol = 0.0f;
    quantizerOptions.alphaCutoff = options.alphaCutoff;
    quantizerOptions.regenerateMipmap = true;
    quantizerOptions.fastKMeans = true;
    quantizerOptions.overwriteInput = true;
    quantizer_->quantize
    (
        frame, 
        paletteSize_, 
        quantizerOptions
    );
    quantizer_->getIndexedTexture(indexedTexture_, firstFrame_);
    quantizer_->getPalette(palette_, firstFrame_);
    if (firstFrame_)
        firstFrame_ = false;
    encodeIndexedFrame
    (
        options.delay, 
        options.flipVertically
    );
}

void GifEncoder::encodeFrame
(
    Framebuffer* frame,
    const EncodingOptions& options
)
{
    if (file_ == nullptr)
        return;
    Quantizer::Settings quantizerOptions = {};
    quantizerOptions.ditherMode = options.ditherMode;
    quantizerOptions.ditherThreshold = options.ditherThreshold;
    quantizerOptions.indexMode = indexMode_;
    quantizerOptions.reseedPalette = firstFrame_;
    quantizerOptions.recalculatePalette = options.updatePalette;
    quantizerOptions.relTol = 0.0f;
    quantizerOptions.alphaCutoff = options.alphaCutoff;
    quantizerOptions.regenerateMipmap = true;
    quantizerOptions.fastKMeans = true;
    quantizerOptions.overwriteInput = true;
    quantizer_->quantize
    (
        frame, 
        paletteSize_, 
        quantizerOptions
    );
    quantizer_->getIndexedTexture(indexedTexture_, firstFrame_);
    quantizer_->getPalette(palette_, firstFrame_);
    if (firstFrame_)
        firstFrame_ = false;
    encodeIndexedFrame
    (
        options.delay, 
        options.flipVertically
    );
}

bool GifEncoder::closeFile()
{
    if (file_==nullptr)
        return false;
    fputc(0x3b, file_); 
    fclose(file_);
    file_ = nullptr;
    firstFrame_ = false;
    delete[] indexedTexture_;
    delete[] palette_;
    return true;
}

}