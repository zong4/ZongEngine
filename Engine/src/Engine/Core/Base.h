#pragma once

#include <memory>
#include "Ref.h"

namespace Engine {

	void InitializeCore();
	void ShutdownCore();

}

#if !defined(ZONG_PLATFORM_WINDOWS) && !defined(ZONG_PLATFORM_LINUX)
	#error Unknown platform.
#endif

#define BIT(x) (1u << x)

#if defined(__clang__)
	#define ZONG_COMPILER_CLANG
#elif defined(_MSC_VER)
	#define ZONG_COMPILER_MSVC
#endif

#define ZONG_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#ifdef ZONG_COMPILER_MSVC
	#define ZONG_FORCE_INLINE __forceinline
#elif defined(ZONG_COMPILER_CLANG)
	#define ZONG_FORCE_INLINE __attribute__((always_inline)) inline
#else
	#define ZONG_FORCE_INLINE inline
#endif

#include "Assert.h"

namespace Engine {

	// Pointer wrappers
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	using byte = uint8_t;

	/** A simple wrapper for std::atomic_flag to avoid confusing
		function names usage. The object owning it can still be
		default copyable, but the copied flag is going to be reset.
	*/
	struct AtomicFlag
	{
		ZONG_FORCE_INLINE void SetDirty() { flag.clear(); }
		ZONG_FORCE_INLINE bool CheckAndResetIfDirty() { return !flag.test_and_set(); }

		explicit AtomicFlag() noexcept { flag.test_and_set(); }
		AtomicFlag(const AtomicFlag&) noexcept {}
		AtomicFlag& operator=(const AtomicFlag&) noexcept { return *this; }
		AtomicFlag(AtomicFlag&&) noexcept {};
		AtomicFlag& operator=(AtomicFlag&&) noexcept { return *this; }

	private:
		std::atomic_flag flag;
	};

	struct Flag
	{
		ZONG_FORCE_INLINE void SetDirty() noexcept { flag = true; }
		ZONG_FORCE_INLINE bool CheckAndResetIfDirty() noexcept
		{
			if (flag)
				return !(flag = !flag);
			else
				return false;
		}

		ZONG_FORCE_INLINE bool IsDirty() const noexcept { return flag; }

	private:
		bool flag = false;
	};

}
