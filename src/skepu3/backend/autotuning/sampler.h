#pragma once

#include <skepu3/backend/autotuning/helpers.h>
#include <skepu3/impl/common.hpp>
#include <skepu3/impl/meta_helpers.hpp>
#include <iostream>
#include <type_traits>
#include <tuple>
#include <random>

using namespace skepu;
using namespace skepu::backend;
using namespace autotuner::helpers;

namespace autotuner
{

    using context = std::initializer_list<int>;

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
        template<typename T>
        T random(size_t to) {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_real_distribution<float> dist6(1, (float)to); // distribution in range [1, 6]
            return static_cast<T>(dist6(rng));
        }

        template<typename T>
        void sample_impl(T& a, size_t size) {
            std::cout << "##### Fix sampling of uniform arguments #####" << std::endl;
            a = random<T>(size);// std::max<size_t>(2, 0.2*size);//size;
            std::cout << "Set to " << a << std::endl;
        }
        
        template<typename T>
        void sample_impl(skepu::Matrix<T>& mat, size_t size) {
            //std::cout << "Matrix Sample " << size << std::endl;
            mat.init(size, size, T{});
        }

        template<typename T>
        void sample_impl(skepu::Vector<T>& vec, size_t size) {
            //std::cout << "Vector Sample" << std::endl;
            vec.init(size, T{}); // Eller utan T{}
        }

        // Helper function to iterate and perform some function, Would not have been need if auto was available in lambda params  
        struct foreach
        {    
            template<size_t... COUNT, typename Tuple>
            static void sample(Tuple& tup, size_t size) 
            {
                context{ (sample_impl(std::get<COUNT>(tup), size), 0)... };
            }

            template<size_t... COUNT, typename Tuple>
            static void print(Tuple& tup) 
            {
                context{ (print_impl(std::get<COUNT>(tup)), 0)... };
            }

        };
        


        // ============= SAMPLER CLASS  ==============

        template<typename Skeleton>
        struct Sampler 
        {

            using ElwiseWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ElwiseArgs>::type;//ArgContainerTup<pm, typename Skeleton::ElwiseArgs>;
            using ResultWrapped    = typename containerized_layer<Skeleton, typename Skeleton::ResultArg>::type;//ArgContainerTup<pm, typename Skeleton::ResultArg>;
            using ContainerWrapped = typename containerized_layer<Skeleton, typename Skeleton::ContainerArgs>::type;//ArgContainerTup<pm, typename Skeleton::ContainerArgs>;
            using UniformWrapped   = typename Skeleton::UniformArgs; //typename containerized_layer<Skeleton, typename Skeleton::UniformArgs>::type;//ArgContainerTup<pm, typename Skeleton::UniformArgs>;

            static const size_t gpu_capacity = 24; // or compute this
            
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
            size_t size;

            MapOverlapSampler(Skeleton& skeleton, size_t size): skeleton{skeleton}, size{size} {}

            using Parent = Sampler<Skeleton>;

            using typename Parent::ResultWrapped;
            using typename Parent::ElwiseWrapped;
            using typename Parent::ContainerWrapped;
            using typename Parent::UniformWrapped; 

            ResultWrapped      resultArg;
            ElwiseWrapped      elwiseArg;
            ContainerWrapped   containerArg;
            UniformWrapped     uniArg; //std::tuple<size_t> 

            template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>
            void sample(pack_indices<OI...>, 
                        pack_indices<EI...>, 
                        pack_indices<CI...>, 
                        pack_indices<UI...>) 
            {

                size_t kernel = std::max<size_t>(2, 0.2*size);
                size_t padedSize = size + (kernel*2);
                skeleton.setOverlap(kernel, kernel); 
                
                //size_t& k = std::get<0>(uniArg); k=1;
            
                foreach::sample<OI...>(resultArg,    size);
                foreach::sample<EI...>(elwiseArg,    padedSize);
                foreach::sample<CI...>(containerArg, size);
                std::get<0>(uniArg) = 3; // this is a constant
                            
                // this->resultArg    = std::move(resultArg);
                // this->elwiseArg    = std::move(elwiseArg);
                // this->containerArg = std::move(containerArg);
            }
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
        template<typename Skeleton>
        struct ASampler: public Sampler<Skeleton> 
        {

            Skeleton& skeleton;
            size_t size;
            ASampler(Skeleton& skeleton, size_t size): skeleton{skeleton}, size{size} {}

            using Parent = Sampler<Skeleton>;

            using typename Parent::ResultWrapped;
            using typename Parent::ElwiseWrapped;
            using typename Parent::ContainerWrapped;
            using typename Parent::UniformWrapped; 


            ResultWrapped    resultArg;
            ElwiseWrapped    elwiseArg;
            ContainerWrapped containerArg;
            UniformWrapped   uniArg;

            template<size_t... OI, size_t... EI, size_t... CI, size_t... UI>  
            void sample(
                    pack_indices<OI...>, 
                    pack_indices<EI...>, 
                    pack_indices<CI...>, 
                    pack_indices<UI...>) 
            {
                foreach::sample<OI...>(resultArg,    size);
                foreach::sample<EI...>(elwiseArg,    size);
                foreach::sample<CI...>(containerArg, size);
                foreach::sample<UI...>(uniArg,       size);
            }

        };
    }
} // namespace autotuner
