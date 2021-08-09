#pragma once

#include <cstdio>
#include <tuple>
using std::tuple;

// DEPRECATED ~~~~~~~~~~~~~~~~~~~~~~~~DEPRECATED~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ DEPRECATED


// #####################################################
template <std::size_t...> struct index_sequence {};

template <std::size_t N, std::size_t... Is>
struct make_index_sequence : make_index_sequence<N / 2, N / 2, Is...> {}; // delta med 2

template <std::size_t... Is> // 4 som basfall
struct make_index_sequence<4u, Is...> : index_sequence<Is...> { using type = index_sequence<Is...>; }; // gör en till för 0

template <typename... T>
struct ssequence {};

template<typename... SSequences>
struct seq_pack {
	//using as_sequences = sequence_sequence<SSequences::as_index_sequence...>;
};

template<typename Seq>
struct ready {};
// ####################################################

namespace as_detail {
  	
  	template<typename... T>
  	struct decompose {};
  
	// One
	template<typename... T>
	struct decompose<ssequence<T...>> {
		using type = ssequence<T...>;
	};

	// All
	template<typename... T, typename... R, typename... SSeq>
	struct decompose<ssequence<T...>, ssequence<R...>, SSeq...> { 
		using type = typename decompose<ssequence<T..., R...>, SSeq...>::type; // Kanske anropa 
	};

	template<typename pack>
	struct ent_decompose {};

    template<typename... Rest>
    struct ent_decompose< seq_pack<Rest...> >
	{ 
		using type = typename decompose<Rest...>::type;
	}; 
};

template<typename SeqPack>
struct as_sequence {
	using type = typename as_detail::template ent_decompose<SeqPack>::type;
};



// ##############################################
template<size_t S, size_t... Ts>
struct apply_perm {
	using type = ssequence<index_sequence<S, Ts>...>;
};

template<size_t S, typename T>
struct extend {};

template <size_t S, size_t... sizes>
struct extend<S, index_sequence<sizes...>> {
	using type = index_sequence<sizes..., S>;
};
	
template<size_t S, typename... Is>
struct apply_extend {
	using type = ssequence<typename extend<S, Is>::type...>;
};
// #############################################


// -------------------------------
template<typename... A>//template<typename A, typename B>
struct combine {};



//--------------------------------------------
// Kolla om basfall combine kräver variadic i template:sen
template<size_t... A>
struct combine<index_sequence<A...>>
{
	// För varje B kombinera med alla A.
	using type = ssequence<index_sequence<A...>>;
};

template<typename... Is>
struct combine<ready<ssequence<Is...>>>
{
	using type = ssequence<Is...>;
};



template<typename... Is, size_t... A, typename... Rest>
struct combine<ready<ssequence<Is...>>, index_sequence<A...>, Rest...>
{
	using type = typename combine<ready< typename as_sequence<seq_pack<typename apply_extend<A, Is...>::type...>>::type >, Rest...>::type;
};


template<size_t... A, size_t... B, typename... Rest>
struct combine<index_sequence<A...>, index_sequence<B...>, Rest...>
{
	// För varje B kombinera med alla A.
	using type = typename combine<ready< typename as_sequence<seq_pack<typename apply_perm<A, B...>::type...> >::type >, Rest...>::type;
};

// --------------------------------

template<typename... R>
struct permutation_sequence
{
	using type = typename combine<R...>::type;
};

template<typename... T>
struct setup_permuation_sequence
{
};

template<typename... T, typename... R, typename... Q, typename... S>
struct setup_permuation_sequence<tuple<T...>, tuple<R...>, tuple<Q...>, tuple<S...>>
{
	using type = typename permutation_sequence<T..., R..., Q..., S...>::type;
};


template<size_t size, typename T>
struct seq_wrapper {};

template<size_t size, typename... T>
struct seq_wrapper<size, tuple<T...>> {
	using type = typename std::conditional<sizeof...(T) == 0, tuple<>, tuple<typename make_index_sequence<size>::type>>::type;	
};


template<typename Is, typename... Args>
struct consume {};

template<size_t first, size_t... sizes, typename... A>
struct consume<index_sequence<first, sizes...>, tuple<A...>> // consume ko
{
    using type = typename std::conditional<sizeof...(A) == 0, index_sequence<first, sizes..., 0u>, index_sequence<sizes..., first>>::type;
};

template<size_t first, size_t... sizes, typename... A, typename... Others>
struct consume<index_sequence<first, sizes...>, tuple<A...>, Others...> 
{
    //sizeof...(A)
    using type = typename std::conditional<sizeof...(A) == 0, consume<index_sequence<first, sizes..., 0u>, Others...>,  consume<index_sequence<sizes..., first>, Others...>>::type::type;  
};

// Should take them as variadic
template<typename Is, typename A, typename B, typename C, typename D>
struct normalize_sequence {};

template<size_t... sizes, typename A, typename B, typename C, typename D>
struct normalize_sequence<index_sequence<sizes...>, A, B, C, D>
{
    using type = typename consume<index_sequence<sizes...>, A, B, C, D>::type; 
};


template<typename Seq, typename... Types>
struct normalize {};

template<typename... Is, typename... Types>
struct normalize<ssequence<Is...>, Types...> 
{
	// template<typename F>
	// static void apply(F& f)	{
 	// 	context { (f.template apply<Is>(), 0)...};
    // }
    //using type = typename normalize_sequence<IndexSequence, Types...>::type;	
	using type = ssequence<typename normalize_sequence<Is, Types...>::type...>;	// TODOD TESTKÖR


};