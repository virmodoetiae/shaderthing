#ifndef VIR_H
#define VIR_H

#include "thirdparty/glm/ext.hpp"
#include "thirdparty/glm/glm.hpp"
#include "thirdparty/glm/gtc/matrix_transform.hpp"
#include "thirdparty/glm/gtc/type_ptr.hpp"
#include "thirdparty/glm/gtx/string_cast.hpp"
#include "thirdparty/glm/gtx/rotate_vector.hpp"

#include "thirdparty/stb/stb_image.h"

#include "vconstants.h"
#include "vcamera/vcamera.h"
#include "vcamera/vinputcamera.h"
#include "veventsystem/vevent.h"
#include "veventsystem/vreceiver.h"
#include "veventsystem/vbroadcaster.h"
#include "vgraphics/vcore/vbuffers.h"
#include "vgraphics/vcore/vgraphicscontext.h"
#include "vgraphics/vcore/vshader.h"
#include "vgraphics/vcore/vprimitives.h"
#include "vgraphics/vcore/vrenderer.h"
#include "vgraphics/vpostprocess/vquantizer.h"
#include "vgraphics/vmisc/vgifencoder.h"
#include "vinput/vinputcodes.h"
#include "vinput/vinputstate.h"
#include "vtime/vtime.h"
#include "vwindow/vwindow.h"
#include "vconstants.h"
#include "vglfwwrapper.h"
#include "vglobalptr.h"
#include "vhelpers.h"
#include "vinitialization.h"
#include "vimgui/vimguirenderer.h"

#endif