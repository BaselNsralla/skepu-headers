#ifndef AUTOTUNER_H
#define AUTOTUNER_H
#include <skepu3/impl/meta_helpers.hpp>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <skepu3/backend/benchmark.h>
#include <cmath>        // std::pow
#include <fstream>
#include <skepu3/impl/common.hpp>
#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/helpers.h>
#include <skepu3/backend/autotuning/sampler.h>
#include <skepu3/backend/autotuning/sample_runner.h>
#include <skepu3/backend/autotuning/arg_sequence.h>

//#include <skepu3/backend/map.h>
//#include <skepu3/backend/mapoverlap.h> Skapara problem

/*
    TODO: 
        Se till att prefered container är rätt
        Se till att alla skeletons funkar med autotuning
        Se till att kunna använda autotune som backend
        Kolla hur vi kan göra för ta bort och lägga till execution plan 
        Kolla hur vi kan göra för att justera max storlek på test [x]
        Kolla varför scan inte kör autotuning [x]
        Printa COMPILEID
            Få in COMPILEID i JSON filen [x]

        Flush to disk with id, send uuid from bash [x]

        Fixa så att ID:et sitter ordentligt och att EXecution plan skapas för varje skeleton
        - Filnamnet ska vara id:ochType eller ett id som genereas i klassen Map som sätts på 
          filen [x]

        Testa hoppa två två istället för att öka exponenten med 1
        Future<ExecutionPlan> option


    TODO:
        - Apply autotuning to all skeletons.
        - See if there are otherways to handle incremental sampling size.
        - Measure performance.
        - Clean up outputs,
        - Transfer ownership of Executionplan
        - Set on thread exit.
        - Cleanup
*/
using namespace skepu;
using namespace skepu::backend;
using namespace autotuner::helpers;
using namespace autotuner::sample;
using std::tuple;


namespace autotuner
{   
    
    // ======================= Autotuning ========================================================
    template<typename Skeleton>
    using isMapOverlap = typename std::integral_constant<bool, Skeleton::skeletonType == SkeletonType::MapOverlap2D>;

    template<typename Skeleton> // binary specialization, would need an extension to nonbinary
    using ConditionalSampler = typename std::conditional<isMapOverlap<Skeleton>::value, MapOverlapSampler<Skeleton>, ASampler<Skeleton>>::type;

    template<typename Skeleton, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    ExecutionPlan sampling(Skeleton& skeleton, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<CI...> ci, pack_indices<UI...> ui) 
    {
        auto argSeq = generate_sequence(skeleton);

        std::cout << "....." << std::endl;
        print_index<OI...>::print();
        print_index<EI...>::print();
        print_index<CI...>::print();
        print_index<UI...>::print();
        std::cout << "OI, EI, CI, UI" << std::endl;

        std::cout << "##### MAX SIZE: " << ConditionalSampler<Skeleton>::max_sample << std::endl;
        std::cout << "##### PREFERS MATRIX: " << Skeleton::prefers_matrix << std::endl;
        
        using SRT = ConditionalSampler<Skeleton>;//Sampler<Skeleton>;

        SampleRunner<
            SRT, 
            pack_indices<OI...>, 
            pack_indices<EI...>, 
            pack_indices<CI...>, 
            pack_indices<UI...>> runner{SRT(skeleton)};
                
        return runner.start(argSeq.samples);
    }

    template<typename Skeleton>
    ExecutionPlan samplingWrapper(Skeleton&  skeleton) {
        return sampling(
            skeleton,
            typename make_pack_indices<std::tuple_size<typename Skeleton::ResultArg>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ElwiseArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ContainerArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::UniformArgs>::value>::type()
        );
    }

} // namespace autotuner

#endif