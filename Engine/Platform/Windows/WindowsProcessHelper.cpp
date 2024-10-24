#include "pch.h"
#include "Engine/Utilities/ProcessHelper.h"

// NOTE(Peter): codecvt has *technically* been deprecated, but the C++ committee has said it's still safe and portable to use until a suitable replacement has been found
#include <codecvt>

namespace Engine {

	static std::unordered_map<UUID, PROCESS_INFORMATION> s_WindowsProcessStorage;

	UUID ProcessHelper::CreateProcess(const ProcessInfo& inProcessInfo)
	{
		std::filesystem::path workingDirectory = inProcessInfo.WorkingDirectory.empty() ? inProcessInfo.FilePath.parent_path() : inProcessInfo.WorkingDirectory;

		std::wstring commandLine = inProcessInfo.IncludeFilePathInCommands ? inProcessInfo.FilePath.wstring() : L"";

		if (!inProcessInfo.CommandLine.empty())
		{
			if (commandLine.size() > 0)
			{
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstringConverter;
				commandLine += L" " + wstringConverter.from_bytes(inProcessInfo.CommandLine);
			}
			else
			{
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstringConverter;
				commandLine = wstringConverter.from_bytes(inProcessInfo.CommandLine);
			}
		}

		PROCESS_INFORMATION processInformation;
		ZeroMemory(&processInformation, sizeof(PROCESS_INFORMATION));

		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		startupInfo.cb = sizeof(STARTUPINFO);

		DWORD creationFlags = NORMAL_PRIORITY_CLASS;

		if (inProcessInfo.Detached)
			creationFlags |= DETACHED_PROCESS;

		BOOL success = ::CreateProcess(
			inProcessInfo.FilePath.c_str(), commandLine.data(),
			NULL, NULL, FALSE, creationFlags, NULL,
			workingDirectory.c_str(), &startupInfo, &processInformation);

		if (!success)
		{
			CloseHandle(processInformation.hThread);
			CloseHandle(processInformation.hProcess);
			return 0;
		}

		UUID processID = UUID();

		if (inProcessInfo.Detached)
		{
			CloseHandle(processInformation.hThread);
			CloseHandle(processInformation.hProcess);
		}
		else
		{
			s_WindowsProcessStorage[processID] = processInformation;
		}

		return processID;
	}

	void ProcessHelper::DestroyProcess(UUID inHandle, uint32_t inExitCode)
	{
		ZONG_CORE_VERIFY(s_WindowsProcessStorage.find(inHandle) != s_WindowsProcessStorage.end(), "Trying to destroy untracked process!");
		const auto& processInformation = s_WindowsProcessStorage[inHandle];
		TerminateProcess(processInformation.hProcess, inExitCode);
		CloseHandle(processInformation.hThread);
		CloseHandle(processInformation.hProcess);
		s_WindowsProcessStorage.erase(inHandle);
	}

}
