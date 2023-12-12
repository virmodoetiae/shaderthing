#ifndef V_OPENGL_KMEANS_QUANTIZER_H
#define V_OPENGL_KMEANS_QUANTIZER_H

#include "thirdparty/glad/include/glad/glad.h"
#include <unordered_map>
#include "vgraphics/vmisc/vkmeansquantizer.h"

namespace vir
{

class TextureBuffer2D;
class Framebuffer;

class OpenGLKMeansQuantizer : public KMeansQuantizer
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
        const Options& options,
        bool float32=false
    );

    //
    void waitSync();
    void resetSync();

public:

    OpenGLKMeansQuantizer();
    ~OpenGLKMeansQuantizer();

    // Overloaded quantization functions for vir:: objects
    virtual void quantize
    (
        TextureBuffer2D* input, 
        unsigned int paletteSize,
        const Options& options
    ) override;
    virtual void quantize
    (
        Framebuffer* input, 
        unsigned int paletteSize,
        const Options& options
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