#ifndef V_GIF_ENCODER_H
#define V_GIF_ENCODER_H

#include <string>
#include "vgraphics/vpostprocess/vquantizer.h"

class GifFileType;

namespace vir
{

class GifEncoder
{
public:

    typedef Quantizer::Settings::IndexMode IndexMode;
    typedef Quantizer::Settings::DitherMode DitherMode;

    struct EncodingOptions
    {
        int        delay           = 4;
        bool       flipVertically  = false;
        DitherMode ditherMode      = DitherMode::None;
        float      ditherThreshold = 0;
        int        alphaCutoff     = -1;
    };

    enum class PaletteMode
    {
        StaticAveraged,
        StaticFirstFrame,
        Dynamic
    };

protected:

    //
    typedef struct
    {
        uint16_t next[256];
    } GifLZWNode;

    // Simple structure to write out the LZW-compressed portion of the image
    // one bit at a time
    typedef struct
    {
        uint8_t  bitIndex;
        uint8_t  byte;      
        uint32_t chunkIndex;
        uint8_t  chunk[256];
    } GifBitStatus;

    FILE*          file_ = nullptr;
    bool           firstFrame_, firstCumulation_;
    uint32_t       width_, height_, paletteBitDepth_, paletteSize_, frameCounter_;
    unsigned char* indexedTexture_;
    unsigned char* palette_;
    PaletteMode    paletteMode_;
    IndexMode      indexMode_;

    Quantizer*     quantizer_;
    
    void encodeIndexedFrame(int delay, bool flipVertically);

public:

    GifEncoder();
    ~GifEncoder();

    bool isFileOpen() const {return file_ != nullptr;}
    bool canRunOnDeviceInUse() const {return quantizer_->canRunOnDeviceInUse();}
    const std::string& errorMessage() const {return quantizer_->errorMessage();}

    bool openFile
    (
        const std::string& filepath, 
        uint32_t width, 
        uint32_t height, 
        uint32_t paletteBitDepth=8,
        PaletteMode paletteMode=PaletteMode::Dynamic,
        IndexMode indexMode=IndexMode::Default
    );

    bool closeFile();

    template
    <
        typename FrameType/*, 
        typename = typename std::enable_if
        <
            std::is_same<FrameType, vir::Framebuffer>::value || 
            std::is_same<FrameType, vir::TextureBuffer2D>::value
        >*/
    >
    void cumulatePaletteForAveraging(FrameType* frame);

    template 
    <
        typename FrameType/*, 
        typename = typename std::enable_if
        <
            std::is_same<FrameType, vir::Framebuffer>::value || 
            std::is_same<FrameType, vir::TextureBuffer2D>::value
        >*/
    >
    void encodeFrame
    (
        FrameType* frame,
        const EncodingOptions& options
    );
};

}

#include "vgraphics/vmisc/vgifencoder_impl.h"

#endif