#ifndef V_OPENGL_QUANTIZER_H
#define V_OPENGL_QUANTIZER_H

#include "vgraphics/vpostprocess/vquantizer.h"
#include "thirdparty/glad/include/glad/glad.h"
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

    class ComputeShaderStage
    {
    protected:
        GLuint id_;
        std::string source_;
        std::unordered_map<std::string, GLint> uniformLocations_;
    public:
        ComputeShaderStage(std::string source):id_(0),source_(source){}
        ~ComputeShaderStage();
        void compile();
        GLint getUniformLocation(std::string& uniformName);
        void setUniformInt(std::string uniformName,int value,bool autoUse=true);
        void setUniformFloat
        (
            std::string uniformName,
            float value,
            bool autoUse=true
        );
        void use();
        void run(int x, int y, int z, GLbitfield barriers=GL_ALL_BARRIER_BITS);
    };

    static bool computeShaderStagesCompiled;
    static ComputeShaderStage computeShader_findMaxSqrDistColSF32;
    static ComputeShaderStage computeShader_setNextPaletteColSF32;
    static ComputeShaderStage computeShader_buildClustersFromPaletteSF32;
    static ComputeShaderStage computeShader_updatePaletteFromClustersSF32;
    static ComputeShaderStage computeShader_quantizeInputSF32;
    static ComputeShaderStage computeShader_findMaxSqrDistColUI8;
    static ComputeShaderStage computeShader_setNextPaletteColUI8;
    static ComputeShaderStage computeShader_buildClustersFromPaletteUI8;
    static ComputeShaderStage computeShader_updatePaletteFromClustersUI8;
    static ComputeShaderStage computeShader_quantizeInputUI8;

    // R32UI Texture_2D used to store data required by the algorithm on the GPU,
    // primarily palette colors, quantization error
    GLuint paletteData_;
    GLuint paletteDataPBO_;
    GLuint paletteDataWriteOnlyPBO_;

    //
    GLuint indexedData_;
    GLuint indexedDataPBO_;
    
    //
    bool firstWaitSyncCall_;
    GLsync dataSync_;

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
    void waitSync();
    void resetSync();

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
        bool allocate=false
    ) override;

    // Retrieve the indexed colors and store them in the provided data array. If
    // allocate is true, the array will be re-allocated with the correct size
    void getIndexedTexture
    (
        unsigned char*& data, 
        bool allocate=false
    ) override;

};

}

#endif