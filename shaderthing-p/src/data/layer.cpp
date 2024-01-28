#include "shaderthing-p/include/data/layer.h"

namespace ShaderThing
{

bool Layer::GUI::errorsInSharedSource = false;

ImGuiExtd::TextEditor Layer::GUI::sharedSourceEditor = ImGuiExtd::TextEditor();

}