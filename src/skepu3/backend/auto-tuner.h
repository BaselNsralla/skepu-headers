#include <skepu3/impl/meta_helpers.hpp>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <skepu3/backend/benchmark.h>
#include "execution_plan.h"
using namespace skepu;

namespace autotuner
{
    // ============== Print cvontext of Pack Index ============
    template<size_t...>
    struct print_index {    
        static void print() {}
    };

    template<size_t t, size_t... things>
    struct print_index<t, things...> {    
        static void print() {
            std::cout << "A size: " << t << std::endl;
            print_index<things...>::print();
        }
    };

    // ============= Print Skepu structures ==================
    template<typename T>
    void print_impl(skepu::Matrix<T>& vec) {
        std::cout << "Matrix Print" << std::endl;
    }

    template<typename T>
    void print_impl(skepu::Vector<T>& vec) {
        std::cout << "Vector Print" << std::endl;
        for(auto& val: vec) {
            std::cout << val << std::endl;
        }
    }

    // =========== Sample Implementations ===================
    template<typename T>
    void sample_impl(skepu::Matrix<T>& mat, size_t size) {
        std::cout << "Matrix Sample" << std::endl;
    }

    template<typename T>
    void sample_impl(skepu::Vector<T>& vec, size_t size) {
        std::cout << "Vector Sample" << std::endl;
        vec.init(size, T{}); // Eller utan T{}
    }
    

    // =========== Helper function to iterate and perform some function, I wish I could pass a template function here
    template<size_t Index>
    struct foreach 
    {    
        // template<typename Tuple, typename... Extra>
        // static void apply(Tuple& tup, Extra&&... extras) {
        //     sample_impl(std::get<Index>(tup), std::forward<Extra>(extras)...);
        //     foreach<Index-1>::apply(tup, std::forward<Extra>(extras)...);
        // }
        template<typename Tuple>
        static void sample(Tuple& tup, size_t size) {
            sample_impl(std::get<Index - 1>(tup), size);
            foreach<Index-1>::sample(tup, size);
        }

        template<typename Tuple>
        static void print(Tuple& tup) {
            print_impl(std::get<Index - 1>(tup));
            foreach<Index-1>::print(tup);
        }
    };

    template<>
    struct foreach<0>
    {
        // template<typename Tuple, typename... Extra>
        // static void apply(Tuple& tup, Extra&&... extras) {}
        template<typename Tuple>
        static void sample(Tuple& tup, size_t size) {}
        template<typename Tuple>
        static void print(Tuple& tup) {}
    };
    

    template<typename Tuple, size_t... Idx>
    void sample_all(Tuple& tup, size_t size, pack_indices<Idx...> ) {
        //size_t l = { (sample_impl(std::get<Idx>(tup)), 0)..., 0 };
        std::initializer_list<size_t> indicies = {Idx...};
        //auto m = std::max(indicies, [](size_t const& a, size_t const& b) { return a < b; });
        std::cout << std::tuple_size<Tuple>::value - 1 << std::endl;
        foreach< std::template tuple_size<Tuple>::value >::sample(tup, size);
    }

    template<typename Tuple, size_t... Idx>
    void print_all(Tuple& tup, pack_indices<Idx...> ) {
        //size_t l = { (sample_impl(std::get<Idx>(tup)), 0)..., 0 };
        std::initializer_list<size_t> indicies = {Idx...};
        //auto m = std::max(indicies, [](size_t const& a, size_t const& b) { return a < b; });
        std::cout << std::tuple_size<Tuple>::value - 1 << std::endl;
        foreach< std::template tuple_size<Tuple>::value >::print(tup);
    }

    template<bool PrefersMatrix, typename T> 
    using PreferedContainer = typename std::conditional<PrefersMatrix, skepu::Matrix<T>, skepu::Vector<T>>::type;

    template<bool PrefersMatrix, typename TupleType>
    using ArgContainerTup = typename std::conditional<PrefersMatrix, 
                                    typename add_container_layer<skepu::template Matrix, TupleType>::type,
                                    typename add_container_layer<skepu::template Vector, TupleType>::type>::type;

    template<typename Skeleton, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    void tune(Skeleton& skeleton, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<CI...> ci, pack_indices<UI...> ui) 
    {
        // ta ut varje parameter frå call args och använd dem i implementationen gör en benchmark och
        // spara undan, kanske låta resten av programmet köra medan vi håller på här?        
        std::cout << "============" << std::endl;
        static constexpr bool pm = Skeleton::prefers_matrix;

        PreferedContainer<pm, int> hi{};
        
        ArgContainerTup<pm, typename Skeleton::ResultArg> resultArg;
        ArgContainerTup<pm, typename Skeleton::ElwiseArgs> elwiseArg;
        ArgContainerTup<pm, typename Skeleton::ContainerArgs> containerArg;
        ArgContainerTup<pm, typename Skeleton::UniformArgs> uniArg;
        
        int current_size = 40;
        sample_all(resultArg,    current_size, oi);
        sample_all(elwiseArg,    current_size, ei);
        sample_all(containerArg, current_size, ci);
        sample_all(uniArg,       current_size, ui);
        //sample_all(resultArg,    current_size, oi);
        //sample_all(elwiseArg,    current_size, ei);
        //sample_all(containerArg, current_size, ci);
        //sample_all(uniArg,       current_size, ui);
        

        size_t repeats = 5; 
        size_t size = 0;
        auto mintime = benchmark::TimeSpan::max();
        auto backendTypes = Backend::availableTypes();
        std::vector<BackendSpec> specs(backendTypes.size());
        std::transform(backendTypes.begin(), backendTypes.end(), specs.begin(), [](Backend::Type& type) {
            return BackendSpec(type);
        });

        std::pair<Backend::Type, benchmark::TimeSpan> bestDuration{Backend::Type::CPU, benchmark::TimeSpan::max()};

    	benchmark::measureForEachBackend(repeats, size, specs, 
            [&](size_t, BackendSpec spec) {
                skeleton.setBackend(spec);
                //std::cout << "INVONKLING " << std::endl;
                // Smple with given size?
                skeleton(std::get<OI>(resultArg)...,
                std::get<EI>(elwiseArg)...,
                std::get<CI>(containerArg)...,
                std::get<UI>(uniArg)...);
            },
            [](benchmark::TimeSpan duration){
                //std::cout << "Benchmark duration " << duration.count() << std::endl; 
            },
            [&](Backend::Type backend, benchmark::TimeSpan duration) {
                //(std::cout << "Median " << backend << " Took " << duration.count() << std::endl;
                if (bestDuration.second > duration) {
                    bestDuration = {backend, duration};
                }
            });
        
        std::cout << "BEST BACKEND " << bestDuration.first << std::endl;
        ExecutionPlan plan{}; 
        plan.insert(bestDuration.first, current_size);

        for (auto backend : skepu::Backend::availableTypes())
		{
            skepu::BackendSpec spec(backend);
			skeleton.setBackend(spec);
            const auto start = std::chrono::steady_clock::now();
            skeleton(std::get<OI>(resultArg)...,
                    std::get<EI>(elwiseArg)...,
                    std::get<CI>(containerArg)...,
                    std::get<UI>(uniArg)...);

            const auto end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << backend << "Chrono Time in microseconds: " << elapsed.count() << '\n';
        }
        print_index<EI...>::print();
        std::cout << "============" << std::endl;
    }

    template<typename Skeleton>
    void tuneWrapper(Skeleton&  skeleton) {
        tune(
            skeleton,
            typename make_pack_indices<std::tuple_size<typename Skeleton::ResultArg>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ElwiseArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ContainerArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::UniformArgs>::value>::type()
        );
    }
} // namespace autotuner
