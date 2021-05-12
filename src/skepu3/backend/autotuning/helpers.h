
#pragma once
#include <skepu3/impl/common.hpp>
#include <skepu3/impl/meta_helpers.hpp>
#include <type_traits>
#include <tuple>

using namespace skepu;
using namespace skepu::backend;

namespace autotuner
{
    namespace helpers 
    {
        template<bool PrefersMatrix, typename TupleType>
        using ArgContainerTup = typename std::conditional<PrefersMatrix, 
                                        typename add_container_layer<skepu::template Matrix, TupleType>::type,
                                        typename add_container_layer<skepu::template Vector, TupleType>::type>::type;

        template<bool PrefersMatrix, typename T> 
        using PreferedContainer = typename std::conditional<PrefersMatrix, skepu::Matrix<T>, skepu::Vector<T>>::type;
        
        template<typename Skeleton, typename Element>
        struct Containerize 
        {
            static constexpr bool pm = Skeleton::skeletonType == SkeletonType::MapOverlap2D || Skeleton::prefers_matrix;
            using type = PreferedContainer<pm, Element>;
        };

        template<typename Skeleton, typename T>
        struct Containerize<Skeleton, skepu::Mat<T>> 
        {
            using type = skepu::Matrix<T>;
        };

        template<typename Skeleton, typename T>
        struct Containerize<Skeleton, skepu::Vec<T>> 
        {
            
            using type = skepu::Vector<T>;
        };

        template<typename Skeleton, typename T>
        struct Containerize<Skeleton, skepu::MatCol<T>> 
        {
            using type = skepu::Matrix<T>;
        };
        
        template<typename Skeleton, typename T>
        struct Containerize<Skeleton, skepu::MatRow<T>> 
        {
            using type = skepu::Matrix<T>;
        };

        template<typename Skeleton, typename T>
        struct containerized_layer
        {};

        template<typename Skeleton, typename... Types>
        struct containerized_layer<Skeleton, std::tuple<Types...>>
        {
            using type = std::tuple<typename Containerize<Skeleton,typename std::decay<Types>::type>::type...>;
        };

        template<size_t...>
        struct max {
            static const size_t value = 0;
        };

        template<size_t size, size_t...  sizes>
        struct max<size, sizes...> {
            using next = max<sizes...>; 
            static const size_t value = size >= next::value ? size : next::value;  
        };


        template<typename T>
        struct Dimension {
            static const size_t value = 0;
        };

        template<typename T>
        struct Dimension<skepu::Matrix<T>> {
            static const size_t value = 2;
        };

        template<typename T>
        struct Dimension<skepu::Vector<T>> {
            static const size_t value = 1;
        };

        //...

        template<typename Container>
        struct highest_dimension {}; //TODO: Just Crash it here

        template<typename... Types>
        struct highest_dimension<std::tuple<Types...>>
        {   
            static const size_t value = max<Dimension<typename std::decay<Types>::type>::value...>::value;
        };

    }
    
} // namespace autotuner
