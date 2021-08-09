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

// =========== DEBUG FUNCTIONS ==================
using context = std::initializer_list<int>;

void print(size_t elemnt) {
    std::cout << elemnt << " ";
}

template<size_t... all>
void arg_dim_printer(Dimensions<all...>) {
    context{ (print(all), 0)... };
    LOG(ERROR) <<  " NEXT " << std::endl;
}
// ===============================================


namespace skepu 
{
    namespace backend 
    {
        namespace autotune
        {   
            // ======================= Autotuning =====================
            template<typename Skeleton>
            using isMapOverlap = typename std::integral_constant<bool, Skeleton::skeletonType == SkeletonType::MapOverlap2D>;

            template<typename Skeleton, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
            ExecutionPlan sampling(Skeleton& skeleton, std::vector<BackendSpec>& specs, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<CI...> ci, pack_indices<UI...> ui) 
            {
                using SRT = ASampler<Skeleton>; // ConditionalSampler<Skeleton>;
                LOG(INFO) << "Skeleton sampling started" << std::endl;
                SampleRunner<
                    SRT, 
                    pack_indices<OI...>, 
                    pack_indices<EI...>, 
                    pack_indices<CI...>, 
                    pack_indices<UI...>> runner{SRT(skeleton)};
                        
                using Dims = typename ArgDimInit<Skeleton>::type; 
                auto plan = runner.template start<Dims>(specs);
                LOG(INFO) << "Sampling is DONE!" << std::endl;
                return plan;
            }

            template<typename Skeleton>
            ExecutionPlan samplingWrapper(Skeleton&  skeleton, std::vector<BackendSpec>& specs) {
                return sampling(
                    skeleton,
                    specs,
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