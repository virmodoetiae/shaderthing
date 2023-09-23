#ifndef V_KMEANS_QUANTIZER_H
#define V_KMEANS_QUANTIZER_H

#include <string>

namespace vir
{

class TextureBuffer2D;
class Framebuffer;

class KMeansQuantizer
{
protected:

    // Depending on the KMeansQuantizer implementation, it might not be able to
    // run under certain systems. For example, the current implementation of
    // the OpenGLKMeansQuantizer can only run if the min available OpenGL 
    // version in use by the host system is OpenGL 4.3, because the current
    // OpenGL implentation of the quantizer runs entirely off compute shaders,
    // which are unavailable prior to OGL 4.3
    bool canRunOnDeviceInUse_;
    std::string errorMessage_;

    // Size of the last quantized image
    uint32_t width_;
    uint32_t height_;

    // Size of the latest computed palette
    uint32_t paletteSize_;
    
    // Protected constructor as any instances of KMeansQuantizer are meant to be
    // created via the static create function
    KMeansQuantizer():
        canRunOnDeviceInUse_(true),
        width_(0),
        height_(0),
        paletteSize_(0){};

public:

    // Create a KMeansQuantizer-type object
    static KMeansQuantizer* create();

    // Destructor
    virtual ~KMeansQuantizer(){};

    // Accessors
    bool canRunOnDeviceInUse() const {return canRunOnDeviceInUse_;}
    const std::string& errorMessage() const {return errorMessage_;}
    uint32_t width(){return width_;}
    uint32_t height(){return height_;}
    uint32_t paletteSize(){return paletteSize_;}

    // Quantize the provided input (it is overwritten) by determining the 
    // n=paletteSize dominant colors. Optional parameters:
    // - inputUnit: which GL_TEXTURE_2D unit should the input be bound to;
    //              please consider that inputUnit+1 will also be reserved for
    //              internal function data storage on the GPU;
    // - reseedPalette: if true, computes an initial best-guess for the palette
    //                  based on a k-means++-like algorithm. It can be fairly
    //                  expensive for a larger paletteSize (>16), so it is
    //                  suggested to run this only if no available palettes
    //                  exist. Note that the palettes of a previous run are 
    //                  always cached (on the GPU) and available;
    // - recalculatePalette: if true, runs the k-means algorithm on the provided
    //                       inputUnit by starting form the currently available
    //                       (i.e., last cached) palette seed. The algorithm is
    //                       run until the relative clustering error change is
    //                       below the provided tolerance relTol;
    // - relTol: tolerance for running the k-means algorithm. If 0, the best 
    //           possible result is obtained;
    // - regenerateMipmap: regenerates the mipMap for the final quantized 
    //                     texture
    // - fastKMeans: if true, the k-means algorithm is not run on the bound
    //               image directly, but on a certain automatically-determined
    //               mipmap level of the image to significantly speed up both
    //               the reseedPalette and the recalculatePalette steps. The
    //               final (actual) quantization step is nonetheless run on the
    //               full mipmap-level-0 image
    virtual void quantize
    (
        TextureBuffer2D* input, 
        uint32_t paletteSize, 
        unsigned char* palette=nullptr,
        uint32_t ditherLevel=0,
        bool reseedPalette=false,
        bool recalculatePalette=true, 
        float relTol=1e-2,
        float ditherThreshold=0,
        int alphaCutoff=-1,
        bool regenerateMipmap=true,
        bool fastKMeans=true,
        bool computeDelta=false,
        uint32_t inputUnit=0
    ) = 0;

    // Quantize the framebuffer color attachment texture (it is overwritten) by
    // determining the n=paletteSize dominant colors. Optional parameters:
    // - inputUnit: which GL_TEXTURE_2D unit should the input be bound to;
    //              please consider that inputUnit+1 will also be reserved for
    //              internal function data storage on the GPU;
    // - reseedPalette: if true, computes an initial best-guess for the palette
    //                  based on a k-means++-like algorithm. It can be fairly
    //                  expensive for a larger paletteSize (>16), so it is
    //                  suggested to run this only if no available palettes
    //                  exist. Note that the palettes of a previous run are 
    //                  always cached (on the GPU) and available;
    // - recalculatePalette: if true, runs the k-means algorithm on the provided
    //                       inputUnit by starting form the currently available
    //                       (i.e., last cached) palette seed. The algorithm is
    //                       run until the relative clustering error change is
    //                       below the provided tolerance relTol;
    // - relTol: tolerance for running the k-means algorithm. If 0, the best 
    //           possible result is obtained;
    // - regenerateMipmap: regenerates the mipMap for the final quantized 
    //                     texture
    // - fastKMeans: if true, the k-means algorithm is not run on the bound
    //               image directly, but on a certain automatically-determined
    //               mipmap level of the image to significantly speed up both
    //               the reseedPalette and the recalculatePalette steps. The
    //               final (actual) quantization step is nonetheless run on the
    //               full mipmap-level-0 image
    virtual void quantize
    (
        Framebuffer* input, 
        uint32_t paletteSize, 
        unsigned char* palette=nullptr,
        uint32_t ditherLevel=0,
        bool reseedPalette=false,
        bool recalculatePalette=true, 
        float relTol=1e-2,
        float ditherThreshold=0,
        int alphaCutoff=-1,
        bool regenerateMipmap=true,
        bool fastKMeans=true,
        bool computeDelta=false,
        uint32_t inputUnit=0
    ) = 0;

    // Retrieve the palette colors and store them in the provided data array. If
    // allocate is true, the array will be re-allocated with the correct size
    virtual void getPalette
    (
        unsigned char*& data, 
        bool allocate=false
    ) = 0;

    // Retrieve the indexed colors and store them in the provided data array. If
    // allocate is true, the array will be re-allocated with the correct size
    virtual void getIndexedTexture
    (
        unsigned char*& data, 
        bool allocate=false
    ) = 0;
};

}

#endif