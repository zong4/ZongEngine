#include "hzpch.h"
#include "Hazel/Core/Thread.h"

#include <pthread.h>

namespace Hazel {

	Thread::Thread(const std::string& name)
		: m_Name(name)
	{
	}

	void Thread::SetName(const std::string& name)
	{
		pthread_setname_np(m_Thread.native_handle(), name.c_str());
	}

	void Thread::Join()
	{
		m_Thread.join();
	}

	// TODO(Emily): `ThreadSignal`

	std::thread::id Thread::GetID() const
	{
		return m_Thread.get_id();
	}
}
