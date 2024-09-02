#pragma once

#include "Ref.h"

#include <functional>
#include  <memory>

namespace Hazel {

	void InitializeCore();
	void ShutdownCore();

}

#if !defined(HZ_PLATFORM_WINDOWS) && !defined(HZ_PLATFORM_LINUX)
	#error Unknown platform.
#endif

#define BIT(x) (1u << x)

#if defined(__GNUC__)
	#if defined(__clang__)
		#define HZ_COMPILER_CLANG
	#else
		#define HZ_COMPILER_GCC
	#endif
#elif defined(_MSC_VER)
	#define HZ_COMPILER_MSVC
#endif

#define HZ_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#ifdef HZ_COMPILER_MSVC
	#define HZ_FORCE_INLINE __forceinline
	#define HZ_EXPLICIT_STATIC static
#elif defined(__GNUC__)
	#define HZ_FORCE_INLINE __attribute__((always_inline)) inline
	#define HZ_EXPLICIT_STATIC
#else
	#define HZ_FORCE_INLINE inline
	#define HZ_EXPLICIT_STATIC
#endif

namespace Hazel {

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
		HZ_FORCE_INLINE void SetDirty() { flag.clear(); }
		HZ_FORCE_INLINE bool CheckAndResetIfDirty() { return !flag.test_and_set(); }

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
		HZ_FORCE_INLINE void SetDirty() noexcept { flag = true; }
		HZ_FORCE_INLINE bool CheckAndResetIfDirty() noexcept
		{
			if (flag)
				return !(flag = !flag);
			else
				return false;
		}

		HZ_FORCE_INLINE bool IsDirty() const noexcept { return flag; }

	private:
		bool flag = false;
	};

}
