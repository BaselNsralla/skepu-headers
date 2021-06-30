#pragma once

#include <tuple>
#include <vector>
#include <skepu3/backend/environment.h>
#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/size.h>
#include <functional>
#include <skepu3/backend/autotuning/tune_range.h>

namespace skepu
{
	namespace backend 
	{
		namespace autotune 
		{

			template<size_t ISize, typename... T>
			struct args_tuple 
			{

				template<size_t... Is>
				static auto value(T&&... args) -> decltype(std::make_tuple(get<Is, T...>(args...)...))
				{
					return std::make_tuple(get<Is, T...>(args...)...);
				}


				template<size_t... Is>
				static auto value(pack_indices<Is...> , T... args) -> decltype(std::make_tuple(get<Is, T...>(args...)...))
				{
					return std::make_tuple(get<Is, T...>(args...)...);
				}
			}; 

			template<typename... T>
			struct args_tuple<0u, T...> 
			{

				template<size_t... Is>
				static std::tuple<> value(T&&... args) 
				{
					return std::tuple<>();
				}

				template<size_t... Is>
				static std::tuple<> value(pack_indices<Is...>, T... args)
				{
					return std::tuple<>();
				}
				
			}; 



			template<size_t... Is>
			struct seq { };

			template<size_t N, size_t... Is>
			struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

			template<size_t... Is>
			struct gen_seq<0, Is...> : seq<Is...> { };

			namespace AT = autotune;

			using AltSizeFunc = std::function<AT::Size(void)>;

			static AltSizeFunc defaultAltSize = [] () {
				return AT::Size{0,0};
			};

			template<typename T>
			AT::Size get_size_impl(skepu::Matrix<T> p,  AltSizeFunc altSize) 
			{
				return AT::Size
					{
						std::min(ulog2(p.size_i()), MULTI_DIM), 
						std::min(ulog2(p.size_j()), MULTI_DIM)
					};
			}

			template<typename T>
			AT::Size get_size_impl(skepu::Vector<T> p,  AltSizeFunc altSize) 
			{
				return AT::Size 
					{
						std::min(ulog2(p.size()), SINGLE_DIM), 0u
					};
			}
			
			// TODO: Hantera flera typer här
			template<typename T>
			AT::Size get_size_impl(T p,  AltSizeFunc altSize) 
			{
				return AT::Size{std::min(ulog2(p), SINGLE_DIM), 0u}; // TODO: COULD CAUSE BUGS?
			}
			
			template<typename T>
			AT::Size get_size_impl(skepu::VectorIterator<T> p, AltSizeFunc altSize) 
			{
				return altSize();
			}

			template<typename T>
			AT::Size get_size_impl(skepu::MatrixIterator<T> p, AltSizeFunc altSize) 
			{
				return altSize();
			}

			template<typename... T, size_t... I>
			std::vector<AT::Size> get_size(std::tuple<T...> paramGroup, seq<I...>, AltSizeFunc altSize = defaultAltSize) 
			{
				//auto l = { (f(std::get<Is>(t)), 0)... }; ret l i värsta fall?
				//context { (argSize.add(combinations<false>(Types())), 0)... };
				std::vector<AT::Size> result{ get_size_impl(std::get<I>(paramGroup), altSize)... };
				if (result.size() == 0) {
					result.push_back(AT::Size{0u,0u});
				}
				return result;
			}

			struct DispatchSize {
				
				size_t legacy;
				std::vector<AT::Size> outputSize; 
				std::vector<AT::Size> elwiseSize; 
				std::vector<AT::Size> containerSize;
				std::vector<AT::Size> uniformSize;
				
				// TODO: hur kan vi ta in dem som referenser?
				template<typename... Res, typename... In, typename... Cont, typename... Uni> //typename... Out,
				static DispatchSize Create(//std::tuple<Out...> outParams,
								size_t essentialSize,
								std::tuple<Res...> resArg,
								std::tuple<In...> inArg, 
								std::tuple<Cont...> contArg, 
								std::tuple<Uni...> uniArg) 
				{
					auto altSize = [&essentialSize]() { return AT::Size{ulog2(essentialSize), 0}; };

					auto d = DispatchSize {
						essentialSize,
						get_size(resArg,  gen_seq<sizeof...(Res)>(),  altSize), 
						get_size(inArg,   gen_seq<sizeof...(In)>(),   altSize), 
						get_size(contArg, gen_seq<sizeof...(Cont)>(), altSize),
						get_size(uniArg,  gen_seq<sizeof...(Uni)>(),  altSize)
					};

					return d;
				}

			};
		}
	}

} // namespace skepu
