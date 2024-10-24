#pragma once

#include "Engine/Project/Project.h"

namespace Engine {

	class ScriptBuilder
	{
	public:
		static void BuildCSProject(const std::filesystem::path& filepath);
		static void BuildScriptAssembly(Ref<Project> project);
	};

}
