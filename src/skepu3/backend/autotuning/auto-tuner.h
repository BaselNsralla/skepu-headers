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
        // ta ut varje parameter frå call args och använd dem i implementationen gör en benchmark och
        // spara undan, kanske låta resten av programmet köra medan vi håller på här?        
        //std::cout << "============ " << STRINGIFY(COMPILATIONID) << std::endl;
        
        auto argSeq = generate_sequence(skeleton);


       /*
        ExecutionPlan plan{};
        if(ExecutionPlan::isReady(plan, STRINGIFY(COMPILATIONID), skeleton.tuneId))
        {
            return plan;
        }
        */
        std::cout << " ..... " << std::endl;
        print_index<OI...>::print();
        print_index<EI...>::print();
        print_index<CI...>::print();
        print_index<UI...>::print();
        std::cout << "OI, EI, CI, UI" << std::endl;
        //return plan;


        //Skeleton::prefers_matrix;

        //PreferedContainer<pm, int> hi{};
        std::cout << "##### MAX SIZE: " << ConditionalSampler<Skeleton>::max_sample << std::endl;
        std::cout << "##### PREFERS MATRIX: " << Skeleton::prefers_matrix << std::endl;
        
        static constexpr size_t MAXPOW  = ConditionalSampler<Skeleton>::max_sample;//10;//26; // TODO: 2^27-2^28 breaks my GPU :( TODO: borde sättas baserat på GPU capacity eller skeleton
        static constexpr size_t MAXSIZE = std::pow<size_t>(size_t(2), size_t(MAXPOW));

        //auto baseTwoPower = [](size_t exp) -> size_t { return std::pow<size_t>(size_t(2), exp); };

        using ElwiseWrapped = ArgContainerTup<true, typename Skeleton::ElwiseArgs>;  
        ElwiseWrapped elwiseArg;

        using SRT = ConditionalSampler<Skeleton>;//Sampler<Skeleton>;
        static constexpr size_t start_size = SRT::max_sample;
        using RawSequence = typename setup_permuation_sequence<
                                            typename seq_wrapper<start_size, typename SRT::ResultWrapped>::type, 
                                            typename seq_wrapper<start_size, typename SRT::ElwiseWrapped>::type, 
                                            typename seq_wrapper<start_size, typename SRT::ContainerWrapped>::type, 
                                            typename seq_wrapper<start_size, typename SRT::UniformWrapped>::type
                                            >::type;

        using NormalizedSequence = typename normalize<RawSequence, typename SRT::ResultWrapped, typename SRT::ElwiseWrapped, typename SRT::ContainerWrapped, typename SRT::UniformWrapped>::type;

        SampleRunner<SRT, pack_indices<OI...>, pack_indices<EI...>, pack_indices<CI...>, pack_indices<UI...>> runner{SRT(skeleton)};
        
        //for(auto& sample: argSeq.samples) {
            
            
            
        //}

        
        
        
        
        
        return runner.start(argSeq.samples);

        //skapa srt i runner och låta srt sköta samplingen för en size? 
        /*
        for (size_t i = 4; i <= MAXPOW; ++i) 
        {
            size_t current_size = baseTwoPower(i);
            //std::cout << "Sample Size is " << 2 << " POW " << i << " " << current_size << std::endl;
            
            // ====== NEW! =====
            //context{ (sample_impl(std::get<EI>(elwiseArg), current_size), 0)... };
            //foreach::sample<EI...>(elwiseArg, current_size);

            ConditionalSampler<Skeleton> sampler{skeleton, current_size};
            sampler.sample(oi, ei, ci, ui);

            //print_all(elwiseArg, ei); // make sure to print something with a size if you want to se anything
            print_index<UI...>::print();

            size_t repeats = 5; 
            size_t size    = 0; // does not matter
           
            auto backendTypes = Backend::availableTypes();

            std::vector<BackendSpec> specs(backendTypes.size());

            std::transform(backendTypes.begin(), backendTypes.end(), specs.begin(), [](Backend::Type& type) {
                return BackendSpec(type);
            });

            std::pair<Backend::Type, benchmark::TimeSpan> bestDuration{Backend::Type::CPU, benchmark::TimeSpan::max()};

            benchmark::measureForEachBackend(repeats, size, specs, 
                [&](size_t, BackendSpec spec) {
                    skeleton.setBackend(spec);
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
                    if (bestDuration.second > duration) 
                    {
                        //std::cout << "Change from " << bestDuration.first << " To " << backend << std::endl;
                        bestDuration = {backend, duration};
                    }
                });
            
            std::cout << "BEST BACKEND IS " << bestDuration.first << " FOR SIZE " << current_size << std::endl;
            plan.insert(bestDuration.first, current_size);
        }
        std::cout << skeleton.tuneId << " ------ " << std::endl;
        ExecutionPlan::persist(plan, skeleton.tuneId);
        return plan;
        */

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