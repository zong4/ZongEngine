//
// Note:	this file is to be included in client applications ONLY
//			NEVER include this file anywhere in the engine codebase
//
#pragma once

#include "Hazel/Core/Application.h"
#include "Hazel/Core/Log.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Core/TimeStep.h"
#include "Hazel/Core/Timer.h"
#include "Hazel/Core/Platform.h"

#include "Hazel/Core/Events/Event.h"
#include "Hazel/Core/Events/ApplicationEvent.h"
#include "Hazel/Core/Events/KeyEvent.h"
#include "Hazel/Core/Events/MouseEvent.h"
#include "Hazel/Core/Events/SceneEvents.h"

#include "Hazel/Core/Math/AABB.h"
#include "Hazel/Core/Math/Ray.h"

#include "imgui/imgui.h"

// --- Hazel Render API ------------------------------
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/Framebuffer.h"
#include "Hazel/Renderer/VertexBuffer.h"
#include "Hazel/Renderer/IndexBuffer.h"
#include "Hazel/Renderer/Pipeline.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/Material.h"
// ---------------------------------------------------

// Scenes
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/SceneCamera.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Scene/Components.h"
