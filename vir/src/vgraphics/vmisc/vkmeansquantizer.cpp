#include "vpch.h"
#include "vgraphics/vmisc/vkmeansquantizer.h"
#include "vgraphics/vmisc/vopengl/vopenglkmeansquantizer.h"

namespace vir
{

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