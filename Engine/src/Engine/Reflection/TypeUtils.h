#pragma once

#include <type_traits>
#include <vector>
#include <array>

namespace Engine::Type {
	//==============================================================================
	namespace member_pointer {
		namespace impl {
			template <typename T>
			struct return_type_function;

			template <typename Object, typename Return, typename... Args>
			struct return_type_function<Return(Object::*)(Args...)> { using type = Return; };

			template <typename Object, typename Return, typename... Args>
			struct return_type_function<Return(Object::*)(Args...) const> { using type = Return; };

			template <typename T>
			struct return_type_object;

			template <typename Object, typename Return>
			struct return_type_object<Return Object::*> { using type = Return; };
		}

		template<typename T>
		struct return_type;

		template<typename T>
		struct return_type : std::enable_if_t<std::is_member_pointer_v<T>,
			std::conditional_t<std::is_member_object_pointer_v<T>,
								impl::return_type_object<T>,
								impl::return_type_function<T>>>
		{
		};
	}

	//==============================================================================
	template <typename, typename = void>
	struct is_specialized : std::false_type {};

	template<typename T>
	struct is_specialized < T, std::void_t<decltype(T{}) >> : std::true_type {};

	//==============================================================================
	namespace is_array_impl {
		template <typename T>					struct is_array_impl : std::false_type {};
		template <typename T, std::size_t N>	struct is_array_impl<std::array<T, N>> : std::true_type {};
		template <typename... Args>				struct is_array_impl<std::vector<Args...>> : std::true_type {};
	}

	template<typename T>
	struct is_array
	{
		static constexpr bool const value = is_array_impl::is_array_impl<std::decay_t<T>>::value;
	};

	template<typename T>
	inline constexpr bool is_array_v = is_array<T>::value;

	//==============================================================================
	// A helper trait to check if the type supports streaming (https://stackoverflow.com/a/66397194)
	template<class T>
	class is_streamable
	{
		// match if streaming is supported
		template<class TT>
		static constexpr auto test(int) -> decltype(std::declval<std::ostream&>() << std::declval<TT>(), std::true_type());

		// match if streaming is not supported:
		template<class>
		static constexpr auto test(...)->std::false_type;

	public:
		// check return value from the matching "test" overload:
		static constexpr bool value = decltype(test<T>(0))::value;
	};

	template<class T>
	inline constexpr bool is_streamable_v = is_streamable<T>::value;

	//==============================================================================
	struct filter_void_alt {};

	template<class T, class Alternative = filter_void_alt>
	struct filter_void
	{
		using type = std::conditional_t<std::is_void_v<T>, Alternative, T>;
	};

	template<class T>
	using filter_void_t = typename filter_void<T>::type;

	//==============================================================================
#if __cplusplus >= 202002L
	template <typename TupleT, typename Fn>
	constexpr void for_each_tuple(TupleT&& tp, Fn&& fn)
	{
		std::apply(
			[&fn]<typename ...T>(T&& ...args)
			{
				(fn(std::forward<T>(args)), ...);
			},
			std::forward<TupleT>(tp)
		);
	}
#else
	template <typename TupleT, typename Fn>
	constexpr void for_each_tuple(TupleT&& tp, Fn&& fn)
	{
		std::apply(
			[&fn](auto&& ...args)
			{
				(fn(std::forward<decltype(args)>(args)), ...);
			},
			std::forward<TupleT>(tp)
		);
	}
#endif

	//==============================================================================
#if __cplusplus >= 202002L
		// C++20
		// Call 'expr' multiple time, efficient.
		template<auto N>
		static constexpr auto unroll = [](auto expr)
		{
			[expr] <auto...Is>(std::index_sequence<Is...>)
			{
				((expr(), void(Is)), ...);
			}(std::make_index_sequence<N>());
		};

		template<auto N, typename...Args>
		constexpr auto nth_element(Args... args)
		{
			return[&]<size_t...Ns>(std::index_sequence<Ns...>)
			{
				return [](decltype((void*)Ns)..., auto* nth, auto*...)
				{
					return *nth;
				}(&args...);
			}(std::make_index_sequence<N>());
		}

		template<auto N>
		constexpr auto nth_element_l = [](auto... args)
		{
			return[&]<std::size_t... Ns>(std::index_sequence<Ns...>)
			{
				return [](decltype((void*)Ns)..., auto* nth, auto*...)
				{
					return *nth;
				}(&args...);
			}
			(std::make_index_sequence<N>{});
		};
#else
		namespace impl {
			template<size_t...Ns, typename ...Args>
			constexpr auto nth_element_unroll(std::index_sequence<Ns...>, Args...args)
			{
				return [](decltype((void*)Ns)..., auto* nth, auto*...)
				{
					return *nth;
				}(&args...);
			}
		}

		template<auto N, typename...Args>
		constexpr auto nth_element(Args... args)
		{
			return impl::nth_element_unroll(std::make_index_sequence<N>(), args...);
		}
#endif

} // namespace Engine::Type
