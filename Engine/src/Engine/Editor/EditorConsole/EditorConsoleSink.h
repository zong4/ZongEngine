#pragma once

#include "Engine/Editor/EditorConsolePanel.h"

#include "spdlog/sinks/base_sink.h"
#include <mutex>

namespace Hazel {

	class EditorConsoleSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		explicit EditorConsoleSink(uint32_t bufferCapacity)
			: m_MessageBufferCapacity(bufferCapacity), m_MessageBuffer(bufferCapacity) {}

		virtual ~EditorConsoleSink() = default;

		EditorConsoleSink(const EditorConsoleSink& other) = delete;
		EditorConsoleSink& operator=(const EditorConsoleSink& other) = delete;

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
			std::string longMessage = fmt::to_string(formatted);
			std::string shortMessage = longMessage;

			if (shortMessage.length() > 100)
			{
				size_t spacePos = shortMessage.find_first_of(' ', 100);
				if (spacePos != std::string::npos)
					shortMessage.replace(spacePos, shortMessage.length() - 1, "...");
			}

			m_MessageBuffer[m_MessageCount++] = ConsoleMessage{ shortMessage, longMessage, GetMessageFlags(msg.level), std::chrono::system_clock::to_time_t(msg.time) };

			if (m_MessageCount == m_MessageBufferCapacity)
				flush_();
		}

		void flush_() override
		{
			for (const auto& message : m_MessageBuffer)
				EditorConsolePanel::PushMessage(message);

			m_MessageCount = 0;
		}

	private:
		static int16_t GetMessageFlags(spdlog::level::level_enum level)
		{
			int16_t flags = 0;

			switch (level)
			{
			case spdlog::level::trace:
			case spdlog::level::debug:
			case spdlog::level::info:
			{
				flags |= (int16_t)ConsoleMessageFlags::Info;
				break;
			}
			case spdlog::level::warn:
			{
				flags |= (int16_t)ConsoleMessageFlags::Warning;
				break;
			}
			case spdlog::level::err:
			case spdlog::level::critical:
			{
				flags |= (int16_t)ConsoleMessageFlags::Error;
				break;
			}
			}

			return flags;
		}

	private:
		uint32_t m_MessageBufferCapacity;
		std::vector<ConsoleMessage> m_MessageBuffer;
		uint32_t m_MessageCount = 0;
	};

}
