#pragma once
#include <skepu3/impl/meta_helpers.hpp>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <skepu3/backend/benchmark.h>
#include <cmath>        // std::pow
#include <fstream>
#include <skepu3/impl/common.hpp>

#include <skepu3/backend/autotuning/helpers.h>
#include <skepu3/backend/autotuning/size.h>

#include <skepu3/backend/logging/logger.h>
#include <skepu3/backend/autotuning/tune_limit.h>
#include <skepu3/impl/region.hpp>
#include <skepu3/backend/autotuning/arg_dimension.h>
namespace skepu 
{
    namespace backend 
    {
        namespace autotune
        {
            template<typename... T>
            struct Permutation {};

            template<typename... T>
            struct Group {};

            template<typename T>
            struct Single {};

            template<typename T>
            struct Ultra {};

            template<typename T>
            struct Standard {};



            using std::tuple; 
            using SampleVec   = std::vector<std::vector<Size>>;
            using ArgCombo    = std::vector<Size>;
            using ArgCatCombo = std::vector<ArgCombo>;

            using context = std::initializer_list<int>;

            enum class Mechanism 
            {
                ALL,
                SYMETRIC
            };


            template<typename InputType, typename OutputType> 
            struct ArgPermBase 
            {
                std::vector<InputType> input;
                std::vector<OutputType> permutation_sequence;
                // TODO: Mechanism constructor

                void add(InputType&& possibilites) 
                { // && och std::move
                    input.push_back(std::move(possibilites));
                }
                
                void permutations(Mechanism mechanism = Mechanism::ALL) 
                {
                    if (input.empty()) { 
                        std::cout << "SHOULD NOT HAPPEN PERM" << std::endl;
                        return;     
                    }
                    
                    auto first = input[0];
                    size_t i = 0;
                    for(auto it = first.begin(); it != first.end(); ++it)
                    {
                        auto& startItem = *it;
                        switch (mechanism)
                        {
                        case Mechanism::ALL:
                            apply({startItem}, std::next(input.begin()));
                            break;

                        case Mechanism::SYMETRIC:
                        {
                            bool zipped = zip({startItem}, std::next(input.begin()), i);
                            if (!zipped) { return; }
                            break;
                        }   
                        default:
                            break;
                        }
                        ++i;
                    }
                }

                decltype(permutation_sequence.begin()) 
                begin() { return permutation_sequence.begin();}

                decltype(permutation_sequence.end())
                end() { return permutation_sequence.end(); }

                size_t size() { return permutation_sequence.size(); }


                OutputType operator[](size_t i) 
                {
                    return permutation_sequence[i];
                }

            private:
                template<typename T>
                void apply(OutputType&& combo, T restIt) 
                {    
                    if (restIt == input.end()) 
                    {
                        
                        permutation_sequence.push_back(std::move(combo));
                        
                    } else {
                        for (auto& size: *restIt) 
                        {   
                            //combo.push_back(size);
                            OutputType nextComb = combo;
                            nextComb.push_back(size);
                            apply(std::move(nextComb), std::next(restIt));
                        }
                    }

                }

                template<typename T>
                bool zip(OutputType&& combo, T restIt, size_t i) 
                {
                    if (restIt != input.end()) 
                    {
                        for (auto it = restIt; it != input.end(); ++it)
                        {
                            if (i >= (*it).size())
                            {
                                return false;
                            } else 
                            {
                                combo.push_back((*it)[i]); //(*it).permutation_sequence[i]
                            }
                        }

                    }
                    permutation_sequence.push_back(combo);

                    return true;
                }
            };

            struct ArgPerm: ArgPermBase<std::vector<Size>, ArgCombo> // Inner 
            {};

            struct ArgCatPerm: ArgPermBase<ArgPerm, ArgCatCombo> // Outer
            {};

            struct ArgSequence // Outer Outer Inner
            {
                std::vector<ArgCatPerm> input;
                
                using ArgSequenceVector = std::vector<std::vector<Size>>;  
                //                             ^category   ^argument      

                std::vector<ArgSequenceVector> samples;
                //   ^category samples

                void add(ArgCatPerm&& i) {
                    input.push_back(std::move(i));
                }


                void permutations(Mechanism mechanism = Mechanism::ALL) 
                {
                    if (input.empty()) { 
                        // Case here TO HANDLE EMPTY
                        std::cout << "SHOULD NOT HAPPEN ARGSEQUENCE" << std::endl;
                        return;
                    }
                    
                    auto first = input[0];
                    size_t i = 0;
                    for(auto it = first.begin(); it != first.end(); ++it)
                    {
                        auto& startItem = *it;
                        
                        // Flatten =========
                        ArgSequenceVector asv;
                        for (auto& item: startItem) 
                        {
                            asv.push_back(item);
                        }
                        // ============
                        
                        switch (mechanism)
                        {            
                        case Mechanism::ALL:
                            apply(std::move(asv), std::next(input.begin()));
                            break;

                        case Mechanism::SYMETRIC:
                        {
                            bool zipped = zip(std::move(asv), std::next(input.begin()), i);
                            if (!zipped) { return; }
                            break;
                        }  
                        default:
                            break;
                        }
                        ++i;
                    }
                }


            private:
                template<typename T>
                void apply(ArgSequenceVector&& combo, T restIt) 
                {    
                    if (restIt == input.end()) 
                    {
                        //std::cout << "LOOPING " << std::endl;
                        samples.push_back(std::move(combo));
                    } else {
                        for (auto& sizes: *restIt) 
                        {   
                            ArgSequenceVector nextASV = combo;
                            // Flatten =========
                            for (auto& item: sizes) 
                            {
                                nextASV.push_back(item);
                            }
                            // ============
                            apply(std::move(nextASV), std::next(restIt));
                        }
                    }
                }

                template<typename T>
                bool zip(ArgSequenceVector&& combo, T restIt, size_t i) 
                {
                    if (restIt != input.end()) 
                    {
                        for (auto it = restIt; it != input.end(); ++it)
                        {
                            if (i >= (*it).size())
                            {
                                return false;
                            } else {
                                for (auto& item: (*it).permutation_sequence[i]) 
                                {
                                    combo.push_back(item);
                                }
                            }
                        }
                    }

                    samples.push_back(combo);
                    return true;
                }

            };

            template<bool permutate>
            std::vector<Size> sizes2D(SampleLimit limit);

            template<>
            std::vector<Size> sizes2D<true>(SampleLimit limit) {
                std::vector<Size> sizes;
                for (size_t size_x{2u}; size_x < limit.multi_dim; ++size_x)
                {
                    for (size_t size_y{0u}; size_y < limit.multi_dim; ++size_y)
                    {
                        sizes.push_back({size_x, size_y});
                    }   
                }
                return sizes;
            }

            template<>
            std::vector<Size> sizes2D<false>(SampleLimit limit) {
                std::vector<Size> sizes;
                for (size_t size_x{1u}; size_x < limit.multi_dim; ++size_x)
                {
                    sizes.push_back({size_x, size_x});
                }
                return sizes;
            }


            std::vector<Size> sizes1D(SampleLimit limit) {
                std::vector<Size> sizes;
                for (size_t size_x{1u}; size_x < limit.single_dim; ++size_x)
                {
                    sizes.push_back({size_x, 0});
                }
                return sizes;
            }

            template<bool tuneFactor, size_t dimensionality>
            std::vector<Size> combinations(SampleLimit limit) 
            {   
                switch (dimensionality)
                {
                case 1u:
                    return sizes1D(limit);
                    break;
                case 2u:
                    return sizes2D<tuneFactor>(limit);                
                    break;
                default:
                    return sizes1D(limit);
                    break;
                }
                 
            }

            ArgPerm empty_arg_combinations() {
                ArgPerm argSize;
                argSize.add(std::vector<Size>( { Size{0u,0u} }) );
                argSize.permutations();
                return argSize;
            }

            ArgPerm arg_combinations(SampleLimit limit, Ultra<Dimensions<>>, bool permutate) {
                return empty_arg_combinations();
            }

            ArgPerm arg_combinations(SampleLimit limit, Standard<Dimensions<>>, bool permutate) {
                return empty_arg_combinations();
            }

            template<size_t... Sizes>
            ArgPerm arg_combinations(SampleLimit limit, Ultra<Dimensions<Sizes...>>, bool permutate) {
                //size_t size = sizeof...(Types);
                ArgPerm argSize;
                context { (argSize.add(combinations<true, Sizes>(limit)), 0)... };
                argSize.permutations(permutate ? Mechanism::ALL : Mechanism::SYMETRIC);
                return argSize;
            };

            template<size_t... Sizes>
            ArgPerm arg_combinations(SampleLimit limit, Standard<Dimensions<Sizes...>>, bool permutate) {
                //size_t size = sizeof...(Types);
                ArgPerm argSize;
                context { (argSize.add(combinations<false, Sizes>(limit)), 0)... };
                argSize.permutations(permutate ? Mechanism::ALL : Mechanism::SYMETRIC);
                return argSize;
            };

            template<typename... T>
            ArgCatPerm resolve_form(SampleLimit limit, Group<T...>) {
                ArgCatPerm perm;
                context { (perm.add(arg_combinations(limit, T(), false)), 0)... }; //assert all are of same container (Standard)
                perm.permutations(Mechanism::SYMETRIC);
                return perm;
            }

            template<typename T>
            ArgCatPerm resolve_form(SampleLimit limit, Single<T>) {
                ArgCatPerm perm;
                context { (perm.add(arg_combinations(limit, T(), true)), 0) }; 
                perm.permutations();
                return perm;
            }

            template<typename... T>
            ArgSequence configured_sequence(SampleLimit limit, Permutation<T...>) {
                ArgSequence seq;
                context { (seq.add(resolve_form(limit, T())), 0)... };
                seq.permutations();
                return seq;
            }   
            
            template<SkeletonType type>
            struct generate_sequence_impl {
                
                template<size_t... ret, size_t... elwise, size_t... cont, size_t... uni>
                static ArgSequence generate( 
                                        SampleLimit limit,
                                        Dimensions<ret...>, 
                                        Dimensions<elwise...>,
                                        Dimensions<cont...>,
                                        Dimensions<uni...>) 
                {   
                    ArgSequence argSeq = configured_sequence(
                        limit,
                        Permutation<
                            Group<Standard<Dimensions<ret...>>, Standard<Dimensions<elwise...>>>,
                            Single<Ultra<Dimensions<cont...>>>,
                            Single<Ultra<Dimensions<uni...>>>
                        >()
                    );
                    return argSeq;
                }

                template<size_t... ret, size_t... cont, size_t... uni>
                static ArgSequence generate( 
                                        SampleLimit limit,
                                        Dimensions<ret...>, 
                                        Dimensions<>,
                                        Dimensions<cont...>,
                                        Dimensions<uni...>) 
                {   
                    ArgSequence argSeq = configured_sequence(
                        limit,
                        Permutation<
                            Single<Standard<Dimensions<ret...>>>,
                            Single<Standard<Dimensions<>>>,
                            Single<Ultra<Dimensions<cont...>>>,
                            Single<Ultra<Dimensions<uni...>>>
                        >()
                    );
                    return argSeq;
                }
            };

            struct reduce_sequence 
            {
                template<size_t... ret, size_t... elwise, size_t... cont, size_t... uni>
                static ArgSequence generate
                                        (
                                        SampleLimit limit, 
                                        Dimensions<ret...>, 
                                        Dimensions<elwise...>,
                                        Dimensions<cont...>,
                                        Dimensions<uni...>) 
                {   
                    ArgSequence argSeq = configured_sequence(
                        limit,
                        Permutation<
                            Single<Standard<Dimensions<ret...>>>,
                            Group<Standard<Dimensions<elwise...>>>,
                            Single<Ultra<Dimensions<cont...>>>,
                            Single<Ultra<Dimensions<uni...>>>
                        >()
                    );
                    return argSeq;
                }
            };

            template<>
            struct generate_sequence_impl<SkeletonType::Reduce1D> : reduce_sequence
            {};


            template<>
            struct generate_sequence_impl<SkeletonType::Reduce2D>: reduce_sequence 
            {};
            
            template<>
            struct generate_sequence_impl<SkeletonType::MapReduce>: reduce_sequence 
            {};



            template<typename Skeleton, size_t... ret, size_t... elwise, size_t... cont, size_t... uni>
            ArgSequence generate_sequence(Skeleton& skeleton, 
                                         Dimensions<ret...> r, 
                                         Dimensions<elwise...> e,
                                         Dimensions<cont...> c,
                                         Dimensions<uni...> u) {
                //Group<Standard<typename Skeleton::ResultArg>, Standard< typename Skeleton::ElwiseArgs>>,
                LOG(INFO) << "Generating all argument sequences..." << std::endl;
                ArgSequence argSeq = generate_sequence_impl<Skeleton::skeletonType>::generate(skeleton.tune_limit(), r, e, c, u);
                LOG(INFO) << "Sequence generation DONE!" << std::endl;
                return argSeq;
            }
        }


    }

}
