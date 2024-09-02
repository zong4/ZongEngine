#pragma once

#include "Hazel/Core/Ref.h"

namespace Hazel {

	class PhysicsCaptureManager : public RefCounted
	{
	public:
		virtual void BeginCapture() = 0;
		virtual void CaptureFrame() = 0;
		virtual void EndCapture() = 0;
		virtual bool IsCapturing() const = 0;
		virtual void OpenCapture(const std::filesystem::path& capturePath) const = 0;

		void OpenRecentCapture() const { OpenCapture(m_RecentCapture); }
		void ClearCaptures();
		void RemoveCapture(const std::filesystem::path& capturePath);
		const std::vector<std::filesystem::path>& GetAllCaptures() const { return m_Captures; }

	protected:
		PhysicsCaptureManager();

	protected:
		std::filesystem::path m_CapturesDirectory = "";
		std::filesystem::path m_RecentCapture = "";
		std::vector<std::filesystem::path> m_Captures;

	};

}
