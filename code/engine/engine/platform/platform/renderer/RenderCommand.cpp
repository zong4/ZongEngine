#include "RenderCommand.h"

std::unique_ptr<zong::platform::RendererAPI> zong::platform::RenderCommand::s_RendererAPI = RendererAPI::create();
