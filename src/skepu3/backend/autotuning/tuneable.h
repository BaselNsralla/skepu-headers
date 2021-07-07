#pragma once

#include "skepu3/backend/autotuning/auto-tuner.h"
#include "skepu3/backend/autotuning/execution_plan.h"
#include <type_traits>
#include <random>
#include <string>
#include <future>
#include <thread>
#include <skepu3/backend/logging/logger.h>
#include <skepu3/backend/autotuning/tune_limit.h>
#include <fstream>
#include <skepu3/backend/autotuning/custom_spec.h>
#include <skepu3/external/json.hpp>
using json = nlohmann::json;

namespace skepu 
{
    namespace backend 
    {
        namespace autotune 
        {
            enum class BackendSpecInputType {
                available, custom
            };

            struct BackendSpecInput {
                BackendSpecInput()  {}
                BackendSpecInput(std::string filePath): filePath{filePath}, type{BackendSpecInputType::custom} {}
                std::string filePath;
                BackendSpecInputType type = BackendSpecInputType::available;
            };

            enum class TuneExecutionPolicy 
            {
                async,
                seq
            };

            template<typename T, typename = std::enable_if<true, decltype(T{}+1)>>
            struct Incremental
            {
                static T next() 
                {
                    static T value = 0; 
                    return value++;
                }
                //static const T value = []() { std::uniform_int_distribution<int> dist; std::mt19937 mt((std::random_device()())); return dist(mt); }();
            };

            template<typename Skeleton>
            struct Tuneable 
            {
                /*
                    TODO: Choose between Tuning methods here as a param to autotuning
                */
                string tuneId   = std::to_string(Incremental<long long>::next());

            private:
                SampleLimit sample_quota;
                std::future<ExecutionPlan> future;
                bool active = false;
                std::atomic<bool> useTuning{false};
                std::thread::id tuningThreadId = std::this_thread::get_id();
                std::promise<autotune::ExecutionPlan> promise;

                bool isTuningThread() {
                    return std::this_thread::get_id() == tuningThreadId;
                }

                // TODO: Kan göra det här i strukten för BackendSpecInput
                std::vector<BackendSpec> resolveBackendSpecs(BackendSpecInput specInput) 
                {
                    if (specInput.type == BackendSpecInputType::custom)
                    {
                        std::ifstream file(specInput.filePath);
                        auto j = json::parse(file);
                        std::vector<CustomSpec> cspecs = j["backendSpecs"];
                        std::vector<BackendSpec> specs(cspecs.size());
                        std::transform(cspecs.begin(), cspecs.end(), specs.begin(), [](CustomSpec& customSpec) {
                            return CustomSpec::toBackendSpec(customSpec);
                        });
                        return specs;
                    } else {
                        auto backendTypes = Backend::availableTypes();
                        
                        // Custom specs?
                        std::vector<BackendSpec> specs(backendTypes.size());
                        std::transform(backendTypes.begin(), backendTypes.end(), specs.begin(), [](Backend::Type& type) {
                            return BackendSpec(type);
                        });
                        return specs;
                    }
                }

            public:
                SampleLimit tune_limit() 
                {
                    return sample_quota;
                }
                //static const long long tuneId = Incremental<long>::value;//::next(); 
                void autotuning(Quota quota = Quota::LOW, BackendSpecInput specInput = BackendSpecInput(), TuneExecutionPolicy policy = TuneExecutionPolicy::seq) 
                {   
                    // TODO: switch tuneType
                    sample_quota = SampleQuota(quota);
                    active = true;
                    auto specs = resolveBackendSpecs(specInput);
                    Skeleton& skeleton = *(static_cast<Skeleton*>(this));
        
                    if (policy == TuneExecutionPolicy::async) 
                    {
                        future = promise.get_future();
                        // TODO: SET value on thread exit instead
                        std::thread thread = std::thread( [&skeleton, &specs] { 
                            skeleton.promise.set_value(samplingWrapper(skeleton, specs)); 
                            skeleton.useTuning.store(true); 
                        });
                        
                        tuningThreadId = thread.get_id();
                        thread.detach();
                        // future = std::async(std::launch::async, [&]{ 
                        //     autotune::ExecutionPlan plan = samplingWrapper(skeleton); 
                        //     skeleton.useTuning.store(true);
                        //     return plan;
                        // });
                    } else 
                    {
                        future = promise.get_future();
                        promise.set_value(samplingWrapper(skeleton, specs));
                        skeleton.useTuning.store(true);
                    }
                }

                bool finalizeTuning() 
                {
                    Skeleton& skeleton = *(static_cast<Skeleton*>(this));
                    if (!skeleton.useTuning.load() && isTuningThread()) {
                        return false; // If tuning is not used return false; 
                    } else if (!future.valid()) 
                    {
                        return active; // true: It is active therefore future invalidity is because of being finished (makeAvailable was called before), or false which is not available. 
                    }
                    LOG(INFO) << "Generating execution plan..." << std::endl;
                    skeleton.setTuneExecPlan(new ExecutionPlan(std::move(future.get()))); // TOOD: transfer ownership instead.
                    return true;
                }

                // Calling this function should alway be after checking the availability, otherwise you get an invalid executionplan or null
                // ExecutionPlan& getPlan() {
                //     return plan;
                // }

                /*
                    It would have been nice to have the plan in the skeletonBase which is possible
                    but due to asynchrounous support it won't be as clean.
                    In case async solution does not yield great results, this will be moved to SkeletonBase
                */

                // const BackendSpec& selectTunedBackend(size_t size) 
                // {
                    
                //     T& skeleton = *(static_cast<T*>(this));
                //     if(!skeleton.useTuning.load() && finalizeTuning()) 
                //     {
                //         skeleton.setBackend(plan.optimalBackend(size));
                //     }
                //     //std::cout << "SIze " << size << std::endl;
                //     return skeleton.selectBackend(size);
                // }
                

                // virtual ~Tuneable() {
                //     delete plan;
                // }

            };
        }
    }
}
