#pragma once

#include <tuple>
#include <vector>
#include <skepu3/backend/environment.h>
#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/size.h>
#include <functional>
#include <cmath>

namespace skepu
{
	namespace backend
	{	
		using std::log2;

		template<typename T>
		size_t ulog2(T num) {
			return log2(num);
		}

		template<size_t ISize, typename... T>
		struct args_tuple {

			template<size_t... Is>
			static auto value(T&&... args) -> decltype(std::make_tuple(get<Is, T...>(args...)...))
			{
				return std::make_tuple(get<Is, T...>(args...)...);
			}  
		}; 

		template<typename... T>
		struct args_tuple<0u, T...> {

			template<size_t... Is>
			static std::tuple<> value(T&&... args) 
			{
				return std::tuple<>();
			}  
		}; 


		namespace AT = autotuner;

		using AltSizeFunc = std::function<AT::Size(void)>;

		template<size_t... Is>
		struct seq { };

		template<size_t N, size_t... Is>
		struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

		template<size_t... Is>
		struct gen_seq<0, Is...> : seq<Is...> { };


		template<typename T>
		AT::Size get_size_impl(skepu::Matrix<T> p,  AltSizeFunc altSize) 
		{
			return AT::Size{ulog2(p.size_i()), ulog2(p.size_j())};
		}

		template<typename T>
		AT::Size get_size_impl(skepu::Vector<T> p,  AltSizeFunc altSize) 
		{
			return AT::Size{ulog2(p.size()), 0u};
		}
		
		// TODO: Hantera flera typer här
		template<typename T>
		AT::Size get_size_impl(T p,  AltSizeFunc altSize) 
		{
			return AT::Size{ulog2(p), 0u};
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
		std::vector<AT::Size> get_size(std::tuple<T...> paramGroup, seq<I...>, AltSizeFunc altSize) 
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
			template< typename... In, typename... Cont, typename... Uni> //typename... Out,
			static DispatchSize Create(//std::tuple<Out...> outParams,
							  size_t outSize, 
							  std::tuple<In...> inParams, 
							  std::tuple<Cont...> contParams, 
							  std::tuple<Uni...> uniParams) 
			{
				auto outputSize = [&outSize]() { return AT::Size{ulog2(outSize), 0}; };
				//get_size(inParams, gen_seq<sizeof...(In)>(), outputSize);

				auto d = DispatchSize {
					outSize,
					{outputSize()}, 
					get_size(inParams,   gen_seq<sizeof...(In)>(),   outputSize), 
					get_size(contParams, gen_seq<sizeof...(Cont)>(), outputSize),
					get_size(uniParams,  gen_seq<sizeof...(Uni)>(),  outputSize)
				};
				// std::cout << "DISPATCH SIZE" << std::endl;
				// for (auto& a: d.elwiseSize) {
				// 	std::cout << a.x << " " << std::endl;
				// }
				return d;
			}



		};
    }

}