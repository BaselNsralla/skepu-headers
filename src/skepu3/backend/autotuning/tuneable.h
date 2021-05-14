#pragma once

#include "skepu3/backend/autotuning/auto-tuner.h"
#include "skepu3/backend/autotuning/execution_plan.h"
#include <type_traits>
#include <random>
#include <string>
#include <future>
#include <thread>

namespace autotuner {

    enum class TuneExecutionPolicy {
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

    struct ExecutionPlanState {
        ExecutionPlan plan;
        bool isRead = false;
    };

    template<typename T>
    struct Tuneable 
    {
        /*
            TODO: Choose between Tuning methods here as a param to autotuning
        */
        string tuneId   = std::to_string(Incremental<long long>::next());

    private:
        std::future<ExecutionPlan> future;
        ExecutionPlan plan;
        bool active = false;
        std::atomic<bool> running{false};
        

    public:
        //static const long long tuneId = Incremental<long>::value;//::next(); 
        void autotuning(TuneExecutionPolicy policy = TuneExecutionPolicy::async) 
        {   
            // TODO: switch tuneType
            active = true;
            running.store(true);
            if (policy == TuneExecutionPolicy::async) 
            {
                future = std::async(std::launch::async, [&]{ 
                    auto plan = samplingWrapper(*(static_cast<T*>(this))); 
                    running.store(false);
                    return plan;
                });
            } else 
            {
                std::promise<ExecutionPlan> p;
                future = p.get_future();
                p.set_value(samplingWrapper(*(static_cast<T*>(this))));
                running.store(false);
            }

        }

        bool available() 
        {
            if (!future.valid()) 
            {
                return active; // true: It is active therefore future invalidity is because of being finished (makeAvailable was called before), or false which is not available. 
            }

            plan = std::move(future.get());
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
        const BackendSpec& selectTunedBackend(size_t size) 
        {
            
            T& skeleton = *(static_cast<T*>(this));
            if(!running.load() && available()) 
            {
                skeleton.setBackend(plan.optimalBackend(size));
            }
            //std::cout << "SIze " << size << std::endl;
            return skeleton.selectBackend(size);
        }
        

        // virtual ~Tuneable() {
        //     delete plan;
        // }

    };
    
}