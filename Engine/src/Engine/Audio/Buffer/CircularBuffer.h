#pragma once

#include <array>

namespace Hazel::Audio
{
	// TODO: typename NumChannels and handle push/pop by multiple values at the same time, or keep counters for a single channel, but increment read position by NumChannels
	template<typename T, size_t size>
	class CircularBuffer final
	{
	public:
		CircularBuffer() = default;

		inline void Clear() noexcept
		{
			m_avail = 0;
			m_readpos = 0;
			m_writepos = 0;
			std::fill(m_buf.begin(), m_buf.end(), T(0));
		}

		inline void Push(T x) noexcept
		{
			m_buf[m_writepos] = x;
			++m_writepos;
			++m_avail;
			if (m_writepos >= m_buf.size()) // TODO: if m_writepos >= m_readpos -> m_readpos = m_writepos + 1 % m_buf.size()
				m_writepos = 0;
		}

		inline T Get() noexcept
		{
			ZONG_CORE_ASSERT(m_avail > 0);
			T x = m_buf[m_readpos];
			++m_readpos;
			--m_avail;
			if (m_readpos >= m_buf.size())
				m_readpos = 0;
			return x;
		}

		inline int Available() const noexcept { return m_avail; }

		inline int GetMultiple(T* buf, int len) noexcept
		{
			ZONG_CORE_ASSERT(m_avail > 0);
			if (len > m_avail)
				len = m_avail;
			for (int i = 0; i < len; ++i)
				buf[i] = Get();
			return len;
		}

		inline void PushMultiple(const T* buf, int len) noexcept
		{
			const size_t free = m_buf.size() - m_writepos;
			const int overflaw = len - (int)free;

			if (overflaw > 0)
			{
				std::memcpy(&m_buf[m_writepos], buf, free * sizeof(T));
				std::memcpy(&m_buf[0], buf + free, overflaw * sizeof(T));

				m_writepos = overflaw;
			}
			else
			{
				std::memcpy(&m_buf[m_writepos], buf, len * sizeof(T));

				m_writepos += len;
				if (m_writepos >= m_buf.size()) // m_writepos > m_buf.size() technically is not possible
					m_writepos = 0;
			}

			m_avail += len;

			ZONG_CORE_ASSERT(m_avail < m_buf.size());
		}

		// Fast version without checking how many available
		template<unsigned int count>
		void GetMultiple(T* dest)
		{
			ZONG_CORE_ASSERT(m_avail >= count);

			std::memcpy(dest, &m_buf[m_readpos], count * sizeof(T));

			m_readpos += count;
			m_avail -= count;

			if (m_readpos >= m_buf.size())
				m_readpos = m_readpos - (int)m_buf.size();
		}

		constexpr size_t GetSize() const noexcept { return m_buf.size(); }

	private:
		int m_writepos = 0;
		int m_readpos = 0;
		int m_avail = 0;
		std::array<T, size> m_buf{ T(0) };
	};

} // namespace Hazel::Audio
