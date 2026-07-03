#include "vgraphics/vmisc/vgifencoder.h"
#include "vgraphics/vcore/vbuffers.h"

namespace vir
{

template <typename FrameType>
void GifEncoder::cumulatePaletteForAveraging(FrameType* frame)
{
    if (file_ == nullptr || paletteMode_ != PaletteMode::StaticAveraged)
        return;
    Quantizer::Settings quantizerOptions = {};
    quantizerOptions.cumulatePalette = true;
    quantizerOptions.indexMode = indexMode_;
    quantizerOptions.recalculatePalette = true;
    quantizerOptions.regenerateMipmap = false;
    quantizerOptions.relTol = 0.0f;
    quantizerOptions.reseedPalette = firstCumulation_;
    quantizer_->quantize
    (
        frame, 
        paletteSize_, 
        quantizerOptions
    );
    firstCumulation_ = false;
}

template <typename FrameType>
void GifEncoder::encodeFrame(FrameType* frame, const EncodingOptions& options)
{
    // Cannot encode a frame if the palette mode is static averaged and at least
    // one palette cumulation has not been performed
    if 
    (
        file_ == nullptr || 
        (
            paletteMode_ == PaletteMode::StaticAveraged && 
            firstCumulation_
        )
    )
        return;
    
    //
    if (firstFrame_ && paletteMode_ == PaletteMode::StaticAveraged)
    {
        quantizer_->getPalette(palette_, true, true);
    }

    Quantizer::Settings quantizerOptions = {};
    quantizerOptions.ditherMode = options.ditherMode;
    quantizerOptions.ditherThreshold = options.ditherThreshold;
    quantizerOptions.indexMode = indexMode_;
    quantizerOptions.reseedPalette = 
        firstFrame_ && 
        (paletteMode_ != PaletteMode::StaticAveraged);
    quantizerOptions.recalculatePalette = 
        paletteMode_ == PaletteMode::Dynamic || 
        (firstFrame_ && paletteMode_ == PaletteMode::StaticFirstFrame);
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
    if (quantizerOptions.reseedPalette || quantizerOptions.recalculatePalette)
        quantizer_->getPalette(palette_, firstFrame_);
    encodeIndexedFrame
    (
        options.delay, 
        options.flipVertically
    );
    firstFrame_ = false;
}

}