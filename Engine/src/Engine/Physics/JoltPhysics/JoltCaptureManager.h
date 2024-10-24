#pragma once

#include "Engine/Physics/PhysicsCaptureManager.h"

#ifndef ZONG_DIST
	#include <Jolt/Core/StreamOut.h>
	#include <Jolt/Renderer/DebugRendererRecorder.h>
	#include <Jolt/Physics/Body/BodyManager.h>
#endif

namespace Engine {

#ifndef ZONG_DIST
	class JoltCaptureOutStream : public JPH::StreamOut
	{
	public:
		void Open(const std::filesystem::path& inPath);
		void Close();
		bool IsOpen() const { return m_Stream.is_open(); }

		virtual void WriteBytes(const void* inData, size_t inNumBytes) override;
		virtual bool IsFailed() const override;

	private:
		std::ofstream m_Stream;
	};

	class JoltCaptureManager : public PhysicsCaptureManager
	{
	public:
		JoltCaptureManager();

		virtual void BeginCapture() override;
		virtual void CaptureFrame() override;
		virtual void EndCapture() override;
		virtual bool IsCapturing() const override { return m_Stream.IsOpen() && m_Recorder != nullptr; }
		virtual void OpenCapture(const std::filesystem::path& capturePath) const override;

	private:
		JoltCaptureOutStream m_Stream;
		std::unique_ptr<JPH::DebugRendererRecorder> m_Recorder = nullptr;
		JPH::BodyManager::DrawSettings m_DrawSettings;
	};

#else

	class JoltCaptureManager : public PhysicsCaptureManager
	{
	public:
		JoltCaptureManager();

		virtual void BeginCapture() override {}
		virtual void CaptureFrame() override {}
		virtual void EndCapture() override {}
		virtual bool IsCapturing() const override { return false; }
		virtual void OpenCapture(const std::filesystem::path& capturePath) const override;
	};

#endif
}
