#pragma once

#include <tuple>
#include <vector>
#include <skepu3/backend/environment.h>
#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/size.h>
#include <functional>
#include <skepu3/backend/autotuning/tune_limit.h>

namespace skepu
{
	namespace backend 
	{
		namespace autotune 
		{
			struct args_tuple_base 
			{
				static std::tuple<> empty() { return std::tuple<>(); } 
			};

			template<size_t ISize, typename... T>
			struct args_tuple : args_tuple_base
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
			struct args_tuple<0u, T...> : args_tuple_base
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
			AT::Size get_size_impl(SampleLimit limit, skepu::Matrix<T> p,  AltSizeFunc altSize) 
			{
				return AT::Size
					{
						std::min(ulog2(p.size_i()), limit.multi_dim), 
						std::min(ulog2(p.size_j()), limit.multi_dim)
					};
			}

			template<typename T>
			AT::Size get_size_impl(SampleLimit limit, skepu::Vector<T> p,  AltSizeFunc altSize) 
			{
				return AT::Size 
					{
						std::min(ulog2(p.size()), limit.single_dim), 0u
					};
			}
			
			// TODO: Hantera flera typer här
			template<typename T>
			AT::Size get_size_impl(SampleLimit limit, T p,  AltSizeFunc altSize) 
			{
				return AT::Size{std::min(ulog2(p), limit.single_dim), 0u}; // TODO: COULD CAUSE BUGS?
			}
			
			template<typename T>
			AT::Size get_size_impl(SampleLimit limit, skepu::VectorIterator<T> p, AltSizeFunc altSize) 
			{
				return altSize();
			}

			template<typename T>
			AT::Size get_size_impl(SampleLimit limit, skepu::MatrixIterator<T> p, AltSizeFunc altSize) 
			{
				return altSize();
			}

			template<typename... T, size_t... I>
			std::vector<AT::Size> get_size(SampleLimit limit, std::tuple<T...> paramGroup, seq<I...>, AltSizeFunc altSize = defaultAltSize) 
			{
				//auto l = { (f(std::get<Is>(t)), 0)... }; ret l i värsta fall?
				//context { (argSize.add(combinations<false>(Types())), 0)... };
				std::vector<AT::Size> result{ get_size_impl(limit, std::get<I>(paramGroup), altSize)... };
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
								SampleLimit limit,
								size_t essentialSize,
								std::tuple<Res...> resArg,
								std::tuple<In...> inArg, 
								std::tuple<Cont...> contArg, 
								std::tuple<Uni...> uniArg) 
				{
					auto altSize = [&essentialSize]() { return AT::Size{ulog2(essentialSize), 0}; };

					auto d = DispatchSize {
						essentialSize,
						get_size(limit, resArg,  gen_seq<sizeof...(Res)>(),  altSize), 
						get_size(limit, inArg,   gen_seq<sizeof...(In)>(),   altSize), 
						get_size(limit, contArg, gen_seq<sizeof...(Cont)>(), altSize),
						get_size(limit, uniArg,  gen_seq<sizeof...(Uni)>(),  altSize)
					};

					return d;
				}
				
				void collapseDimension(std::vector<AT::Size>& someArg) 
				{
					std::vector<AT::Size> collapsed(someArg.size());
					std::transform(someArg.begin(), someArg.end(), collapsed.begin(), [] (AT::Size const& multiDim) {
						size_t y = multiDim.y == 0 ? 1 : multiDim.y;
						return AT::Size{multiDim.y*multiDim.x, 0};
					});

					someArg = std::move(collapsed);
				}

			};
		}
	}

} // namespace skepu
