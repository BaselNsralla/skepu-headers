
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
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

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

        Fixa så att ID:et sitter ordentligt och att EXecution plan skapas för varje skeleton
        - Filnamnet ska vara id:ochType eller ett id som genereas i klassen Map som sätts på 
          filen 
*/


using namespace skepu;
using namespace skepu::backend;
using namespace autotuner::helpers;
using namespace autotuner::sample;

namespace autotuner
{   
    
    // ======================= Autotuning ========================================================
    template<typename Skeleton>
    using isMapOverlap = typename std::integral_constant<bool, Skeleton::skeletonType == SkeletonType::MapOverlap2D>;

    template<typename Skeleton> // binary specialization, would need an extension to nonbinary
    using ConditionalSampler = typename std::conditional<isMapOverlap<Skeleton>::value, MapOverlapSampler<Skeleton>, ASampler<Skeleton>>::type;

    template<typename Skeleton, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    void sampling(Skeleton& skeleton, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<CI...> ci, pack_indices<UI...> ui) 
    {
        // ta ut varje parameter frå call args och använd dem i implementationen gör en benchmark och
        // spara undan, kanske låta resten av programmet köra medan vi håller på här?        
        std::cout << "============ " << STRINGIFY(COMPILATIONID) << std::endl;
        
        // ====================== ABSTRACT THIS ===========================
        ExecutionPlan plan{}; 
        std::ifstream sfile("/home/lized/Skrivbord/test/executionplan.json");
        if(sfile) {
            sfile >> plan;
            if (plan.id == STRINGIFY(COMPILATIONID)) {
                return;
            } else {
                plan.clear();
            }
        }
        plan.id = STRINGIFY(COMPILATIONID);
        // =================================================================


        //Skeleton::prefers_matrix;

        //PreferedContainer<pm, int> hi{};
        std::cout << "##### " << ConditionalSampler<Skeleton>::max_sample << std::endl;
        std::cout << Skeleton::prefers_matrix << " S " << std::endl;
        static constexpr size_t MAXSIZE = std::pow<size_t>(size_t(2), size_t(36));
        static constexpr size_t MAXPOW  = 10;//26; // TODO: 2^27-2^28 breaks my GPU :( TODO: borde sättas baserat på GPU capacity eller skeleton
        auto baseTwoPower = [](size_t exp) -> size_t { return std::pow<size_t>(size_t(2), exp); };

        using ElwiseWrapped = ArgContainerTup<true, typename Skeleton::ElwiseArgs>;  
        ElwiseWrapped elwiseArg;


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
                    if (bestDuration.second > duration) {
                        bestDuration = {backend, duration};
                    }
                });
            
            std::cout << "BEST BACKEND IS " << bestDuration.first << " FOR SIZE " << current_size << std::endl;
            plan.insert(bestDuration.first, current_size);
        }
        
        std::cout << "WRITING ########### "<< STRINGIFY(COMPILATIONID) << std::endl;
        std::ofstream file("/home/lized/Skrivbord/test/executionplan.json");
        if(file) {
            file << plan << std::flush;
            std::cout << "###########OK" << std::endl;
        }
    }

    template<typename Skeleton>
    void samplingWrapper(Skeleton&  skeleton) {
        sampling(
            skeleton,
            typename make_pack_indices<std::tuple_size<typename Skeleton::ResultArg>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ElwiseArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ContainerArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::UniformArgs>::value>::type()
        );
    }

} // namespace autotuner

#endif