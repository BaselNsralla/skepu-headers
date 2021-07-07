#pragma once
#include <cmath>
/*
#define MULTI_DIM     15u
#define SINGLE_DIM    18u
#define TUNE_CAPACITY 24u
*/
namespace skepu {


    enum class Quota {
        LOW,
        MEDIUM,
        HIGH
    };

    struct SampleLimit {
        size_t single_dim;
        size_t multi_dim;
        size_t tune_cap; // TODO: Not used
        // Dynamic Dimension
        size_t dynamic(size_t dim) {
            return std::pow(single_dim, 1/dim);
        }
    };

    static SampleLimit SampleQuota(Quota cap) 
    {
        switch(cap) {
            case Quota::LOW:
                return SampleLimit{7, 7, 24};
                //return SampleLimit{8, 6, 24};
                break;
            case Quota::MEDIUM:
                return SampleLimit{18, 15, 24};
                break;
            case Quota::HIGH:
                return SampleLimit{20, 17, 24};
                break;
        };
        return SampleLimit{15, 13, 24};
    }

};


//#define MULTI_DIM     std::size_t(15u)
//#define SINGLE_DIM    std::size_t(18u)
//#define TUNE_CAPACITY std::size_t(24u)