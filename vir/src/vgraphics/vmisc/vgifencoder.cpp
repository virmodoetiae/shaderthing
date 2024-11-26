#include "vpch.h"
#include "vgraphics/vmisc/vgifencoder.h"

namespace vir
{

std::map<GifEncoder::PaletteMode, const char*>
GifEncoder::paletteModeToName = 
{
    {GifEncoder::PaletteMode::Dynamic, "Dynamic"},
    {GifEncoder::PaletteMode::StaticAveraged, "Static (averaged)"},
    {GifEncoder::PaletteMode::StaticFirstFrame, "Static (1st frame)"}
};

// Protected functions -------------------------------------------------------//

void GifEncoder::writePaletteData()
{
    if (file_ == nullptr || palette_ == nullptr)
        return;
    
    // Write dummy color which will be used for transparency only
    if (indexMode_ != Quantizer::Settings::IndexMode::Default)
    {
        fputc(0, file_);
        fputc(0, file_);
        fputc(0, file_);
    }
    // Write palette data
    for(uint32_t i=0; i<paletteSize_; i++)
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
}

void GifEncoder::encodeIndexedFrame
(
    int delay, 
    bool flipVertically
)
{
    if (firstFrame_)
    {
        // Comments are referred to the different sections in the latest GIF
        // specification at https://www.w3.org/Graphics/GIF/spec-gif89a.txt

        // 17. HEADER, bytes 0-2 "GIF", bytes 3-5 "89a" ------------------------
        fputs("GIF89a", file_);

        // 18. LOGICAL SCREEN DESCRIPTOR, bytes 0-3 ----------------------------
        fputc(width_ & 0xff, file_);
        fputc((width_ >> 8) & 0xff, file_);
        fputc(height_ & 0xff, file_);
        fputc((height_ >> 8) & 0xff, file_);
        
        // Byte 4 of LOGICAL SCREEN DESCRIPTOR
        uint8_t packedFields = 0;
        packedFields |= (1 << 7); // Bit 7, global color table always is present
        packedFields |= (7 << 4); // Bits 6,5,4 equal to number of bits per 
                                  // primary color minus one (always 7 in our 
                                  // case)
        if (paletteMode_ != PaletteMode::Dynamic) // Bits 2,1,0 equal to 
                                                  // log2(paletteSize)-1 of
                                                  // global palette
            packedFields |= ((paletteBitDepth_-1) << 0);

        fputc(packedFields, file_);
        fputc(0, file_); // Byte 5, background color index (always 0 in my
                         // setup)
        fputc(0, file_); // Byte 6, '0' means square pixel aspect ratio
        
        // 19. GLOBAL COLOR TABLE ----------------------------------------------
        if (paletteMode_ != PaletteMode::Dynamic)
            writePaletteData();
        else // If bits 2,1,0 of byte 4 of the logical screen descriptor are set
             // to 0, that still corresponds to a global color table with two
             // colors. Thus, put two dummy colorsin such case (with 3 
             // components each, so 6 values in total). I might have set bit 7
             // to the appropriate state if no global color table is present,
             // but I noticed that that prevents the resulting GIFs to be read
             // correctly by various pieces of software (they still render ok 
             // though). So I keep the global color table just in case
        {
            for (int i=0; i<6; i++)
                fputc(0, file_);
        }

        // 26. APPLICATION EXTENSION -------------------------------------------
        fputc(0x21, file_); // EXTENSION INTRODUCER (fixed value, 0x21)
        fputc(0xff, file_); // EXTENSION LABEL (fixed value, 0xff)
        fputc(11, file_); // EXTENSION SIZE (fixed value, 11)
        fputs("ANIMEXTS1.0", file_); // APPLICATION IDENTIFIER (ANIMEXTS) and
                                     // AUTHENTICATION CODE for animated GIFs,
                                     // (same as NETSCAPE2.0)
        fputc(3, file_); // Size of ANIMEXTS1 APPLICATION DATA (3 bytes)
        fputc(1, file_); // Byte 0 of ANIMEXTS data, 1 -> looping on
        fputc(0, file_); // Bytes 1,2 of ANIMEXTS data, 00 -> loop infinitely
        fputc(0, file_);
        fputc(0, file_); // Block terminator
    }

    // 23. GRAPHIC CONTROL EXTENSION preceeding the image descriptor -----------
    fputc(0x21, file_); // Byte 0, EXTENSION INTRODUCER (fixed value)
    fputc(0xf9, file_); // Byte 1, GRAPHIC CONTROL LABEL (fixed value)
                        // Byte 2, BLOCK SIZE
    fputc(0x04, file_); 

    // Byte 3 - packed data
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
    
    // Bytes 4,5 - delay
    fputc(delay & 0xff, file_);
    fputc((delay >> 8) & 0xff, file_);
    
    // Byte 6 - transparent color palette index (even if not applicable)
    fputc(0, file_);
    
    fputc(0, file_); // Block terminator for GRAPHIC CONTROL EXTENSION

    // 20. IMAGE DESCRIPTOR BLOCK ----------------------------------------------
    // A lot of optimizations might be implemented acting here, as I could e.g.,
    // only write the actual region of the GIF which has been updated, compared
    // to the previous frame. I am not doing this, presently

    fputc(0x2c, file_); // IMAGE SEPARATOR (fixed value, 0x2c)
    fputc(0 & 0xff, file_); // IMAGE POSITION (i.e., coordinates of the top-left
                            // image corner with respect to an origin at the 
                            // top-left GIF corder; bytes 0,1 are for the 
                            // horizontal offset, bytes 2,3 for the vertical one
    fputc((0 >> 8) & 0xff, file_);
    fputc(0 & 0xff, file_);
    fputc((0 >> 8) & 0xff, file_);

    fputc(width_ & 0xff, file_); // IMAGE WIDTH (bytes 4-5)
    fputc((width_ >> 8) & 0xff, file_);
    fputc(height_ & 0xff, file_); // IMAGE HEIGHT (bytes 6-7)
    fputc((height_ >> 8) & 0xff, file_);

    uint8_t packedFields = 0;
    if (paletteMode_ == PaletteMode::Dynamic)
    {
        packedFields |= (1 << 7); // Bit 7 == 1 means local palette present
                                  // Bits 6,5 control interlacing, sort, all set
                                  // to OFF, while bits 4,3 are reserved and
                                  // should not be set
        packedFields += paletteBitDepth_-1;// Bits 2,1,0 are log2(paletteSize)-1
        fputc(packedFields, file_);
        // 21. LOCAL COLOR TABLE -----------------------------------------------
        writePaletteData();
    }
    else
        fputc(packedFields, file_); // No local color table

    const int minCodeSize = paletteBitDepth_;
    const uint32_t clearCode = 1 << paletteBitDepth_;
    fputc(paletteBitDepth_, file_);
    
    // 22. TABLE BASED IMAGE DATA ----------------------------------------------
    // This is pure magic I took from https://github.com/charlietangora/gif-h,
    // much appreciated (licensed under the "Unlicense" license)

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

    // Image block terminator for TABLE BASED IMAGE DATA
    fputc(0, file_); 
    free(codetree);
}

// Public functions ----------------------------------------------------------//

GifEncoder::GifEncoder():
    file_(nullptr),
    firstFrame_(false),
    firstCumulation_(false),
    width_(0),
    height_(0),
    paletteSize_(0),
    frameCounter_(0),
    paletteMode_(PaletteMode::Dynamic),
    indexMode_(IndexMode::Default)
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
    uint32_t paletteBitDepth,
    PaletteMode paletteMode,
    IndexMode indexMode
)
{
    #if defined(_MSC_VER) && (_MSC_VER >= 1400)
	    file_ = 0;
        fopen_s(&file_, filepath.c_str(), "wb");
    #else
        file_ = fopen(filepath.c_str(), "wb");
    #endif
    if(!file_ || paletteBitDepth < 1 || width*height == 0) 
        return false;

    width_ = width;
    height_ = height;
    firstFrame_ = true;
    firstCumulation_ = true;
    paletteBitDepth_ = paletteBitDepth;
    paletteMode_ = paletteMode;
    paletteSize_ = (1<<paletteBitDepth);

    // I need to reserve one color as the 'transparent' color for delta or alpha
    // encoding only if my paletteSize is already at max capacity, i.e. 256 (due 
    // to the GIF format limitations)
    indexMode_ = indexMode;
    if (indexMode_ != Quantizer::Settings::IndexMode::Default)
    {
        paletteBitDepth_ = std::min(paletteBitDepth_+1, 8u);
        if (paletteSize_ == 256)
            paletteSize_ -= 1;
    }

    return true;
}

bool GifEncoder::closeFile()
{
    if (file_ == nullptr)
        return false;

    // https://www.w3.org/Graphics/GIF/spec-gif89a.txt
    // 27. TRAILER
    fputc(0x3b, file_); // END OF GIF (fixed value, 0x3b) 
    fclose(file_);
    
    file_ = nullptr;
    width_ = 0;
    height_ = 0;
    paletteSize_ = 0;
    indexMode_ = IndexMode::Default;
    delete[] indexedTexture_;
    delete[] palette_;
    return true;
}

}