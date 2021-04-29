
#ifndef AUTOTUNER_H
#define AUTOTUNER_H

#include <skepu3/impl/meta_helpers.hpp>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <skepu3/backend/benchmark.h>
#include "execution_plan.h"
#include <cmath>        // std::pow
#include <fstream>
#include <skepu3/impl/common.hpp>
//#include <skepu3/backend/map.h>
//#include <skepu3/backend/mapoverlap.h> Skapara problem
using namespace skepu;
using namespace skepu::backend;

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
            std::cout << "Index Position: " << t << std::endl;
            print_index<things...>::print();
        }
    };

    // ============= Print Skepu structures ==================
    template<typename T>
    void print_impl(skepu::Matrix<T>& vec) {
        std::cout << "Matrix Print" << std::endl;
        for(auto& val: vec) {
            std::cout << val << std::endl;
        }
    }

    template<typename T>
    void print_impl(skepu::Vector<T>& vec) {
        std::cout << "Vector Print" << std::endl;
        for(auto& val: vec) {
            std::cout << val << std::endl;
        }
    }

    // =========== Sample Implementations ===================

    void sample_impl(size_t& a, size_t size) {
        std::cout << "##### Fix sampling of uniform arguments #####" << std::endl;
        a = 1;
    }
    
    template<typename T>
    void sample_impl(skepu::Matrix<T>& mat, size_t size) {
        std::cout << "Matrix Sample " << size << std::endl;
        mat.init(size, size, T{});
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
    
    // ======================== Apply Util Functions on All the Tuples ===========================
    template<typename Tuple, size_t... Idx>
    void sample_all(Tuple& tup, size_t size, pack_indices<Idx...> ) {
        //size_t l = { (sample_impl(std::get<Idx>(tup)), 0)..., 0 };
        std::initializer_list<size_t> indicies = {Idx...};
        //auto m = std::max(indicies, [](size_t const& a, size_t const& b) { return a < b; });
        // std::cout << std::tuple_size<Tuple>::value - 1 << std::endl;
        foreach< std::template tuple_size<Tuple>::value >::sample(tup, size);
    }

    template<typename Tuple, size_t... Idx>
    void print_all(Tuple& tup, pack_indices<Idx...> ) {
        //size_t l = { (sample_impl(std::get<Idx>(tup)), 0)..., 0 };
        std::initializer_list<size_t> indicies = {Idx...};
        //auto m = std::max(indicies, [](size_t const& a, size_t const& b) { return a < b; });
        // std::cout << std::tuple_size<Tuple>::value - 1 << std::endl;
        foreach< std::template tuple_size<Tuple>::value >::print(tup);
    }

    template<bool PrefersMatrix, typename TupleType>
    using ArgContainerTup = typename std::conditional<PrefersMatrix, 
                                    typename add_container_layer<skepu::template Matrix, TupleType>::type,
                                    typename add_container_layer<skepu::template Vector, TupleType>::type>::type;

    template<bool PrefersMatrix, typename T> 
    using PreferedContainer = typename std::conditional<PrefersMatrix, skepu::Matrix<T>, skepu::Vector<T>>::type;
    
    template<typename Skeleton>
    struct MapOverlapSampler 
    {
        // Typerna för variablerna och sample metoden bör abstraheras till flera using deklarationer
        size_t size;

        MapOverlapSampler(size_t size): size{size} {}

        static constexpr bool pm = true; 
        ArgContainerTup<pm, typename Skeleton::ResultArg>     resultArg;
        ArgContainerTup<pm, typename Skeleton::ElwiseArgs>    elwiseArg;
        ArgContainerTup<pm, typename Skeleton::UniformArgs>   containerArg;
        std::tuple<size_t>                                    uniArg;

        void sample() 
        {
           foreach< std::template tuple_size<ArgContainerTup<pm, typename Skeleton::ResultArg> >::value >::sample(resultArg,    size);
           foreach< std::template tuple_size<ArgContainerTup<pm, typename Skeleton::ElwiseArgs>>::value >::sample(elwiseArg,    size);
           foreach< std::template tuple_size<std::tuple<size_t>>::value>::sample(uniArg, size);
           foreach< std::template tuple_size<ArgContainerTup<pm, typename Skeleton::UniformArgs> >::value >::sample(containerArg, size);

        }
    };

    template<typename Skeleton>
    struct ASampler // Specialization not needed for now<Skeleton, false>
    {
        size_t size;
        ASampler(size_t size): size{size} {}

        static constexpr bool pm = true; 
        ArgContainerTup<pm, typename Skeleton::ResultArg>     resultArg;
        ArgContainerTup<pm, typename Skeleton::ElwiseArgs>    elwiseArg;
        ArgContainerTup<pm, typename Skeleton::ContainerArgs> containerArg;
        //typename Skeleton::UniformArgs coArgs;
        ArgContainerTup<pm, typename Skeleton::UniformArgs>   uniArg;

        void sample() 
        {
           foreach< std::template tuple_size<ArgContainerTup<pm, typename Skeleton::ResultArg> >::value >::sample(resultArg,    size);
           foreach< std::template tuple_size<ArgContainerTup<pm, typename Skeleton::ElwiseArgs>>::value >::sample(elwiseArg,    size);
           foreach< std::template tuple_size<ArgContainerTup<pm, typename Skeleton::ContainerArgs>>::value >::sample(containerArg, size);
           foreach< std::template tuple_size<ArgContainerTup<pm, typename Skeleton::UniformArgs> >::value >::sample(uniArg,       size);
           //foreach< std::template tuple_size<typename Skeleton::ContainerArgs>::value >::sample(coArgs,       size);
        }

    };


    // ======================= Autotuning ========================================================
    template<typename Skeleton>
    using isMapOverlap = typename std::integral_constant<bool, Skeleton::skeletonType == SkeletonType::MapOverlap2D>;

    template<typename Skeleton>
    using ConditionalSampler = typename std::conditional<isMapOverlap<Skeleton>::value, MapOverlapSampler<Skeleton>, ASampler<Skeleton>>::type;

    template<typename Skeleton, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    void tune(Skeleton& skeleton, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<CI...> ci, pack_indices<UI...> ui) 
    {
        // ta ut varje parameter frå call args och använd dem i implementationen gör en benchmark och
        // spara undan, kanske låta resten av programmet köra medan vi håller på här?        
        std::cout << "============" << std::endl;
        //Skeleton::prefers_matrix;

        //PreferedContainer<pm, int> hi{};
        static constexpr size_t MAXSIZE = std::pow<size_t>(size_t(2), size_t(36));
        static constexpr size_t MAXPOW  = 13;//26; // TODO: 2^27-2^28 breaks my GPU :( TODO: borde sättas baserat på GPU capacity
        auto baseTwoPower = [](size_t exp) -> size_t { return std::pow<size_t>(size_t(2), exp); };
        ExecutionPlan plan{}; 

        for (size_t i = 4; i <= MAXPOW; ++i) 
        {
            size_t current_size = baseTwoPower(i);
            std::cout << "Sample Size is " << 2 << " POW " << i << " " << current_size << std::endl;
            // ArgContainerTup<pm, typename Skeleton::ResultArg> resultArg;
            // ArgContainerTup<pm, typename Skeleton::ElwiseArgs> elwiseArg;
            // ArgContainerTup<pm, typename Skeleton::ContainerArgs> containerArg;
            // ArgContainerTup<pm, typename Skeleton::UniformArgs> uniArg;
            
            //sample_all(resultArg,    current_size, oi);
            //sample_all(elwiseArg,    current_size, ei);
            //sample_all(containerArg, current_size, ci);
            //sample_all(uniArg,       current_size, ui);

            ConditionalSampler<Skeleton> sampler{current_size};
            sampler.sample();

            //print_all(elwiseArg, ei); // make sure to print something with a size if you want to se anything
            print_index<UI...>::print();

            size_t repeats = 5; 
            size_t size    = 0;
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
                    std::cout << "INVONKING " << std::endl;
                    skeleton(
                        std::get<OI>(sampler.resultArg)...,
                        std::get<EI>(sampler.elwiseArg)...,
                        std::get<CI>(sampler.containerArg)...,
                        std::get<UI>(sampler.uniArg)...);
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
            plan.insert(bestDuration.first, current_size);


            // for (auto backend : skepu::Backend::availableTypes())
            // {
            //     skepu::BackendSpec spec(backend);
            //     skeleton.setBackend(spec);
            //     const auto start = std::chrono::steady_clock::now();
            //     skeleton(std::get<OI>(resultArg)...,
            //             std::get<EI>(elwiseArg)...,
            //             std::get<CI>(containerArg)...,
            //             std::get<UI>(uniArg)...);

            //     const auto end = std::chrono::steady_clock::now();
            //     auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            //     std::cout << backend << "Chrono Time in microseconds: " << elapsed.count() << '\n';
            // }
            // print_index<EI...>::print();
            // std::cout << "============" << std::endl;
        }
        
        std::cout << "WRITING ###########" << std::endl;
        std::ofstream file("/home/lized/Skrivbord/test/ok1337_2.json");
        if(file) {
            file << plan << std::flush;
            std::cout << "###########OK" << std::endl;
        }

        std::ifstream sfile("/home/lized/Skrivbord/test/ok1337_2.json");
        sfile >> plan;

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

    template<typename T>
    struct Tuner 
    {
        void autotuning() 
        {
            autotuner::tuneWrapper(*(static_cast<T*>(this)));
        }
    };

} // namespace autotuner

#endif