#include "pch.h"
#include "JoltCaptureManager.h"
#include "JoltScene.h"

#include "Engine/Utilities/FileSystem.h"
#include "Engine/Utilities/StringUtils.h"
#include "Engine/Utilities/ProcessHelper.h"

namespace Engine {

#ifndef ZONG_DIST
	void JoltCaptureOutStream::Open(const std::filesystem::path& inPath)
	{
		ZONG_CORE_VERIFY(!m_Stream.is_open());
		m_Stream.open(inPath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
	}

	void JoltCaptureOutStream::Close()
	{
		if (m_Stream.is_open())
			m_Stream.close();
	}

	void JoltCaptureOutStream::WriteBytes(const void* inData, size_t inNumBytes)
	{
		m_Stream.write((const char*)inData, inNumBytes);
	}

	bool JoltCaptureOutStream::IsFailed() const { return m_Stream.fail(); }

#endif

	JoltCaptureManager::JoltCaptureManager()
		: PhysicsCaptureManager()
	{
	}

#ifndef ZONG_DIST

	void JoltCaptureManager::BeginCapture()
	{
		if (IsCapturing())
			return;

		m_RecentCapture = m_CapturesDirectory / ("Capture-" + Utils::String::GetCurrentTimeString(true, true) + ".jor");
		m_Captures.push_back(m_RecentCapture);
		m_Stream.Open(m_RecentCapture);
		m_Recorder = std::make_unique<JPH::DebugRendererRecorder>(m_Stream);
	}

	void JoltCaptureManager::CaptureFrame()
	{
		if (!IsCapturing())
			return;

		JoltScene::GetJoltSystem().DrawBodies(m_DrawSettings, m_Recorder.get());
		m_Recorder->EndFrame();
	}

	void JoltCaptureManager::EndCapture()
	{
		if (!IsCapturing())
			return;

		m_Stream.Close();
		m_Recorder.reset();
	}

#endif

	void JoltCaptureManager::OpenCapture(const std::filesystem::path& capturePath) const
	{
		if (!FileSystem::Exists(capturePath))
			return;

		ProcessInfo joltViewerProcessInfo;
		joltViewerProcessInfo.FilePath = std::filesystem::path("Tools") / "JoltViewer" / "JoltViewer.exe";
		joltViewerProcessInfo.CommandLine = std::filesystem::absolute(capturePath).string();
		joltViewerProcessInfo.WorkingDirectory = "";
		joltViewerProcessInfo.Detached = true;
		ProcessHelper::CreateProcess(joltViewerProcessInfo);
	}

}

