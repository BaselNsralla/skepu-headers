#pragma once

#include <skepu3/backend/autotuning/helpers.h>
#include <skepu3/impl/common.hpp>
#include <skepu3/impl/meta_helpers.hpp>
#include <iostream>
#include <type_traits>
#include <tuple>
#include <random>
#include <skepu3/backend/autotuning/dimensional_sequence.h>
#include <cmath>
#include <skepu3/backend/autotuning/arg_sequence.h>
#include <skepu3/backend/logging/logger.h>

using namespace skepu;
using namespace skepu::backend;
using namespace skepu::backend::autotune::helpers;

namespace skepu
{
    namespace backend 
    {
        namespace autotune
        {
            using context = std::initializer_list<int>;
        
            size_t base2Pow(size_t exp) { return std::pow<size_t>(size_t(2), exp); }
        
            namespace sampler 
            {

                // Print cvontext of Pack Index
                template<size_t...>
                struct print_index {    
                    static void print() {
                        std::cout << "END" << std::endl;
                    }
                };

                template<size_t t, size_t... things>
                struct print_index<t, things...> {    
                    static void print() {
                        std::cout << "Index Positions: " << t << ", ";
                        print_index<things...>::print();
                    }
                };

                // ============= Print Skepu structures ==================
                template<typename T>
                void print_impl(skepu::Matrix<T>& vec) {
                    std::cout << "Matrix Print" << std::endl;
                    for(auto& val: vec) {
                        std::cout << val << std::endl;
                    }
                }

                template<typename T>
                void print_impl(skepu::Vector<T>& vec) {
                    std::cout << "Vector Print" << std::endl;
                    for(auto& val: vec) {
                        std::cout << val << std::endl;
                    }
                }

                // ============= SAMPLE IMPLEMENTATION ===================
                template<size_t pos, typename T>
                void sample_impl(T& a, std::vector<Size> sizes) {
                    LOG(TODO) << "##### Fix sampling for uniform arguments #####" << std::endl;
                    a = base2Pow(sizes[pos].x);
                }
                
                template<size_t pos, typename T>
                void sample_impl(skepu::Matrix<T>& mat, std::vector<Size> sizes) {
                    //std::cout << "Matrix Sample " << size << std::endl;
                    mat.init(base2Pow(sizes[pos].x), base2Pow(sizes[pos].y), T{});
                }

                template<size_t pos, typename T>
                void sample_impl(skepu::Vector<T>& vec, std::vector<Size> sizes) {
                    //std::cout << "Vector Sample" << std::endl;
                    vec.init(base2Pow(sizes[pos].x), T{}); // Eller utan T{}
                }

                // Helper function to iterate and perform some function, Would not have been need if auto was available in lambda params  
                struct foreach
                {    
                    template<size_t... COUNT, typename Tuple>
                    static void print(Tuple& tup) 
                    {
                        context{ (print_impl(std::get<COUNT>(tup)), 0)... };
                    }

                    template<size_t... COUNT, typename Tuple>
                    static void sample(Tuple& tup, std::vector<Size> arg_vec) 
                    {
                        // NOTE: One Argument Category can consist of multiple elements COUNTs
                        context{ (sample_impl<COUNT>(std::get<COUNT>(tup), arg_vec), 0)... };
                    }
                };

                template<typename A, typename B, typename C, typename D>
                struct SampledArgs {
                    A resultArg;
                    B elwiseArg;
                    C containerArg;
                    D uniArg;
                };

                template<typename Skeleton >
                auto static_setup(Skeleton& skeleton) -> typename std::enable_if<Skeleton::skeletonType != SkeletonType::MapOverlap1D>::type 
                { 
                    
                }

                template<typename Skeleton >
                auto static_setup(Skeleton& skeleton) -> typename std::enable_if<Skeleton::skeletonType == SkeletonType::MapOverlap1D>::type 
                {
                    // switch (skeleton.skeletonType) 
                    // {
                    //     case skepu::SkeletonType::MapOverlap1D:
                            skeleton.setOverlap(2);
                    //         break;
                    //     default:
                    //         break;
                    // }
                }

                // ============= SAMPLER CLASS  ==============
                template<typename Skeleton>
                struct Sampler 
                {

                    using ResultWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ResultArg>::type;        //ArgContainerTup<pm, typename Skeleton::ResultArg>;
                    using ElwiseWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ElwiseArgs>::type;       //ArgContainerTup<pm, typename Skeleton::ElwiseArgs>;
                    using ContainerWrapped = typename containerized_layer<Skeleton, typename Skeleton::ContainerArgs>::type;    //ArgContainerTup<pm, typename Skeleton::ContainerArgs>;
                    using UniformWrapped   = typename Skeleton::UniformArgs;                                                    //typename containerized_layer<Skeleton, typename Skeleton::UniformArgs>::type;//ArgContainerTup<pm, typename Skeleton::UniformArgs>;

                    static const size_t gpu_capacity = 16; // or compute this
                    
                    /*
                    Vi behöver göra alla containers eftersom det varierar mellan alla skeletons och vissa har 0 i sig.
                    */
                    static const size_t max_sample   = 
                        gpu_capacity / max<
                                        highest_dimension<ContainerWrapped>::value,
                                        highest_dimension<ElwiseWrapped>::value,
                                        highest_dimension<ResultWrapped>::value
                                    >::value;
                };

                /*
                If the this will be created once in the tuner function then the 
                template function sample, can be replaced by a normal function
                and the template parameters can be included in the strcut template
                with nameless arguments in the constructor  

                The reason for the  sample being a template here is becasue the indicies will
                be very hard to handle outside the construtor, (i.e declaring the type only)
                because i don't really know if it can see different contigous spreads as different
                template parameters,

                Example:
                X<EI.., CI...> will this cause ambiguity?
                */
                template<typename Skeleton, SkeletonType = Skeleton::skeletonType>
                struct ASampler: public Sampler<Skeleton> 
                {

                    Skeleton& skeleton;
                    ASampler(Skeleton& skeleton): skeleton{skeleton} {}

                    using Parent = Sampler<Skeleton>;

                    using typename Parent::ResultWrapped;
                    using typename Parent::ElwiseWrapped;
                    using typename Parent::ContainerWrapped;
                    using typename Parent::UniformWrapped; 

                    void cleanup() {}

                    template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>
                    SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> 
                        sample(
                            std::vector<std::vector<Size>>& sample_vec,
                            pack_indices<OI...>, 
                            pack_indices<EI...>, 
                            pack_indices<CI...>, 
                            pack_indices<UI...>) // Vi kan få sizes här och de kommer alltid ha samma ordning, så en index_sequence här
                    {
                        SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> result;
                    
                        foreach::sample<OI...>(result.resultArg,    sample_vec[0]);
                    
                        foreach::sample<EI...>(result.elwiseArg,    sample_vec[1]);
                    
                        foreach::sample<CI...>(result.containerArg, sample_vec[2]);
                    
                        foreach::sample<UI...>(result.uniArg,       sample_vec[3]);

                        static_setup<Skeleton>(skeleton);

                        return result;
                    }
                };

                template<typename Skeleton>
                struct ASampler<Skeleton, SkeletonType::MapOverlap2D>: public Sampler<Skeleton>
                {
                    // Typerna för variablerna och sample metoden bör abstraheras till flera using deklarationer
                    Skeleton& skeleton;

                    ASampler(Skeleton& skeleton): skeleton{skeleton} {}

                    using Parent = Sampler<Skeleton>;

                    using typename Parent::ResultWrapped;
                    using typename Parent::ElwiseWrapped;
                    using typename Parent::ContainerWrapped;
                    using typename Parent::UniformWrapped; 

                    // NOTE: This is for MapOverlap Only
                    template<typename... Ts>
                    void setOverlap(Skeleton& skeleton, std::tuple<Ts...>& args) 
                    {
                        //if (sizeof...(Ts) == 0) 
                        //{
                            skeleton.setOverlap(1,1);
                            skeleton.setEdgeMode(skepu::Edge::Duplicate);
                        //} else {
                        //    skeleton.setOverlap(std::get<0>(args), std::get<0>(args));
                        //}
                    }

                    void setOverlap(Skeleton& skeleton, std::tuple<>& args)
                    {
                        skeleton.setOverlap(2,2);
                        skeleton.setEdgeMode(skepu::Edge::Duplicate); // NOTE!! kräver det ingen padding då?
                    }

                    void cleanup() {
                        skeleton.setEdgeMode(skepu::Edge::None);
                    }

                    template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>
                    SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> 
                        sample(
                            std::vector<std::vector<Size>>& sample_vec,
                            pack_indices<OI...>, 
                            pack_indices<EI...>, 
                            pack_indices<CI...>, 
                            pack_indices<UI...>) // Vi kan få sizes här och de kommer alltid ha samma ordning, så en index_sequence här
                    {
                        SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> result;
                        
                        size_t outputSizeX = base2Pow(sample_vec[0][0].x);
                        size_t outputSizeY = base2Pow(sample_vec[0][0].y);

                        size_t kernelX     = std::max<size_t>(2, 0.2*outputSizeX);
                        size_t kernelY     = std::max<size_t>(2, 0.2*outputSizeY);

                        size_t padedSizeX  = outputSizeX + (kernelX*2);
                        size_t padedSizeY  = outputSizeY + (kernelY*2);

                        // TODO: !!!!! Det här bör inte göras här
                        auto output_sample_size = sample_vec[0].size();
                        std::vector<autotune::Size> output_sample_vec(output_sample_size);
                        std::fill_n(output_sample_vec.begin(), output_sample_size, Size::Log2(outputSizeX, outputSizeY)); // TODO: Size rätt? eller behöver vi kolla typen


                        //std::generate_n(std::back_inserter(output_sample_vec), sample_vec[0].size(), [&outputSize]() { return outputSize; });
                        
                        auto elwise_sample_size = sample_vec[1].size();
                        std::vector<autotune::Size> elwise_sample_vec(elwise_sample_size);
                        std::fill_n(elwise_sample_vec.begin(), elwise_sample_size, Size::Log2(padedSizeX, padedSizeY));
                        //std::generate_n(std::back_inserter(elwise_sample_vec), sample_vec[1].size(), [&padedSize]() { return padedSize; });
                        foreach::sample<OI...>(result.resultArg,    sample_vec[0]);//output_sample_vec);
                        foreach::sample<EI...>(result.elwiseArg,    sample_vec[1]);//elwise_sample_vec);
                        foreach::sample<CI...>(result.containerArg, sample_vec[2]);
                        foreach::sample<UI...>(result.uniArg,       sample_vec[3]);

                        setOverlap(skeleton, result.uniArg);
                        return result;
                    }
                };

            }
        } // namespace autotune
    }
}
