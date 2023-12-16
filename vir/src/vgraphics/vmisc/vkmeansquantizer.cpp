#include "vpch.h"
#include "vgraphics/vmisc/vkmeansquantizer.h"
#include "vgraphics/vmisc/vopengl/vopenglkmeansquantizer.h"

namespace vir
{

typedef KMeansQuantizer::Settings::IndexMode IndexMode;
typedef KMeansQuantizer::Settings::DitherMode DitherMode;

std::unordered_map<IndexMode, std::string> 
    KMeansQuantizer::Settings::indexModeToName = 
    {
        {IndexMode::Delta, "Delta indexing"},
        {IndexMode::Alpha, "Alpha indexing"},
        {IndexMode::Default, "Default indexing"}
        
    };

std::unordered_map<DitherMode, std::string> 
    KMeansQuantizer::Settings::ditherModeToName = 
    {
        {DitherMode::Order4, "4x4 Kernel"},
        {DitherMode::Order2, "2x2 Kernel"},
        {DitherMode::None, "None"}
    };

KMeansQuantizer* KMeansQuantizer::create()
{
    Window* window = nullptr;
    if (!GlobalPtr<Window>::valid(window))
        return nullptr;
    switch(window->context()->type())
    {
        case (GraphicsContext::Type::OpenGL) :
            return new OpenGLKMeansQuantizer();
    }
    return nullptr;
}

KMeansQuantizer::~KMeansQuantizer()
{
    if (output_ != nullptr) 
        delete output_;
}

void KMeansQuantizer::prepareOutput(const Framebuffer* input)
{
    if (output_ == nullptr)
        output_ = Framebuffer::create(input->width(), input->height());
    else if
    (
        output_->width() != input->width() || 
        output_->height() != input->height() ||
        output_->colorBufferInternalFormat() != 
            input->colorBufferInternalFormat()
    )
    {
        delete output_;
        output_ = Framebuffer::create
        (
            input->width(), 
            input->height(),
            input->colorBufferInternalFormat()
        );
    }
    for (int i=0; i<2; i++)
    {
        if (output_->colorBufferWrapMode(i) != input->colorBufferWrapMode(i))
            output_->setColorBufferWrapMode(i, input->colorBufferWrapMode(i));
    }
    if (output_->colorBufferMagFilterMode()!=input->colorBufferMagFilterMode())
        output_->setColorBufferMagFilterMode(input->colorBufferMagFilterMode());
    if (output_->colorBufferMinFilterMode()!=input->colorBufferMinFilterMode())
        output_->setColorBufferMinFilterMode(input->colorBufferMinFilterMode());
}

void KMeansQuantizer::prepareOutput(const TextureBuffer2D* input)
{
    if (output_ == nullptr)
        output_ = Framebuffer::create(input->width(), input->height());
    else if
    (
        output_->width() != input->width() || 
        output_->height() != input->height() ||
        output_->colorBufferInternalFormat() != 
            input->internalFormat()
    )
    {
        delete output_;
        output_ = Framebuffer::create
        (
            input->width(), 
            input->height(),
            input->internalFormat()
        );
    }
}

}