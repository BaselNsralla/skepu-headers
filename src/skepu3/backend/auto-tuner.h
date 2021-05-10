
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

/*
    TODO: 
        Se till att prefered container är rätt
        Se till att alla skeletons funkar med autotuning
        Se till att kunna använda autotune som backend
        Kolla hur vi kan göra för ta bort och lägga till execution plan
        Kolla hur vi kan göra för att justera max storlek på test
        Kolla varför scan inte kör autotuning
        Printa COMPILEID
            Få in COMPILEID i JSON filen

        Flush to disk with id, send uuid from bash
*/


using namespace skepu;
using namespace skepu::backend;

namespace autotuner
{   
    using context = std::initializer_list<int>;
 


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
        a = std::max<size_t>(2, 0.2*size);//size;
        std::cout << "Set to " << a << std::endl;
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


    // =========== Helper function to iterate and perform some function, Would not have been need if auto was available in lambda params
    
    struct foreach
    {    
        template<size_t... COUNT, typename Tuple>
        static void sample(Tuple& tup, size_t size) 
        {
            context{ (sample_impl(std::get<COUNT>(tup), size), 0)... };
        }

        template<size_t... COUNT, typename Tuple>
        static void print(Tuple& tup) 
        {
            context{ (print_impl(std::get<COUNT>(tup)), 0)... };
        }

    };
    
    // ========= SAMPLER CLASS  ==============

    template<bool PrefersMatrix, typename TupleType>
    using ArgContainerTup = typename std::conditional<PrefersMatrix, 
                                    typename add_container_layer<skepu::template Matrix, TupleType>::type,
                                    typename add_container_layer<skepu::template Vector, TupleType>::type>::type;

    template<bool PrefersMatrix, typename T> 
    using PreferedContainer = typename std::conditional<PrefersMatrix, skepu::Matrix<T>, skepu::Vector<T>>::type;
    
    template<typename Skeleton, typename Element>
    struct Containerize 
    {
        static constexpr bool pm = Skeleton::skeletonType == SkeletonType::MapOverlap2D || Skeleton::prefers_matrix;
        using type = PreferedContainer<pm, Element>;
    };

    // template<typename Skeleton>
    // struct Containerize<Skeleton, skepu::Index1D>
    // {
    //     using type = skepu::Vector<typename Skeleton::ResultArg>; // Not sure about result arg here since it's a tuple
    // };

    // template<typename Skeleton>
    // struct Containerize<Skeleton, skepu::Index2D>
    // {
    //     using type = skepu::Matrix<typename Skeleton::ResultArg>; // Specializations går inte eftersom T är tuplar
    // };

    template<typename Skeleton, typename T>
    struct Containerize<Skeleton, skepu::Mat<T>> 
    {
        using type = skepu::Matrix<T>;
    };

    template<typename Skeleton, typename T>
    struct Containerize<Skeleton, skepu::Vec<T>> 
    {
        using type = skepu::Vector<T>;
    };

    template<typename Skeleton, typename T>
    struct Containerize<Skeleton, skepu::MatCol<T>> 
    {
        using type = skepu::Matrix<T>;
    };
    
    template<typename Skeleton, typename T>
    struct Containerize<Skeleton, skepu::MatRow<T>> 
    {
        using type = skepu::Matrix<T>;
    };

    template<typename Skeleton, typename T>
    struct containerized_layer
    {};

    template<typename Skeleton, typename... Types>
    struct containerized_layer<Skeleton, std::tuple<Types...>>
    {
        using type = std::tuple<typename Containerize<Skeleton,typename std::decay<Types>::type>::type...>;
    };

    


    template<typename Skeleton>
    struct Sampler 
    {
        // static constexpr bool pm = Skeleton::skeletonType == SkeletonType::MapOverlap2D || 
        //                            Skeleton::skeletonType == SkeletonType::Scan         ||
        //                            Skeleton::prefers_matrix;

        using ElwiseWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ElwiseArgs>::type;//ArgContainerTup<pm, typename Skeleton::ElwiseArgs>;
        using ResultWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ResultArg>::type;//ArgContainerTup<pm, typename Skeleton::ResultArg>;
        using ContainerWrapped = typename containerized_layer<Skeleton, typename Skeleton::ContainerArgs>::type;//ArgContainerTup<pm, typename Skeleton::ContainerArgs>;
        using UniformWrapped   = typename containerized_layer<Skeleton, typename Skeleton::UniformArgs>::type;//ArgContainerTup<pm, typename Skeleton::UniformArgs>;
    };




    template<typename Skeleton>
    struct MapOverlapSampler: public Sampler<Skeleton>
    {
        // Typerna för variablerna och sample metoden bör abstraheras till flera using deklarationer
        Skeleton& skeleton;
        size_t size;

        MapOverlapSampler(Skeleton& skeleton, size_t size): skeleton{skeleton}, size{size} {}

        using Parent = Sampler<Skeleton>;

        using typename Parent::ResultWrapped;
        using typename Parent::ElwiseWrapped;
        using typename Parent::ContainerWrapped;
        using typename Parent::UniformWrapped; 

        ResultWrapped      resultArg;
        ElwiseWrapped      elwiseArg;
        ContainerWrapped   containerArg;
        std::tuple<size_t> uniArg;

        template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>
        void sample(pack_indices<OI...>, 
                    pack_indices<EI...>, 
                    pack_indices<CI...>, 
                    pack_indices<UI...>) 
        {

            foreach::sample<0>(uniArg, size);
            size_t kernel = std::get<0>(uniArg);
            size_t padedSize = size + (kernel*2);
            skeleton.setOverlap(kernel, kernel); 
            
            size_t& k = std::get<0>(uniArg); k=1;
           
            foreach::sample<OI...>(resultArg,    size);
            foreach::sample<EI...>(elwiseArg,    padedSize);
            foreach::sample<CI...>(containerArg, size);

                        
            // this->resultArg    = std::move(resultArg);
            // this->elwiseArg    = std::move(elwiseArg);
            // this->containerArg = std::move(containerArg);
        }
    };


    /*
    If the this will be created once in the tuner function then the 
    template function sample, can be replaced by a normal function
    and the template parameters can be included in the strcut template
    with nameless arguments in the constructor  

    The reason for the  sample being a template here is becasue the indicies will
    be very hard to handle outside the construtor, (i.e declaring the type only)
    because i don't really know if it can see different contigous spreads as different
    template parameters,

    Example:
    X<EI.., CI...> will this cause ambiguity?
    */
    template<typename Skeleton>
    struct ASampler: public Sampler<Skeleton> 
    {

        Skeleton& skeleton;
        size_t size;
        ASampler(Skeleton& skeleton, size_t size): skeleton{skeleton}, size{size} {}

        using Parent = Sampler<Skeleton>;

        using typename Parent::ResultWrapped;
        using typename Parent::ElwiseWrapped;
        using typename Parent::ContainerWrapped;
        using typename Parent::UniformWrapped; 


        ResultWrapped    resultArg;
        ElwiseWrapped    elwiseArg;
        ContainerWrapped containerArg;
        UniformWrapped   uniArg;

        template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>  
        void sample(
                pack_indices<OI...>, 
                pack_indices<EI...>, 
                pack_indices<CI...>, 
                pack_indices<UI...>) 
        {
            foreach::sample<OI...>(resultArg,    size);
            foreach::sample<EI...>(elwiseArg,    size);
            foreach::sample<CI...>(containerArg, size);
            foreach::sample<UI...>(uniArg,       size);
        }

    };


    // ======================= Autotuning ========================================================
    template<typename Skeleton>
    using isMapOverlap = typename std::integral_constant<bool, Skeleton::skeletonType == SkeletonType::MapOverlap2D>;

    template<typename Skeleton> // binary specialization, would need an extension to nonbinary
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
        static constexpr size_t MAXPOW  = 8;//26; // TODO: 2^27-2^28 breaks my GPU :( TODO: borde sättas baserat på GPU capacity
        auto baseTwoPower = [](size_t exp) -> size_t { return std::pow<size_t>(size_t(2), exp); };
        ExecutionPlan plan{}; 

        using ElwiseWrapped = ArgContainerTup<true, typename Skeleton::ElwiseArgs>;  
        ElwiseWrapped elwiseArg;


        for (size_t i = 4; i <= MAXPOW; ++i) 
        {
            size_t current_size = baseTwoPower(i);
            std::cout << "Sample Size is " << 2 << " POW " << i << " " << current_size << std::endl;
            
            // ====== NEW! =====
            //context{ (sample_impl(std::get<EI>(elwiseArg), current_size), 0)... };
            //foreach::sample<EI...>(elwiseArg, current_size);

            ConditionalSampler<Skeleton> sampler{skeleton, current_size};
            sampler.sample(oi, ei, ci, ui);

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
                        std::get<OI>(sampler.resultArg)..., // sprider eftersom OI är en pack
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
        
        std::cout << "WRITING ########### "<< COMPILATIONID << std::endl;
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