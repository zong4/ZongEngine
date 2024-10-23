#include "hzpch.h"
#include "Engine/Script/ScriptBuilder.h"

#include <ShlObj.h>

#include <spdlog/fmt/fmt.h>

namespace Hazel {

	void ScriptBuilder::BuildCSProject(const std::filesystem::path& filepath)
	{
		TCHAR programFilesFilePath[MAX_PATH];
		SHGetSpecialFolderPath(0, programFilesFilePath, CSIDL_PROGRAM_FILES, FALSE);
		std::filesystem::path msBuildPath = std::filesystem::path(programFilesFilePath) / "Microsoft Visual Studio" / "2022" / "Community" / "Msbuild" / "Current" / "Bin" / "MSBuild.exe";

		std::string command = fmt::format("cd \"{}\" && \"{}\" \"{}\" -property:Configuration=Debug", filepath.parent_path().string(), msBuildPath.string(), filepath.filename());
		system(command.c_str());
	}

	void ScriptBuilder::BuildScriptAssembly(Ref<Project> project)
	{
		auto projectAssemblyFile = std::filesystem::absolute(Project::GetActive()->GetAssetDirectory() / "Scripts" / (Project::GetProjectName() + ".csproj"));
		BuildCSProject(projectAssemblyFile);
	}

}
