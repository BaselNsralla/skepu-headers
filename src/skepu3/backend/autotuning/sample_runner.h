
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

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)



namespace autotuner
{

    //struct Size;

    template<typename SamplerType, typename... PackIndicies>
    struct SampleRunner {};

    template<typename SamplerType, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    struct SampleRunner <SamplerType, pack_indices<OI...>, pack_indices<EI...>, pack_indices<CI...>, pack_indices<UI...>>
    {
        SamplerType sampler;
        SampleRunner(SamplerType&& sampler): sampler{std::forward<SamplerType>(sampler)} {}


        //template<size_t ResSize, size_t ElemSize, size_t ContSize, size_t UniSize> //index_sequence<sizes...>
        void run(SampleVec& data, std::vector<BackendSpec>& specs, ExecutionPlan& plan) {
            
            std::pair<Backend::Type, benchmark::TimeSpan> bestDuration{Backend::Type::CPU, benchmark::TimeSpan::max()};
            
            auto args = sampler.sample(
                        data,
                        pack_indices<OI...>(), 
                        pack_indices<EI...>(), 
                        pack_indices<CI...>(), 
                        pack_indices<UI...>());

            size_t repeats = 5; 
            size_t size    = 0; // does not matter
            //TODO Rest of sampling
            benchmark::measureForEachBackend(repeats, size, specs, 
                [&](size_t, BackendSpec spec) {
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
                [&]  (Backend::Type backend, benchmark::TimeSpan duration) mutable {
                    //(std::cout << "Median " << backend << " Took " << duration.count() << std::endl;
                    if (duration < bestDuration.second )// && bestDuration.first != backend) 
                    {
                        std::cout << "Change from " << bestDuration.first << " To " << backend << std::endl;
                        bestDuration = {backend, duration};
                    }
                }); 

            
            /// TODOD NEXT
            //std::cout << "BEST BACKEND IS " << bestDuration.first << " FOR SIZE " << base2Pow(ResSize) << std::endl;
            //plan.insert(bestDuration.first,       base2Pow(ResSize)); // Execution plan parser
            plan.insert(SizeModel(bestDuration.first, bestDuration.second.count(), data));
        }


        ExecutionPlan start(std::vector<SampleVec>& allSamples) {
            auto backendTypes = Backend::availableTypes();

            std::vector<BackendSpec> specs(backendTypes.size());

            std::transform(backendTypes.begin(), backendTypes.end(), specs.begin(), [](Backend::Type& type) {
                return BackendSpec(type);
            });
            
            ExecutionPlan plan{};
            if(ExecutionPlan::isReady(plan, STRINGIFY(COMPILATIONID), sampler.skeleton.tuneId))
            {
                return plan;
            }
        


            //
            //FÖR VARJE I Is... kör run => run kör sample och benchmark och vi får resultat in i
            //en plan variabel som skapas i konstruktorn här
            //
            
            for (auto& sample: allSamples) 
            {
                run(sample, specs, plan);
            }
            


            std::cout << sampler.skeleton.tuneId << " ------ " << std::endl;
            ExecutionPlan::persist(plan, sampler.skeleton.tuneId);
            return plan;
        }
    };


    /*
    template<typename SamplerType, typename Seq, typename... PackIndicies>
    struct SampleRunner {};

    template<typename SamplerType, typename... Is, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    struct SampleRunner<SamplerType, ssequence<Is...>, pack_indices<OI...>, pack_indices<EI...>, pack_indices<CI...>, pack_indices<UI...>> 
    {
        SamplerType sampler;
        SampleRunner(SamplerType&& sampler): sampler{std::forward<SamplerType>(sampler)} {}
            

        template<size_t ResSize, size_t ElemSize, size_t ContSize, size_t UniSize> //index_sequence<sizes...>
        void run(index_sequence<ResSize, ElemSize, ContSize, UniSize>, std::vector<BackendSpec>& specs, ExecutionPlan& plan) {
            
            std::pair<Backend::Type, benchmark::TimeSpan> bestDuration{Backend::Type::CPU, benchmark::TimeSpan::max()};
            auto result = sampler.sample(
                          index_sequence<ResSize, ElemSize, ContSize, UniSize>  (), // TODOD FIXA DETTA
                          pack_indices<OI...>(), 
                          pack_indices<EI...>(), 
                          pack_indices<CI...>(), 
                          pack_indices<UI...>());

            size_t repeats = 5; 
            size_t size    = 0; // does not matter
            //TODO Rest of sampling
            benchmark::measureForEachBackend(repeats, size, specs, 
                [&](size_t, BackendSpec spec) {
                    sampler.skeleton.setBackend(spec);
                    sampler.skeleton(
                        std::get<OI>(result.resultArg)..., // sprider eftersom OI är en pack
                        std::get<EI>(result.elwiseArg)...,
                        std::get<CI>(result.containerArg)...,
                        std::get<UI>(result.uniArg)...);
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
            std::cout << "BEST BACKEND IS " << bestDuration.first << " FOR SIZE " << base2Pow(ResSize) << std::endl;
            plan.insert(bestDuration.first, base2Pow(ResSize)); // Execution plan parser
        }
        
        ExecutionPlan start() {
            auto backendTypes = Backend::availableTypes();

            std::vector<BackendSpec> specs(backendTypes.size());

            std::transform(backendTypes.begin(), backendTypes.end(), specs.begin(), [](Backend::Type& type) {
                return BackendSpec(type);
            });
            
            ExecutionPlan plan{};
            if(ExecutionPlan::isReady(plan, STRINGIFY(COMPILATIONID), sampler.skeleton.tuneId))
            {
                return plan;
            }
        


            //
            //FÖR VARJE I Is... kör run => run kör sample och benchmark och vi får resultat in i
            //en plan variabel som skapas i konstruktorn här
            //
            context { (run(Is(),specs, plan),0)... };
            std::cout << sampler.skeleton.tuneId << " ------ " << std::endl;
            ExecutionPlan::persist(plan, sampler.skeleton.tuneId);
            return plan;
        }

    };
    */

}

