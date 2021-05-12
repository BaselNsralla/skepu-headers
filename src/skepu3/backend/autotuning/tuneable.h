#pragma once

#include "skepu3/backend/autotuning/auto-tuner.h"
#include <type_traits>
#include <random>
#include <string>

namespace autotuner {

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

    template<typename T>
    struct Tuneable 
    {
        /*
            TODO: Choose between Tuning methods here as a param to autotuning
        */
        string tuneId = std::to_string(Incremental<long long>::next()); 
        //static const long long tuneId = Incremental<long>::value;//::next(); 
        void autotuning() 
        {   
            // TODO: switch tuneType
            autotuner::samplingWrapper(*(static_cast<T*>(this)));
        }
    };
    
}