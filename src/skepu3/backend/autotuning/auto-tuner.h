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
#include <skepu3/backend/logging/logger.h>
#include <skepu3/backend/autotuning/arg_dimension.h>


using namespace skepu::backend::autotune::sampler;
using std::tuple;

// ==================
using context = std::initializer_list<int>;

void print(size_t elemnt) {
    std::cout << elemnt << " ";
}

template<size_t... all>
void arg_dim_printer(Dimensions<all...>) {
    context{ (print(all), 0)... };
    LOG(INFO) <<  " NEXT " << std::endl;
}
// ==================


namespace skepu 
{
    namespace backend 
    {
        namespace autotune
        {   

            
            
        
            // ======================= Autotuning =====================
            template<typename Skeleton>
            using isMapOverlap = typename std::integral_constant<bool, Skeleton::skeletonType == SkeletonType::MapOverlap2D>;

            template<typename Skeleton> // binary specialization, would need an extension to nonbinary
            using ConditionalSampler = typename std::conditional<isMapOverlap<Skeleton>::value, MapOverlapSampler<Skeleton>, ASampler<Skeleton>>::type;

            template<typename Skeleton, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
            ExecutionPlan sampling(Skeleton& skeleton, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<CI...> ci, pack_indices<UI...> ui) 
            {
                
                /*
                std::cout << "....." << std::endl;
                print_index<OI...>::print();
                print_index<EI...>::print();
                print_index<CI...>::print();
                print_index<UI...>::print();
                std::cout << "OI, EI, CI, UI" << std::endl;
                */

                using SRT = ConditionalSampler<Skeleton>;
                LOG(INFO) << "Skeleton sampling started" << std::endl;
                SampleRunner<
                    SRT, 
                    pack_indices<OI...>, 
                    pack_indices<EI...>, 
                    pack_indices<CI...>, 
                    pack_indices<UI...>> runner{SRT(skeleton)};
                        
                using Dims = typename ArgDimInit<Skeleton>::type; 
                auto plan = runner.template start<Dims>();
                LOG(INFO) << "Sampling is DONE!" << std::endl;

                // ========= NEW == Prints dimensions
                // using dims = typename ArgDimInit<Skeleton>::type; 
                // using ret  = typename dims::ret_dim;
                // using el   = typename dims::elwise_dim;
                // using cont = typename dims::cont_dim;
                // using uni  = typename dims::uni_dim;
                // arg_dim_printer(ret());
                // arg_dim_printer(el());
                // arg_dim_printer(cont());
                // arg_dim_printer(uni());
                // =========


                return plan;
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

        } // namespace autotune
    }
}

#endif