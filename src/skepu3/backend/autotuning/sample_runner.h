
#pragma once
#include <iostream>
#include <skepu3/impl/meta_helpers.hpp>
#include <skepu3/backend/autotuning/dimensional_sequence.h>
#include <skepu3/backend/benchmark.h>
#include <skepu3/impl/common.hpp>
#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/helpers.h>
#include <skepu3/backend/autotuning/sampler.h>

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

namespace autotuner
{
    template<typename SamplerType, typename Seq, typename... PackIndicies>
    struct SampleRunner {};

    template<typename SamplerType, typename... Is, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    struct SampleRunner<SamplerType, ssequence<Is...>, pack_indices<OI...>, pack_indices<EI...>, pack_indices<CI...>, pack_indices<UI...>> 
    {
        SamplerType sampler;
        SampleRunner(SamplerType&& sampler): sampler{std::forward<SamplerType>(sampler)} {}
            
        template<typename IndexSequence> //index_sequence<sizes...>
        void run() {
            
            std::pair<Backend::Type, benchmark::TimeSpan> bestDuration{Backend::Type::CPU, benchmark::TimeSpan::max()};
            sampler.sample(
                          IndexSequence   (), // TODOD FIXA DETTA
                          pack_indices<OI...>(), 
                          pack_indices<EI...>(), 
                          pack_indices<CI...>(), 
                          pack_indices<UI...>());

            //TODO Rest of sampling
        
        }
        
        void start() {

            size_t repeats = 5; 
            size_t size    = 0; // does not matter
           
            auto backendTypes = Backend::availableTypes();

            std::vector<BackendSpec> specs(backendTypes.size());

            std::transform(backendTypes.begin(), backendTypes.end(), specs.begin(), [](Backend::Type& type) {
                return BackendSpec(type);
            });

            /*
            FÖR VARJE I Is... kör run => run kör sample och benchmark och vi får resultat in i
            en plan variabel som skapas i konstruktorn här
            */
           context { (run<Is>(),0)... };


        }

    };
}

