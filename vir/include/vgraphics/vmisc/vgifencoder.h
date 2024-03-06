#ifndef V_GIF_ENCODER_H
#define V_GIF_ENCODER_H

#include <string>
#include "vgraphics/vpostprocess/vquantizer.h"

class GifFileType;

namespace vir
{

class TextureBuffer2D;
class Framebuffer;

class GifEncoder
{
public:

    typedef Quantizer::Settings::IndexMode IndexMode;
    typedef Quantizer::Settings::DitherMode DitherMode;

    struct EncodingOptions
    {
        // Delay between frames in hundredths of a second
        int delay = 4;

        // 
        bool flipVertically = false;

        //
        DitherMode ditherMode = DitherMode::None;
        
        // 
        float ditherThreshold = 0;

        //
        int alphaCutoff = -1;
        
        //
        bool updatePalette = true;
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
        uint8_t bitIndex;
        uint8_t byte;      
        uint32_t chunkIndex;
        uint8_t chunk[256];
    } GifBitStatus;

    FILE* file_ = nullptr;
    bool firstFrame_;
    uint32_t width_, height_, paletteBitDepth_, paletteSize_;
    unsigned char* indexedTexture_;
    unsigned char* palette_;
    IndexMode indexMode_;

    Quantizer* quantizer_;
    
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
        IndexMode indexMode=IndexMode::Default
    );
    
    void encodeFrame
    (
        TextureBuffer2D* frame,
        const EncodingOptions& options
    );
    
    void encodeFrame
    (
        Framebuffer* frame,
        const EncodingOptions& options
    );
    
    bool closeFile();

};

}

#endif