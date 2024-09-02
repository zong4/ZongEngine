#pragma once

#include "Hazel/Project/Project.h"

namespace Hazel {

	class ScriptBuilder
	{
	public:
		static void BuildCSProject(const std::filesystem::path& filepath);
		static void BuildScriptAssembly(Ref<Project> project);
	};

}
