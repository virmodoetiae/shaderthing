#include "data/about.h"
#include "data/data.h"
#include "resources/resource.h"
#include "thirdparty/stb/stb_image.h"

namespace ShaderThing
{

About::About():
isGuiOpen_(false),
isGuiInMenu_(true),
virmodoetiaeImage_(nullptr)
{
    virmodoetiaeImage_ = new Resource();
    if 
    (
        virmodoetiaeImage_->set
        (
            ImageData::virmodoetiae0Data, 
            ImageData::virmodoetiae0Size
        )
    ) return;
    delete virmodoetiaeImage_;
    virmodoetiaeImage_ = nullptr;

}

About::~About()
{
    if (virmodoetiaeImage_ == nullptr)
        return;
    delete virmodoetiaeImage_;
    virmodoetiaeImage_ = nullptr;
}

}