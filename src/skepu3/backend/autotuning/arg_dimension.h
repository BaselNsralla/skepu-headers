#pragma once

#include <skepu3/impl/meta_helpers.hpp>
#include <skepu3/impl/common.hpp>
#include <skepu3/backend/logging/logger.h>
#include <skepu3/impl/region.hpp>
#include <tuple>
using std::decay;
using std::tuple;

namespace skepu 
{
    template<typename T>
    static constexpr size_t  placeholder()
    { return 0; }

    template<typename T>
    struct dimension_impl 
    {
        static const size_t value = 1;
    };

    template<typename T>
    struct dimension_impl<skepu::Mat<T>> 
    {
        static const size_t value = 2;
    };

    template<typename T>
    struct dimension_impl<skepu::Vec<T>> 
    {
        
        static const size_t value = 1;
    };

    template<typename T>
    struct dimension_impl<skepu::MatCol<T>> 
    {
        static const size_t value = 2;
    };

    template< typename T>
    struct dimension_impl<skepu::MatRow<T>> 
    {
        static const size_t value = 2;
    };


    template< typename T>
    struct dimension_impl<skepu::Region1D<T>> 
    {
        static const size_t value = 1;
    };

    template< typename T>
    struct dimension_impl<skepu::Region2D<T>> 
    {
        static const size_t value = 2;
    };

    template<size_t... dims>
    struct Dimensions {
        static std::vector<size_t> toVector()
        {
            std::vector<size_t> result{dims...};
            return result;
            //context { (result.push_back(dims), 0)...};
        }

    };


    // template<typename A, typename B, typename C, typename D>
    // struct arg_dim_bound {};

    // template<typename A, typename B, typename C, typename D>
    // struct arg_dim_inferred {};

    // template<typename... Ret, typename... Elwise, typename... Cont, typename... Uni>
    // struct arg_dim_bound<tuple<Ret...>, tuple<Elwise...>, tuple<Cont...>, tuple<Uni...>>
    // {
    //     using ret_dim    = Dimensions<dimension_impl<typename std::decay<Elwise>::type>::value...>;
    //     using elwise_dim = Dimensions<dimension_impl<typename std::decay<Elwise>::type>::value...>;
    //     using cont_dim   = Dimensions<dimension_impl<typename std::decay<Cont>::type>::value...>;
    //     using uni_dim    = Dimensions<dimension_impl<typename std::decay<Uni>::type>::value...>;

    // };

    // template<typename... Ret, typename... Elwise, typename... Cont, typename... Uni>
    // struct arg_dim_inferred<tuple<Ret...>, tuple<Elwise...>, tuple<Cont...>, tuple<Uni...>>
    // {
    //     using ret_dim    = Dimensions<dimension<Ret>::value...>;
    //     using elwise_dim = Dimensions<dimension<Elwise>::value...>;
    //     using cont_dim   = Dimensions<dimension<Cont>::value...>;
    //     using uni_dim    = Dimensions<dimension<Uni>::value...>;

    // };
    
    template<typename T>
    struct deduced_dimensionality {};

    template<typename... T>
    struct deduced_dimensionality<tuple<T...>> {
        using type = Dimensions<dimension_impl<typename std::decay<T>::type>::value...>;
    };

    template<typename T, size_t val>
    struct defined_dimensionality {};


    template<typename... T, size_t Val>
    struct defined_dimensionality<tuple<T...>, Val> 
    {
        using type = Dimensions<(placeholder<T>(), Val)...>; 

    };
    // ===============================================
    template<typename T,  typename R, template<size_t...> class C = Dimensions> 
    struct unfold {};

    template<template<size_t ...> class OC, template<size_t ...> class IC, size_t... R, size_t... UE, template<size_t...> class C>
    struct unfold<OC<UE...>, IC<R...>, C>
    {
        using type = C<R..., UE...>;
    };

    // R is Primary, T is Secondary
    template<typename R, typename T>
    struct consume_dim {};

    // If Primary is empty we are done.
    template<typename T>
    struct consume_dim<tuple<>, T> 
    { 
        using type = Dimensions<>; 
    };

    // While Primary still has elements, deduce it's dimensions.
    template<typename R, typename... OR>
    struct consume_dim<tuple<R, OR...>, tuple<>> 
    { 
        using type = Dimensions<dimension_impl<typename decay<R>::type>::value, dimension_impl<typename decay<OR>::type>::value...>; 
    };

    // While Secondary (Master) has elements, control the dimensions of the primary.
    template<typename R, typename... OR, typename T, typename... OT>
    struct consume_dim<tuple<R, OR...>, tuple<T, OT...>> 
    {
        using type = typename unfold< typename consume_dim< tuple<OR...>, tuple<OT...> >::type, Dimensions<dimension_impl<typename decay<T>::type>::value > > ::type; 
    };


    // Master is second
    template<typename R, typename T>
    struct master_deduced_dimensionality {};


    template<typename... T, typename... R> // We make this specialization to throw error earlier.
    struct master_deduced_dimensionality<tuple<T...>, tuple<R...>> 
    {
        using type = typename consume_dim<tuple<T...>, tuple<R...>>::type; 

    };
    // ================================================

    template<typename Skeleton, SkeletonType skeletonType>
    struct ArgDim {
        using ret_dim    = typename deduced_dimensionality<typename Skeleton::ResultArg>::type; 
        using elwise_dim = typename deduced_dimensionality<typename Skeleton::ElwiseArgs>::type; 
        using cont_dim   = typename deduced_dimensionality<typename Skeleton::ContainerArgs>::type; 
        using uni_dim    = typename deduced_dimensionality<typename Skeleton::UniformArgs>::type; 
    };

    template<typename Skeleton>
    struct ArgDim<Skeleton, SkeletonType::MapOverlap2D> {
        using ret_dim    = typename defined_dimensionality<typename Skeleton::ResultArg,  2>::type; 
        using elwise_dim = typename defined_dimensionality<typename Skeleton::ElwiseArgs, 2>::type; 
        using cont_dim   = typename deduced_dimensionality<typename Skeleton::ContainerArgs>::type; 
        using uni_dim    = typename deduced_dimensionality<typename Skeleton::UniformArgs>::type; 
    };

    template<typename Skeleton>
    struct ArgDim<Skeleton, SkeletonType::Map> {
        using ret_dim    = typename master_deduced_dimensionality<
                                typename Skeleton::ResultArg, 
                                typename Skeleton::ElwiseArgs>::type; // TODO: This could make it slower  if elwise is bigger than return args
        using elwise_dim = typename deduced_dimensionality<typename Skeleton::ElwiseArgs>::type; 
        using cont_dim   = typename deduced_dimensionality<typename Skeleton::ContainerArgs>::type; 
        using uni_dim    = typename deduced_dimensionality<typename Skeleton::UniformArgs>::type; 
    };

    struct ArgDimReduce 
    {
        using ret_dim    = Dimensions<>;//typename master_deduced_dimensionality<typename Skeleton::ResultArg, typename Skeleton::ElwiseArgs>::type; // TODO: This could make it slower  if elwise is bigger than return args
        using elwise_dim = Dimensions<1>;//typename deduced_dimensionality<typename Skeleton::ElwiseArgs>::type; 
        using cont_dim   = Dimensions<>;//typename deduced_dimensionality<typename Skeleton::ContainerArgs>::type; 
        using uni_dim    = Dimensions<>;//typename deduced_dimensionality<typename Skeleton::UniformArgs>::type; 
    };

    template<typename Skeleton>
    struct ArgDim<Skeleton, SkeletonType::Reduce1D>: ArgDimReduce {};

    template<typename Skeleton>
    struct ArgDim<Skeleton, SkeletonType::Reduce2D>: ArgDimReduce {};

    template<typename Skeleton>
    struct ArgDimInit
    {
        //using type = Skeleton::skeletonType == SkeletonType::MapOverlap2D ? MapOverlap2DArg
        using type = ArgDim<Skeleton, Skeleton::skeletonType>;
    };

    struct Dimensionality 
    {
        using DimUnit = std::vector<size_t>;
        DimUnit ret;
        DimUnit elwise;
        DimUnit container;
        DimUnit uniform;
    };


};