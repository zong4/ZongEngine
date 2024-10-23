//
// Note:	this file is to be included in client applications ONLY
//			NEVER include this file anywhere in the engine codebase
//
#pragma once

#include "Engine/Core/Application.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/TimeStep.h"
#include "Engine/Core/Timer.h"
#include "Engine/Core/Platform.h"

#include "Engine/Core/Events/Event.h"
#include "Engine/Core/Events/ApplicationEvent.h"
#include "Engine/Core/Events/KeyEvent.h"
#include "Engine/Core/Events/MouseEvent.h"
#include "Engine/Core/Events/SceneEvents.h"

#include "Engine/Core/Math/AABB.h"
#include "Engine/Core/Math/Ray.h"

#include "imgui/imgui.h"

// --- Engine Render API ------------------------------
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/SceneRenderer.h"
#include "Engine/Renderer/RenderPass.h"
#include "Engine/Renderer/Framebuffer.h"
#include "Engine/Renderer/VertexBuffer.h"
#include "Engine/Renderer/IndexBuffer.h"
#include "Engine/Renderer/Pipeline.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Renderer/Camera.h"
#include "Engine/Renderer/Material.h"
// ---------------------------------------------------

// Scenes
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/SceneCamera.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Scene/Components.h"
