#include "vpch.h"
#include "vgraphics/vpostprocess/vpostprocess.h"
#include "vgraphics/vcore/vbuffers.h"

namespace vir
{

std::unordered_map<PostProcess::Type, std::string> PostProcess::typeToName = 
{
    {PostProcess::Type::Bloom, "Bloom"},
    {PostProcess::Type::Quantization, "Quantization"}
};

PostProcess::PostProcess(Type type) :
type_(type),
output_(nullptr),
canRunOnDeviceInUse_(true),
errorMessage_()
{}

PostProcess::~PostProcess()
{
    if (output_ != nullptr)
        delete output_;
    output_ = nullptr;
}

void PostProcess::prepareOutput(const Framebuffer* input)
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

void PostProcess::prepareOutput(const TextureBuffer2D* input)
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
    for (int i=0; i<2; i++)
    {
        if (output_->colorBufferWrapMode(i) != input->wrapMode(i))
            output_->setColorBufferWrapMode(i, input->wrapMode(i));
    }
    if (output_->colorBufferMagFilterMode()!=input->magFilterMode())
        output_->setColorBufferMagFilterMode(input->magFilterMode());
    if (output_->colorBufferMinFilterMode()!=input->minFilterMode())
        output_->setColorBufferMinFilterMode(input->minFilterMode());
}

}