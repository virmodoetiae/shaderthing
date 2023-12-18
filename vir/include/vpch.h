// Std libraries -------------------------------------------------------------//

//#define _USE_MATH_DEFINES // Eh, maybe not
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <unordered_map>
#include <variant>
#include <vector>

// Third party libraries -----------------------------------------------------//

// GLAD provides a way to retrieve OS-specific OpenGL driver locations in a 
// simple way that is OS-independent from the perspective of the end user
#include "thirdparty/glad/include/glad/glad.h"   // <- must be included before GLFW if 
                                    // GLFW_INCLUDE_NONE is not defined
#include "thirdparty/glfw3/include/GLFW/glfw3.h"

// Image loading and manipulation for many formats
#include "thirdparty/stb/stb_image.h"

// GLM
#include "thirdparty/glm/ext.hpp"
#include "thirdparty/glm/glm.hpp"
#include "thirdparty/glm/gtc/matrix_transform.hpp"
#include "thirdparty/glm/gtc/type_ptr.hpp"
#include "thirdparty/glm/gtx/string_cast.hpp"
#include "thirdparty/glm/gtx/rotate_vector.hpp"

// My libraries --------------------------------------------------------------//

#include "veventsystem/vevent.h"
#include "veventsystem/vreceiver.h"

#include "vglobalptr.h"
#include "vtime/vtime.h"
#include "veventsystem/vbroadcaster.h"
#include "vwindow/vwindow.h"
#include "vinput/vinputstate.h"

#include "vgraphics/vcore/vgraphicscontext.h"
#include "vgraphics/vcore/vbuffers.h"
#include "vgraphics/vcore/vshader.h"
#include "vgraphics/vcore/vrenderer.h"
#include "vgraphics/vcore/vprimitives.h"

#include "vgraphics/vpostprocess/vquantizer.h"
#include "vgraphics/vmisc/vgifencoder.h"

#include "vimgui/vimguirenderer.h"
#include "vhelpers.h"
