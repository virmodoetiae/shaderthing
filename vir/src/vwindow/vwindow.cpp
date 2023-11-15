#include "vpch.h"

namespace vir
{

Window::Window(uint32_t w, uint32_t h, std::string t, bool r) :
title_(t),
width_(w),
height_(h),
viewportWidth_(w),
viewportHeight_(w),
aspectRatio_(float(w)/float(h)),
resizable_(r)
{
    // Set a very high priority for the window
    this->receiverPriority() = 1389;
}

Window::~Window()
{
    delete context_;
    context_ = nullptr;
}

}