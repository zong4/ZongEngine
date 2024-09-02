#include "hzpch.h"
#include "Hazel/Script/ScriptBuilder.h"

#include <ShlObj.h>

#include <format>

namespace Hazel {

	void ScriptBuilder::BuildCSProject(const std::filesystem::path& filepath)
	{
		TCHAR programFilesFilePath[MAX_PATH];
		SHGetSpecialFolderPath(0, programFilesFilePath, CSIDL_PROGRAM_FILES, FALSE);
		std::filesystem::path msBuildPath = std::filesystem::path(programFilesFilePath) / "Microsoft Visual Studio" / "2022" / "Community" / "Msbuild" / "Current" / "Bin" / "MSBuild.exe";
		std::string command = std::format("cd \"{}\" && \"{}\" \"{}\" -property:Configuration=Debug -t:restore,build", filepath.parent_path().string(), msBuildPath.string(), filepath.filename().string());
		system(command.c_str());
	}

	void ScriptBuilder::BuildScriptAssembly(Ref<Project> project)
	{
		BuildCSProject(project->GetScriptProjectPath());
	}

}
