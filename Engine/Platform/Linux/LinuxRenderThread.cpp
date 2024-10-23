#include "pch.h"
#include "Hazel/Core/RenderThread.h"

#include "Hazel/Renderer/Renderer.h"

#include <condition_variable>
#include <mutex>

namespace Hazel {

	struct RenderThreadData
	{
		std::mutex m_CriticalSection;
		std::condition_variable m_ConditionVariable;

		RenderThread::State m_State = RenderThread::State::Idle;
	};

	RenderThread::RenderThread(ThreadingPolicy coreThreadingPolicy)
		: m_RenderThread("Render Thread"), m_ThreadingPolicy(coreThreadingPolicy)
	{
		m_Data = new RenderThreadData();
	}

	void RenderThread::Run()
	{
		m_IsRunning = true;
		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
			m_RenderThread.Dispatch(Renderer::RenderThreadFunc, this);
	}

	void RenderThread::Terminate()
	{
		m_IsRunning = false;
		Pump();

		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
			m_RenderThread.Join();

		delete m_Data;
	}

	void RenderThread::Wait(State waitForState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		std::unique_lock lock(m_Data->m_CriticalSection);
		while (m_Data->m_State != waitForState)
		{
			m_Data->m_ConditionVariable.wait(lock);
		}
	}

	void RenderThread::WaitAndSet(State waitForState, State setToState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		std::unique_lock lock(m_Data->m_CriticalSection);
		while (m_Data->m_State != waitForState)
		{
			m_Data->m_ConditionVariable.wait(lock);
		}
		m_Data->m_State = setToState;
		m_Data->m_ConditionVariable.notify_one();
	}

	void RenderThread::Set(State setToState)
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		std::unique_lock lock(m_Data->m_CriticalSection);
		m_Data->m_State = setToState;
		m_Data->m_ConditionVariable.notify_one();
	}

	void RenderThread::NextFrame()
	{
		m_AppThreadFrame++;
		Renderer::SwapQueues();
	}

	void RenderThread::BlockUntilRenderComplete()
	{
		if (m_ThreadingPolicy == ThreadingPolicy::SingleThreaded)
			return;

		Wait(State::Idle);
	}

	void RenderThread::Kick()
	{
		if (m_ThreadingPolicy == ThreadingPolicy::MultiThreaded)
		{
			Set(State::Kick);
		}
		else
		{
			Renderer::WaitAndRender(this);
		}
	}

	void RenderThread::Pump()
	{
		NextFrame();
		Kick();
		BlockUntilRenderComplete();
	}

}
