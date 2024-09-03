#pragma once

#include <type_traits>

//==============================================================================
/*	
	A bunch of useful meta-programming utilities, directly taken from <variant>
	header in case they are removed in the future.
*/
namespace meta {
	//==============================================================================
	template <class...>
	struct All_same : std::true_type {};
	// variadic is_same
	template <class First, class... Rest>
	struct All_same<First, Rest...> : std::bool_constant<std::conjunction_v<std::is_same<First, Rest>...>> {};

	// variadic is_convertible
	template <class To, class... From>
	struct Convertible_from_all : std::bool_constant<std::conjunction_v<std::is_convertible<From, To>...>>
	{
	};

	//==============================================================================
	// A sequence of types
	template <class...>
	struct Meta_list {}; 
	struct Meta_nil {};

	template <auto...i>
	struct Meta_list_i {};

	template <class List>
	struct Meta_front_;
	// Extract the first type in a sequence (head of list)
	template <class List>
	using Meta_front =
		typename Meta_front_<List>::type;

	template <template <class...> class List, class First, class... Rest>
	struct Meta_front_<List<First, Rest...>>
	{
		using type = First;
	};

	template <class List>
	struct Meta_pop_front_;
	// Subsequence including all but the first type (tail of list)
	template <class List>
	using Meta_pop_front =
		typename Meta_pop_front_<List>::type;

	template <template <class...> class List, class First, class... Rest>
	struct Meta_pop_front_<List<First, Rest...>>
	{
		using type = List<Rest...>;
	};

	template <class List, class Ty>
	struct Meta_push_front_;
	// Prepend a new type onto a sequence
	template <class List, class Ty>
	using Meta_push_front =
		typename Meta_push_front_<List, Ty>::type;

	template <template <class...> class List, class... Types, class Ty>
	struct Meta_push_front_<List<Types...>, Ty>
	{
		using type = List<Ty, Types...>;
	};

	//==============================================================================
	template <class Void, template <class...> class Fn, class... Args>
	struct Meta_quote_helper_;
	template <template <class...> class Fn, class... Args>
	struct Meta_quote_helper_<std::void_t<Fn<Args...>>, Fn, Args...>
	{
		using type = Fn<Args...>;
	};

	/*	Encapsulate a template into a meta-callable type.

		using TTest = Meta_quote<Meta_list>::_Invoke<char, unsigned int>;
		static_assert(std::is_same_v<TTest, Meta_list<char, unsigned int>>);
	*/
	template <template <class...> class Fn>
	struct Meta_quote
	{
		template <class... Types>
		using Invoke = typename Meta_quote_helper_<void, Fn, Types...>::type;
	};

	//==============================================================================
	template <class Void, template <auto...> class Fn, auto...Args>
	struct Meta_quote_helper_i_;
	template <template <auto...> class Fn, auto...Args>
	struct Meta_quote_helper_i_<std::void_t<Fn<Args...>>, Fn, Args...>
	{
		using type = Fn<Args...>;
	};

	/*	Encapsulate a template into a meta-callable type.

		using TTest = Meta_quote<Meta_list>::_Invoke<char, unsigned int>;
		static_assert(std::is_same_v<TTest, Meta_list<char, unsigned int>>);
	*/
	template <template <auto...> class Fn>
	struct Meta_quote_i
	{
		template <auto... Types>
		using Invoke = typename Meta_quote_helper_i_<void, Fn, Types...>::type;
	};
	
	//==============================================================================
	/*	Invoke meta-callable Fn with Args.

		using TTest = Meta_invoke<Meta_quote<Meta_list>, char, unsigned int>;
		static_assert(std::is_same_v<TTest, Meta_list<char, unsigned int>>);
	*/
	template <class Fn, class... Args>
	using Meta_invoke = 
		typename Fn::template Invoke<Args...>;

	template <class Fn, auto... Args>
	using Meta_invoke_i =
		typename Fn::template Invoke<Args...>;

	//==============================================================================
	/* Construct a meta-callable that passes its arguments and Args to Fn.

		using TTest = Meta_bind_back<Meta_quote<Meta_list>, char, unsigned int>::_Invoke<bool>;
		static_assert(std::is_same_v<TTest, Meta_list<bool, char, unsigned int>>);
	*/
	template <class Fn, class... Args>
	struct Meta_bind_back
	{
		template <class... Types>
		using Invoke = Meta_invoke<Fn, Types..., Args...>;
	};


	//==============================================================================
	template <class Fn, class List>
	struct Meta_apply_;

	/*	Unpack List into the parameters of meta-callable Fn
		
		using TTest = Meta_apply<Meta_quote<Meta_list>, Meta_list<char, unsigned int>>;
		static_assert(std::is_same_v<TTest, Meta_list<char, unsigned int>>);
		
	*/
	template <class Fn, class List>
	using Meta_apply =
		typename Meta_apply_<Fn, List>::type;

	// Invoke meta-callable Fn with the parameters of a template specialization
	template <class Fn, template <class...> class List, class... Types>
	struct Meta_apply_<Fn,
		List<Types...>> { 
		using type = Meta_invoke<Fn, Types...>;
	};

	// Invoke meta-callable Fn with the elements of an integer_sequence
	template <class Fn, class Ty, Ty... Idxs>
	struct Meta_apply_<Fn,
		std::integer_sequence<Ty, Idxs...>> {
		using type = Meta_invoke<Fn, std::integral_constant<Ty, Idxs>...>;
	};

	//==============================================================================
	template <class Fn, class List>
	struct Meta_transform_;

	/*	Transform sequence of Types... into sequence of Fn<Types...>
		
		using TTest = Meta_transform<Meta_quote<Meta_list>, Meta_list<char, unsigned int>>;
		static_assert(std::is_same_v<TTest, Meta_list<Meta_list<char>, Meta_list<unsigned int>>>);
	*/
	template <class Fn, class List>
	using Meta_transform =
		typename Meta_transform_<Fn, List>::type;

	template <template <class...> class List, class Fn, class... Types>
	struct Meta_transform_<Fn, List<Types...>>
	{
		using type = List<Meta_invoke<Fn, Types>...>;
	};

	//==============================================================================
	template <class, class, template <class...> class>
	struct Meta_repeat_n_c_;

	/*	Construct a sequence consisting of repetitions of Ty

		using TTest = Meta_repeat_n_c<3, float, Meta_list>;
		static_assert(std::is_same_v<TTest, Meta_list<float, float, float>>);
	*/
	template <size_t Count, class Ty, template <class...> class Continue>
	using Meta_repeat_n_c =
		typename Meta_repeat_n_c_<Ty, std::make_index_sequence<Count>, Continue>::type;

	template <class Ty, size_t>
	using Meta_repeat_first_helper = Ty;

	template <class Ty, size_t... Idxs, template <class...> class Continue>
	struct Meta_repeat_n_c_<Ty, std::index_sequence<Idxs...>, Continue>
	{
		using type = Continue<Meta_repeat_first_helper<Ty, Idxs>...>;
	};

	//==============================================================================
	template <class List, size_t Idx, class = void>
	struct Meta_at_;

	/*	Extract the Idx-th type from List

		using TTest = Meta_at_c<Meta_list<bool, char, unsigned int>, 1>;
		static_assert(std::is_same_v<TTest, char>);
	*/	
#ifdef __clang__
	template <template <class...> class List, class... Types, size_t Idx>
	struct Meta_at_<List<Types...>, Idx, std::void_t<__type_pack_element<Idx, Types...>>>
	{
		using type = __type_pack_element<Idx, Types...>;
	};
#else // ^^^ __clang__ / !__clang__ vvv
	template <class... VoidPtrs>
	struct Meta_at_impl
	{
		template <class Ty, class... Types>
		static Ty Eval(VoidPtrs..., Ty*, Types*...); // undefined
	};

	/// JP. This is taken from <type_traits> header
	//----------------------------------------------------------------------------
	template <class Ty>
	struct Identity
	{
		using type = Ty;
	};
	template <class Ty>
	using Identity_t _MSVC_KNOWN_SEMANTICS = typename Identity<Ty>::type;
	//----------------------------------------------------------------------------

	template <class Ty>
	constexpr Identity<Ty>* Type_as_pointer()
	{
		return nullptr;
	}

	template <template <class...> class List, class... Types, size_t Idx>
	struct Meta_at_<List<Types...>, Idx, std::enable_if_t<(Idx < sizeof...(Types))>> {
		using type =
			typename decltype(Meta_repeat_n_c<Idx, void*, Meta_at_impl>::Eval(Type_as_pointer<Types>()...))::type;
	};
#endif // _clang__

	//==============================================================================
	inline constexpr auto Meta_npos = ~size_t{ 0 };

	template <class List, class Ty>
	struct Meta_find_index_
	{
		using type = std::integral_constant<size_t, Meta_npos>;
	};

	/*	Find the index of the first occurrence of Ty in List
		
		using TTest = Meta_find_index<Meta_list<bool, char, unsigned int>, unsigned int>;
		static_assert(std::is_same_v<TTest, std::integral_constant<size_t, 2>>);
		static_assert(TTest::value != Meta_npos);
	*/
	template <class List, class Ty>
	using Meta_find_index = 
		typename Meta_find_index_<List, Ty>::type;

	constexpr size_t Meta_find_index_i_(const bool* const Ptr, const size_t Count,
		size_t Idx = 0)
	{ // return the index of the first true in the Count bools at Ptr, or Meta_npos if all are false
		for (; Idx < Count; ++Idx)
		{
			if (Ptr[Idx])
			{
				return Idx;
			}
		}

		return Meta_npos;
	}

	template <template <class...> class List, class First, class... Rest, class Ty>
	struct Meta_find_index_<List<First, Rest...>, Ty>
	{
		static constexpr bool Bools[] = { std::is_same_v<First, Ty>, std::is_same_v<Rest, Ty>... };
		using type = std::integral_constant<size_t, Meta_find_index_i_(Bools, 1 + sizeof...(Rest))>;
	};

	//==============================================================================
	template <class List, class Ty>
	struct Meta_find_unique_index_
	{
		using type = std::integral_constant<size_t, Meta_npos>;
	};

	/*	The index of Ty in List if it occurs exactly once, otherwise Meta_npos

		using TTest = Meta_find_unique_index<Meta_list<bool, char, char>, char>;
		static_assert(TTest::value == Meta_npos);

		using TTest2 = Meta_find_unique_index<Meta_list<bool, char, char>, bool>;
		static_assert(TTest2::value == 0);
	*/
	template <class List, class Ty>
	using Meta_find_unique_index =
		typename Meta_find_unique_index_<List, Ty>::type;

	constexpr size_t Meta_find_unique_index_i_2(const bool* const Ptr, const size_t Count,
		const size_t
			First)
	{ // return First if there is no First < j < Count such that Ptr[j] is true, otherwise Meta_npos
		return First != Meta_npos && Meta_find_index_i_(Ptr, Count, First + 1) == Meta_npos ? First : Meta_npos;
	}

	constexpr size_t Meta_find_unique_index_i_(const bool* const Ptr,
		const size_t Count)
	{ // Pass the smallest i such that Ptr[i] is true to Meta_find_unique_index_i_2
		return Meta_find_unique_index_i_2(Ptr, Count, Meta_find_index_i_(Ptr, Count));
	}

	template <template <class...> class List, class First, class... Rest, class Ty>
	struct Meta_find_unique_index_<List<First, Rest...>, Ty>
	{
		using type = std::integral_constant<size_t,
			Meta_find_unique_index_i_(Meta_find_index_<List<First, Rest...>, Ty>::Bools, 1 + sizeof...(Rest))>;
	};

	//==============================================================================
	template <class>
	struct Meta_as_list_;
	
	/*	Convert Ty to a Meta_list

		using TTest = Meta_as_list<std::tuple<bool, char, unsigned int>>;
		static_assert(std::is_same_v<TTest, Meta_list<bool, char, unsigned int>>);
	*/
	template <class Ty>
	using Meta_as_list =
		typename Meta_as_list_<Ty>::type;

	template <template <class...> class List, class... Types>
	struct Meta_as_list_<List<Types...>>
	{ // convert the parameters of an arbitrary template specialization to a
	 // Meta_list of types
		using type = Meta_list<Types...>;
	};

	template <class Ty, Ty... Idxs>
	struct Meta_as_list_<std::integer_sequence<Ty, Idxs...>>
	{ // convert an integer_sequence to a Meta_list of
	 // integral_constants
		using type = Meta_list<std::integral_constant<Ty, Idxs>...>;
	};

	//==============================================================================
	template <class List>
	struct Meta_as_integer_sequence_;

	/*	Convert a list of integral_constants to an integer_sequence.

		using TTest = Meta_as_list<std::make_index_sequence<3>>;
		static_assert(std::is_same_v < TTest, Meta_list<std::integral_constant<size_t, 0>, std::integral_constant<size_t, 1>, std::integral_constant<size_t, 2>>>);

		using TTest2 = Meta_as_integer_sequence<TTest>;
		static_assert(std::is_same_v<TTest2, std::integer_sequence<size_t, 0, 1, 2>>);
	*/
	template <class List>
	using Meta_as_integer_sequence =
		typename Meta_as_integer_sequence_<List>::type;

	template <template <class...> class List, class Ty, Ty... Idxs>
	struct Meta_as_integer_sequence_<List<std::integral_constant<Ty, Idxs>...>>
	{
		using type = std::integer_sequence<Ty, Idxs...>;
	};


	//==============================================================================
	template <class...>
	struct Meta_concat_;

	/*	Merge several lists into one.
		
		using TTest = Meta_concat<Meta_list<bool, char>, Meta_list<unsigned int, float>>;
		static_assert(std::is_same_v<TTest, Meta_list<bool, char, unsigned int, float>>);
	*/
	template <class... Types>
	using Meta_concat =
		typename Meta_concat_<Types...>::type;

	template <template <class...> class List>
	struct Meta_concat_<List<>>
	{
		using type = List<>;
	};

	template <template <class...> class List, class... Items1>
	struct Meta_concat_<List<Items1...>>
	{
		using type = List<Items1...>;
	};

	template <template <class...> class List, class... Items1, class... Items2>
	struct Meta_concat_<List<Items1...>, List<Items2...>>
	{
		using type = List<Items1..., Items2...>;
	};

	template <template <class...> class List, class... Items1, class... Items2, class... Items3>
	struct Meta_concat_<List<Items1...>, List<Items2...>, List<Items3...>>
	{
		using type = List<Items1..., Items2..., Items3...>;
	};

	template <template <class...> class List, class... Items1, class... Items2, class... Items3, class... Rest>
	struct Meta_concat_<List<Items1...>, List<Items2...>, List<Items3...>, Rest...>
	{
		using type = Meta_concat<List<Items1..., Items2..., Items3...>, Rest...>;
	};

	//==============================================================================
	/* Transform a list of lists of elements into a single list containing those elements.

		using TTest = Meta_join<Meta_list<Meta_list<bool, char>, Meta_list<unsigned int, float>>>;
		static_assert(std::is_same_v<TTest, Meta_list<bool, char, unsigned int, float>>);
	*/
	template <class ListOfLists>
	using Meta_join =
		Meta_apply<Meta_quote<Meta_concat>, ListOfLists>;

	//==============================================================================
	template <class>
	struct Meta_cartesian_product_;

	/*	Find the n-ary Cartesian Product of the lists in the input list.

		using TTest = Meta_cartesian_product<Meta_list<Meta_list<int>, Meta_list<bool, char>>>;
		static_assert(std::is_same_v<TTest, Meta_list<Meta_list<int, bool>, Meta_list<int, char>>>);

		using TTest2 = Meta_cartesian_product<Meta_list<Meta_list<int, float>, Meta_list<bool, char>>>;
		static_assert(std::is_same_v<TTest2, Meta_list<Meta_list<int, bool>, Meta_list<int, char>, Meta_list<float, bool>, Meta_list<float, char>>>);
	*/
	template <class ListOfLists>
	using Meta_cartesian_product =
		typename Meta_cartesian_product_<ListOfLists>::type;

	template <template <class...> class List>
	struct Meta_cartesian_product_<List<>>
	{
		using type = List<>;
	};

	template <template <class...> class List1, template <class...> class List2, class... Items>
	struct Meta_cartesian_product_<List1<List2<Items...>>>
	{
		using type = List1<List2<Items>...>;
	};

	template <template <class...> class List1, class... Items, template <class...> class List2, class... Lists>
	struct Meta_cartesian_product_<List1<List2<Items...>, Lists...>>
	{
		using type = Meta_join<List1<Meta_transform<Meta_bind_back<Meta_quote<Meta_push_front>, Items>,
			Meta_cartesian_product<List1<Lists...>>>...>>;
	};

} // namespace meta
