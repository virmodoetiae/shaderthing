#ifndef V_GIF_ENCODER_H
#define V_GIF_ENCODER_H

#include <string>

class GifFileType;

namespace vir
{

class TextureBuffer2D;
class Framebuffer;
class KMeansQuantizer;

class GifEncoder
{
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

    FILE* file_;
    bool firstFrame_;
    uint32_t width_, height_, paletteBitDepth_, paletteSize_;
    unsigned char* indexedTexture_;
    unsigned char* palette_;

    KMeansQuantizer* quantizer_;
    
    void encodeIndexedFrame(int delay, bool flipVertically);

public:

    GifEncoder();
    ~GifEncoder();

    bool openFile
    (
        const std::string& filepath, 
        uint32_t width, 
        uint32_t height, 
        uint32_t paletteBitDepth=8
    );
    
    void encodeFrame
    (
        TextureBuffer2D* frame,
        int delay,
        uint32_t ditherLevel=0,
        float ditherThreshold=0,
        bool flipVertically=false,
        bool updatePalette=true
    );
    
    void encodeFrame
    (
        Framebuffer* frame,
        int delay,
        uint32_t ditherLevel=0,
        float ditherThreshold=0,
        bool flipVertically=false,
        bool updatePalette=true
    );
    
    bool closeFile();

};

}

#endif