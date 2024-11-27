#ifndef V_QUANTIZER_H
#define V_QUANTIZER_H

#include "vgraphics/vpostprocess/vpostprocess.h"
#include <unordered_map>

namespace vir
{

class TextureBuffer2D;
class Framebuffer;

// A post-processing effect which quantizes the color range of an input
// Framebuffer or TextureBuffer2D
class Quantizer : public PostProcess
{
public:

struct Settings
{
    // Texture index mode
    enum IndexMode
    {
        Default = 0,
        Alpha = 1,
        Delta = 2
    };

    // Dither mode, ranging from no dithering, to a 2x2 dithering kernel, to a
    // 4x4 one
    enum class DitherMode
    {
        None = 0,
        Order2 = 1,
        Order4 = 2,
    };

    // Name mappings for convenience
    static std::unordered_map<Settings::IndexMode, std::string> 
        indexModeToName;
    static std::unordered_map<Settings::DitherMode, std::string> 
        ditherModeToName;

    // Palette color data used for quantization if provided, if not provided
    // the palette is determined automatically by the KMeans algorithm
    unsigned char* paletteData=nullptr;

    // If true, a KMeans++-like (but deterministic) algorithm is used to
    // refresh the starting palette which is used as a guess value by the 
    // KMeans clustering algorithm. It is a fairly computationally expensive
    // step. Please note that if the internal quantizer palette cache is
    // empty (e.g., right after quantizer initialization), this option will
    // default to true until at least one palette is cached
    bool reseedPalette=false;

    // If true, run the KMeans clustering algorithm to determined the best
    // possible palette of paletteSize colors for the quantization of the 
    // provided texture, starting from the latest available palette seed
    bool recalculatePalette=true;

    // When the relative difference between the total clustering errors of
    // successive KMeans iterations falls below relTol, the KMeans algorithm is
    // deemed converged and terminated. Set to 0 for the best possible result,
    // given the seed palette determined by the reseed/recalculation steps
    float relTol=1e-2;

    // Selected ordered dithering mode
    DitherMode ditherMode=DitherMode::None;

    // A value in the [0.0, 1.0] range that determines the 'aggressiveness' of 
    // the dithering process. The larger the value, the more dithered the image.
    // If ditherMode == DitherMode::None, no dithering is performed on the 
    // quantized
    float ditherThreshold=0;

    // Different modes of texture indexing:
    // - IndexMode::Default : for a palette of ordered n=paletteSize colors, the
    //   indexed texture indices represent the index of the chosen pixel color
    //   in the palette, for each texture pixel;
    // - IndexMode::Delta : modifies texture indexing so that index number
    //   paletteSize-1 is reserved for pixels whose color is left unchanged 
    //   between successive calls of the quantize function. Useful for e.g., GIF
    //   delta encoding. Please note that the final palette size (actual number
    //   of quantization colors) will be paletteSize-1, not paletteSize;
    // - IndexMode::Alpha : modifies texture indexing so that index number
    //   paletteSize-1 is reserved for pixels whose alpha channel (when 
    //   interpreted as an 8-bit integer) is below alphaCutoff (i.e., if the
    //   pixel is to be interpreted as fully transparent). If a value for
    //   alphaCutoff is not specified (i.e., if it is left at its default value
    //   of -1) and this IndexMode is selected, alphaCutoff with re-default to
    //   127 (i.e., half-way in the [0, 255] range). Please note that the final 
    //   palette size (actual numberof quantization colors) will be
    //   paletteSize-1, not paletteSize
    IndexMode indexMode=IndexMode::Default;

    // A value in the [-1, 255] range which, if > -1, quantizes the alpha 
    // channel of each quantized texture pixel to either fully opaque or fully
    // transparent (regardless of the texture format), depending on whether the
    // original alpha channel value (interpreted as an 8-bit int) is below or 
    // above alphaCutoff, respectively. If == -1, no alpha channel quantization
    // is performed, unless indexMode is set to IndexMode::Alpha. In such case,
    // alphaCutoff default to 127 if not provided
    int alphaCutoff=-1;

    // If true, the KMeans algorithm is not run on the texture image as-is, 
    // but it is run on a certain automatically-determined mipmap level of
    // the image to significantly speed up both the reseedPalette and the
    // recalculatePalette steps. The final (actual) quantization step is
    // nonetheless run on the full mipmap-level-0 image
    bool fastKMeans=true;
    
    // If true, mimaps of the quantized texture will be re-generated after 
    // quantization
    bool regenerateMipmap=true;

    //
    bool overwriteInput=false;

    // The target TEXTURE2D unit to which the input texture will be bound
    // in order to perform the quantization. Inconsequential on the quantization
    // process
    unsigned int inputUnit=0;

    // The quantizer uses an SSBO for some internal operations. Said SSBO
    // requires a binding point, which is automatically determined on the first
    // quantizer run. For subsequent runs, in case changes in the availability of
    // SSBO binding points are expected, set this flag to true. If you know SSBO
    // binding point availability has not changed, leave it to false
    bool ensureFreeSSBOBinding=true;

    // If true, the palette computed from this quantization call will
    // be appended to a texture of palettes
    bool cumulatePalette=false;
};

protected:

    // Size of the last quantized image
    uint32_t width_;
    uint32_t height_;

    // (Real) size of the latest computed palette
    uint32_t paletteSize_;

    // (Cached) set of settings used for the latest quantize call
    Settings settings_;
    
    // Protected constructor as any instances of Quantizer are meant to be
    // created via the static create function
    Quantizer() :
        PostProcess(Type::Quantization),
        width_(0),
        height_(0),
        paletteSize_(0),
        settings_({}){}

    // Delete copy-construction & copy-assignment ops
    Quantizer(const Quantizer&) = delete;
    Quantizer& operator= (const Quantizer&) = delete;

public:

    // Create a Quantizer-type object
    static Quantizer* create();

    // Destructor
    virtual ~Quantizer(){}

    // Accessors
    uint32_t width(){return width_;}
    uint32_t height(){return height_;}
    uint32_t paletteSize(){return paletteSize_;}

    // Quantize the provided texture (it is overwritten) with the provided 
    // settings
    virtual void quantize
    (
        TextureBuffer2D* input, 
        unsigned int paletteSize,
        const Settings& settings
    ) = 0;

    // Quantize the framebuffer color attachment texture (it is overwritten)
    // with the provided settings
    virtual void quantize
    (
        Framebuffer* input, 
        unsigned int paletteSize,
        const Settings& settings
    ) = 0;

    // Retrieve the palette colors and store them in the provided data array. If
    // allocate is true, the array will be re-allocated with the correct size
    virtual void getPalette
    (
        unsigned char*& data, 
        bool allocate=false,
        bool cumulated=false
    ) = 0;

    // Retrieve the indexed colors and store them in the provided data array. If
    // allocate is true, the array will be re-allocated with the correct size
    virtual void getIndexedTexture
    (
        unsigned char*& data, 
        bool allocate=false
    ) = 0;

    //...
    virtual int getCumulatedPaletteImageId() const = 0;
};

}

#endif