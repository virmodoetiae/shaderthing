#ifndef V_OPENGL_QUANTIZER_H
#define V_OPENGL_QUANTIZER_H

#include "vgraphics/vpostprocess/vquantizer.h"
#include "vgraphics/vcore/vopengl/vopenglcomputeshader.h"
#include <unordered_map>

namespace vir
{

class TextureBuffer2D;
class Framebuffer;

// An OpenGL implementation of the color quantizer leveraging a K-Means
// algorithm implemented on compute shaders. As such, it requires OpenGL
// version 4.3 or higher to run
class OpenGLQuantizer : public Quantizer
{
protected:

    static bool computeShaderStagesCompiled;
    static OpenGLComputeShader computeShader_findMaxSqrDistColSF32;
    static OpenGLComputeShader computeShader_setNextPaletteColSF32;
    static OpenGLComputeShader computeShader_buildClustersFromPaletteSF32;
    static OpenGLComputeShader computeShader_updatePaletteFromClustersSF32;
    static OpenGLComputeShader computeShader_quantizeInputSF32;
    static OpenGLComputeShader computeShader_findMaxSqrDistColUI8;
    static OpenGLComputeShader computeShader_setNextPaletteColUI8;
    static OpenGLComputeShader computeShader_buildClustersFromPaletteUI8;
    static OpenGLComputeShader computeShader_updatePaletteFromClustersUI8;
    static OpenGLComputeShader computeShader_quantizeInputUI8;

    // R32UI Texture_2D used to store data required by the algorithm on the GPU,
    // primarily palette colors, quantization error
    GLuint paletteData_;
    GLuint paletteDataPBO_;
    GLuint paletteDataWriteOnlyPBO_;

    //
    GLint maxNCumulatedPalettes_;
    GLuint cumulatedPaletteData_;
    unsigned int cumulatedPaletteRow_ = 0;

    //
    GLuint indexedData_;
    GLuint indexedDataPBO_;
    
    //
    //bool firstWaitSyncCall_;
    //GLsync dataSync_;

    // Persistently GPU-to-CPU mapped palette data and indices data
    void* mappedPaletteData_;
    void* mappedIndexedData_;

    //
    void* mappedWriteOnlyPaletteData_;
    
    // Atomic counter for storing the clustering error
    GLuint clusteringError_;

    // 
    GLuint oldQuantizedInput_;

    //
    void quantizeOpenGLTexture
    (
        GLuint id, // inputId
        uint32_t width,
        uint32_t height,
        unsigned int paletteSize,
        const Settings& settings,
        bool float32=false
    );

    //
    //void waitSync();
    //void resetSync();

    // Delete copy-construction & copy-assignment ops
    OpenGLQuantizer(const OpenGLQuantizer&) = delete;
    OpenGLQuantizer& operator= (const OpenGLQuantizer&) = delete;

public:
    
    //
    OpenGLQuantizer();
    virtual ~OpenGLQuantizer();

    // Overloaded quantization functions for vir:: objects
    virtual void quantize
    (
        TextureBuffer2D* input, 
        unsigned int paletteSize,
        const Settings& settings
    ) override;
    virtual void quantize
    (
        Framebuffer* input, 
        unsigned int paletteSize,
        const Settings& settings
    ) override;

    // Retrieve the palette colors and store them in the provided data array. If
    // allocate is true, the array will be re-allocated with the correct size
    void getPalette
    (
        unsigned char*& data, 
        bool allocate=false,
        bool cumulated=false
    ) override;

    // Retrieve the indexed colors and store them in the provided data array. If
    // allocate is true, the array will be re-allocated with the correct size
    void getIndexedTexture
    (
        unsigned char*& data, 
        bool allocate=false
    ) override;

    int getCumulatedPaletteImageId() const override {return cumulatedPaletteData_;}
};

}

#endif