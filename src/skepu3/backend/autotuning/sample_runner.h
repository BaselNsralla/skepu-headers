#pragma once
#include <iostream>
#include <skepu3/impl/meta_helpers.hpp>
#include <skepu3/backend/autotuning/dimensional_sequence.h>
#include <skepu3/backend/benchmark.h>
#include <skepu3/impl/common.hpp>
#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/helpers.h>
#include <skepu3/backend/autotuning/sampler.h>
#include <skepu3/backend/autotuning/arg_sequence.h>
#include <skepu3/backend/logging/logger.h>
#include <limits>

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

namespace skepu
{
    namespace backend 
    {
        namespace autotune
        {
            using benchmark::TimeSpan;
            template<typename SamplerType, typename... PackIndicies>
            struct SampleRunner {};

            template<typename SamplerType, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
            struct SampleRunner <SamplerType, pack_indices<OI...>, pack_indices<EI...>, pack_indices<CI...>, pack_indices<UI...>>
            {
                SamplerType sampler;
                SampleRunner(SamplerType&& sampler): sampler{std::forward<SamplerType>(sampler)} {}

                void run(SampleVec& data, std::vector<BackendSpec>& specs, ExecutionPlan& plan) {
                    
                    std::pair<Backend::Type, TimeSpan> bestDuration{Backend::Type::CPU, TimeSpan::max()};
                    //LOG(DEBUG) << "Argument category size should be four: " << data.size() << std::endl;
                    auto args = sampler.sample(
                                data,
                                pack_indices<OI...>(), 
                                pack_indices<EI...>(), 
                                pack_indices<CI...>(), 
                                pack_indices<UI...>());

                    size_t repeats = 5; 
                    size_t size    = 0; // does not matter
                    //TODO Rest of sampling
                    benchmark::measureMedianForEachBackend(repeats, size, specs, 
                        [&](size_t, BackendSpec spec) {
                            LOG(INFO) << "Running on backend " << spec.getType() << std::endl;
                            sampler.skeleton.setBackend(spec);
                            sampler.skeleton(
                                std::get<OI>(args.resultArg)..., // sprider eftersom OI är en pack
                                std::get<EI>(args.elwiseArg)...,
                                std::get<CI>(args.containerArg)...,
                                std::get<UI>(args.uniArg)...);
                        },
                        [](benchmark::TimeSpan duration){
                            //std::cout << "Benchmark duration " << duration.count() << std::endl; 
                        },
                        [&]  (Backend::Type backend, TimeSpan duration) mutable {
                            if (duration.count() < bestDuration.second.count()) // && bestDuration.first != backend
                            {
                                bestDuration = {backend, duration};
                            }
                            skepu::containerutils::updateHostAndInvalidateDevice(
                                std::get<OI>(args.resultArg)...,
                                std::get<EI>(args.elwiseArg)...,
                                std::get<CI>(args.containerArg)...);
                        }); 

                    sampler.cleanup();
                    /// TODOD NEXT
                    //std::cout << "BEST BACKEND IS " << bestDuration.first << " FOR SIZE " << base2Pow(ResSize) << std::endl;
                    plan.insert(SizeModel(bestDuration.first, bestDuration.second.count(), data));
                }

                template<class ArgDimType>
                ExecutionPlan start(std::vector<BackendSpec>& specs) {                    
                    // TODO exist
                    ExecutionPlan planA{};
                    if(ExecutionPlan::exist(planA, STRINGIFY(COMPILATIONID), sampler.skeleton.tuneId))
                    {
                        return planA;
                    }
                    
                    ExecutionPlan plan {
                        STRINGIFY(COMPILATIONID),
                        Dimensionality {
                            ArgDimType::ret_dim::toVector(),
                            ArgDimType::elwise_dim::toVector(),
                            ArgDimType::cont_dim::toVector(),
                            ArgDimType::uni_dim::toVector()
                        }
                    };

                    //plan.setSearchDimension() Tror det är bättre med konstruktor då vi behöver den alltid för consistency
                    auto allSamples = generate_sequence(sampler.skeleton, 
                                                        typename ArgDimType::ret_dim(),
                                                        typename ArgDimType::elwise_dim(),
                                                        typename ArgDimType::cont_dim(),
                                                        typename ArgDimType::uni_dim()
                                                        ).samples;

                    //FÖR VARJE I Is... kör run => run kör sample och benchmark och vi får resultat in i
                    //en plan variabel som skapas i konstruktorn här
                    size_t i = 1;
                    for (auto& sample: allSamples) 
                    {   
                        LOG(INFO) << "Runinng a sample from generated samples " << "#" << i << std::endl;
                        run(sample, specs, plan);
                        ++i;
                    }
            
                    ExecutionPlan::persist(plan, sampler.skeleton.tuneId);
                    return plan;
                }
            };
        }
    }
}



