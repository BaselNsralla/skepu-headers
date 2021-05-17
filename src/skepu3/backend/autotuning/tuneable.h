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

    template<typename Skeleton>
    struct Tuneable 
    {
        /*
            TODO: Choose between Tuning methods here as a param to autotuning
        */
        string tuneId   = std::to_string(Incremental<long long>::next());

    private:
        std::future<ExecutionPlan> future;
        bool active = false;
        std::atomic<bool> useTuning{false};
        std::thread::id tuningThreadId = std::this_thread::get_id();
        std::promise<autotuner::ExecutionPlan> promise;

        bool isTuningThread() {
            return std::this_thread::get_id() == tuningThreadId;
        }

    public:
        //static const long long tuneId = Incremental<long>::value;//::next(); 
        void autotuning(TuneExecutionPolicy policy = TuneExecutionPolicy::seq) 
        {   
            // TODO: switch tuneType
            active = true;
            Skeleton& skeleton = *(static_cast<Skeleton*>(this));
   
            if (policy == TuneExecutionPolicy::async) 
            {
                future = promise.get_future();
                // TODO: SET value on thread exit instead
                std::thread thread = std::thread( [&skeleton]{ skeleton.promise.set_value(samplingWrapper(skeleton)); skeleton.useTuning.store(true); });
                tuningThreadId = thread.get_id();
                thread.detach();
                // future = std::async(std::launch::async, [&]{ 
                //     autotuner::ExecutionPlan plan = samplingWrapper(skeleton); 
                //     skeleton.useTuning.store(true);
                //     return plan;
                // });
            } else 
            {
                future = promise.get_future();
                promise.set_value(samplingWrapper(skeleton));
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
            std::cout << "WILL WAIT "<< std::endl;
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