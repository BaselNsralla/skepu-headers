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

using namespace skepu;
using namespace skepu::backend;
using namespace autotuner::helpers;

namespace autotuner
{

    using context = std::initializer_list<int>;
 
    size_t base2Pow(size_t exp) { return std::pow<size_t>(size_t(2), exp); }
 
    namespace sample 
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
        /*
        template<typename T>
        T random(size_t to) {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_real_distribution<float> dist6(1, (float)to); // distribution in range [1, 6]
            return static_cast<T>(dist6(rng));
        }
        */
        template<size_t pos, typename T>
        void sample_impl(T& a, std::vector<Size> sizes) {
            std::cout << "##### Fix sampling of uniform arguments #####" << std::endl;
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
        //     template<size_t... COUNT, typename Tuple>
        //     static void sample(Tuple& tup, size_t size) 
        //     {
        //         context{ (sample_impl(std::get<COUNT>(tup), size), 0)... };
        //     }

            template<size_t... COUNT, typename Tuple>
            static void print(Tuple& tup) 
            {
                context{ (print_impl(std::get<COUNT>(tup)), 0)... };
            }

            template<size_t... COUNT, typename Tuple>
            static void sample(Tuple& tup, std::vector<Size> arg_vec) 
            {
                std::cout << " ONE ARGUMENT: " << std::endl;
                for (Size& size : arg_vec) {
                    std::cout << size.x << std::endl;
                }
                context{ (sample_impl<COUNT>(std::get<COUNT>(tup), arg_vec), 0)... };
            }



        };

        
        
        template<typename A, typename B, typename C, typename D>
        struct SampledArgs {
            A resultArg;
            B elwiseArg;
            C containerArg;
            D uniArg;

            // template<typename IndexSequence>
            // void apply()  {
            //     zot<
            //     typename normalized_sequence<IndexSequence, A, B, C, D>::type,
            //     A, B, C, D                
            //     >(a, b, c, d);
            // }
        };

        // ============= SAMPLER CLASS  ==============

        template<typename Skeleton>
        struct Sampler 
        {

            using ElwiseWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ElwiseArgs>::type;       //ArgContainerTup<pm, typename Skeleton::ElwiseArgs>;
            using ResultWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ResultArg>::type;        //ArgContainerTup<pm, typename Skeleton::ResultArg>;
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

        template<typename Skeleton>
        struct MapOverlapSampler: public Sampler<Skeleton>
        {
            // Typerna för variablerna och sample metoden bör abstraheras till flera using deklarationer
            Skeleton& skeleton;

            MapOverlapSampler(Skeleton& skeleton): skeleton{skeleton} {}

            using Parent = Sampler<Skeleton>;

            using typename Parent::ResultWrapped;
            using typename Parent::ElwiseWrapped;
            using typename Parent::ContainerWrapped;
            using typename Parent::UniformWrapped; 

            //ResultWrapped      resultArg;
            //ElwiseWrapped      elwiseArg;
            //ContainerWrapped   containerArg;
            //UniformWrapped     uniArg; //std::tuple<size_t> 
            /*
            template<size_t ResSize, size_t ElemSize, size_t ContSize, size_t UniSize, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
            SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> 
                     sample(
                        index_sequence<ResSize, ElemSize, ContSize, UniSize>,
                        pack_indices<OI...>, 
                        pack_indices<EI...>, 
                        pack_indices<CI...>, 
                        pack_indices<UI...>) 
            {
                SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> result;

                size_t outputSize = base2Pow(ResSize);
                size_t kernel     = std::max<size_t>(2, 0.2*outputSize);
                size_t padedSize  = outputSize + (kernel*2);
                skeleton.setOverlap(kernel, kernel); 
                
                //size_t& k = std::get<0>(uniArg); k=1;
            
                foreach::sample<OI...>(result.resultArg,    outputSize);
                foreach::sample<EI...>(result.elwiseArg,    padedSize);
                foreach::sample<CI...>(result.containerArg, base2Pow(ContSize));
                std::get<0>(result.uniArg) = 3; // this is a constant
                return result;
                // this->resultArg    = std::move(resultArg);
                // this->elwiseArg    = std::move(elwiseArg);
                // this->containerArg = std::move(containerArg);
            }
            */
            template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>
            SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> 
                sample(
                    std::vector<std::vector<Size>> sample_vec,
                    pack_indices<OI...>, 
                    pack_indices<EI...>, 
                    pack_indices<CI...>, 
                    pack_indices<UI...>) // Vi kan få sizes här och de kommer alltid ha samma ordning, så en index_sequence här
            {
                SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> result;

                size_t outputSize = base2Pow(sample_vec[0][0].x);
                size_t kernel     = std::max<size_t>(2, 0.2*outputSize);
                size_t padedSize  = outputSize + (kernel*2);
                skeleton.setOverlap(kernel, kernel); 

                // TODO: !!!!! Det här bör inte göras här
                std::vector<Size> output_sample_vec;
                std::generate_n(std::back_inserter(output_sample_vec), sample_vec[0].size(), [&outputSize]() { return outputSize; });

                std::vector<Size> elwise_sample_vec;
                std::generate_n(std::back_inserter(elwise_sample_vec), sample_vec[0].size(), [&padedSize]() { return padedSize; });

                foreach::sample<OI...>(result.resultArg,    output_sample_vec);
                foreach::sample<EI...>(result.elwiseArg,    elwise_sample_vec);
                foreach::sample<CI...>(result.containerArg, sample_vec[2]);
                foreach::sample<UI...>(result.uniArg,       sample_vec[3]);
                return result;


            }


        };


        // template<typename... Others, size_t... sizes>
        // struct consume {};

        // template<typename... A, size_t first, size_t... sizes>
        // struct consume<tuple<A...>, first, sizes...> // consume ko
        // {
        //     using type = typename std::conditional<sizeof...(A) == 0, index_sequence<sizes..., 0>, index_sequence<sizes..., first>>::type;
        // };

        // template<typename... A, typename... Others, size_t first, size_t... sizes>
        // struct consume<tuple<A...>, Others..., first, sizes...> 
        // {
        //     //sizeof...(A)
        //     using type = typename std::conditional<sizeof...(A) == 0, typename std::conditional<Others..., first, sizes..., 0>, typename std::conditional<Others..., sizes..., first>>::type::type  
        // };

        // template<typename Is, typename A, typename B, typename C, typename D>
        // struct normalize_sequence {};

        // template<size_t... sizes, typename A, typename B, typename C, typename D>
        // struct normailized_sequence<index_sequence<sizes...>, A, B, C, D>
        // {
        //     using type = consume<A, B, C, D, sizes...>::type; 
        // };

        // template<typename A, typename B, typename C, typename D>
        // struct SampledArgs {
        //     A a;
        //     B b;
        //     C c;
        //     D d;

            // template<typename IndexSequence>
            // void apply()  {
            //     zot<
            //     typename normalized_sequence<IndexSequence, A, B, C, D>::type,
            //     A, B, C, D                
            //     >(a, b, c, d);
            // }
        //};


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
        template<typename Skeleton>
        struct ASampler: public Sampler<Skeleton> 
        {

            Skeleton& skeleton;
            ASampler(Skeleton& skeleton): skeleton{skeleton} {}

            using Parent = Sampler<Skeleton>;

            using typename Parent::ResultWrapped;
            using typename Parent::ElwiseWrapped;
            using typename Parent::ContainerWrapped;
            using typename Parent::UniformWrapped; 


            // ResultWrapped    resultArg;
            // ElwiseWrapped    elwiseArg;
            // ContainerWrapped containerArg;
            // UniformWrapped   uniArg;

            // template<size_t ResSize, size_t ElemSize, size_t ContSize, size_t UniSize, size_t... OI, size_t... EI, size_t... CI, size_t... UI>  
            // SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> 
            //     sample(
            //         index_sequence<ResSize, ElemSize, ContSize, UniSize>,
            //         pack_indices<OI...>, 
            //         pack_indices<EI...>, 
            //         pack_indices<CI...>, 
            //         pack_indices<UI...>) // Vi kan få sizes här och de kommer alltid ha samma ordning, så en index_sequence här
            // {
            //     SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> result;
            //     foreach::sample<OI...>(result.resultArg,    base2Pow(ResSize));
            //     foreach::sample<EI...>(result.elwiseArg,    base2Pow(ElemSize));
            //     foreach::sample<CI...>(result.containerArg, base2Pow(ContSize));
            //     foreach::sample<UI...>(result.uniArg,       base2Pow(UniSize));
            //     return result;
            //     /*
            //         for each non empty type, but its object in the applied tuple
            //         send the content as a variadict thin and the index_sequence 
            //         to impl
            //         impl will apply sample foreach on each param together with each index in the sequence :  ..., ... <- or something like that

            //     */
            // }


            template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>
            SampledArgs<ResultWrapped, ElwiseWrapped, ContainerWrapped, UniformWrapped> 
                sample(
                    std::vector<std::vector<Size>> sample_vec,
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
                return result;


            }



        };
    }
} // namespace autotuner
