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
//#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/helpers.h>
#include <skepu3/backend/autotuning/size.h>
//#include <skepu3/backend/autotuning/sampler.h>
//#include <skepu3/backend/autotuning/sample_runner.h>

#include <skepu3/backend/autotuning/tune_range.h>

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

            enum class Mechanism {
                ALL,
                SYMETRIC
            };


            template<typename InputType, typename OutputType> 
            struct ArgPermBase 
            {
                std::vector<InputType> input;
                std::vector<OutputType> permutation_sequence;
                //Mechanism constructor

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
                    //for (auto& startItem: first) // Impolementera Iterator för InputType
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
                    //for (auto& startItem: first) // Impolementera Iterator för ArgPerm
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
            std::vector<Size> sizes2D();

            template<>
            std::vector<Size> sizes2D<true>() {
                std::vector<Size> sizes;
                for (size_t size_x{2u}; size_x < MULTI_DIM; ++size_x)
                {
                    for (size_t size_y{0u}; size_y < MULTI_DIM; ++size_y)
                    {
                        sizes.push_back({size_x, size_y});
                    }   
                }
                return sizes;
            }

            template<>
            std::vector<Size> sizes2D<false>() {
                std::vector<Size> sizes;
                for (size_t size_x{2u}; size_x < MULTI_DIM; ++size_x)
                {
                    sizes.push_back({size_x, size_x});
                }
                return sizes;
            }


            std::vector<Size> sizes1D() {
                std::vector<Size> sizes;
                for (size_t size_x{2u}; size_x < SINGLE_DIM; ++size_x)
                {
                    sizes.push_back({size_x, 0});
                }
                return sizes;
            }

            // std::array istället?
            template<bool tuneFactor, typename T>
            std::vector<Size> combinations(T) 
            {
                return sizes1D();
            }

            // std::array istället?
            template<bool tuneFactor, typename T>
            std::vector<Size> combinations(skepu::MatCol<T>) 
            {
                return sizes2D<tuneFactor>();
            }

            // std::array istället?
            template<bool tuneFactor, typename T>
            std::vector<Size> combinations(skepu::MatRow<T>) 
            {
                return sizes2D<tuneFactor>();
            }

            // std::array istället?
            template<bool tuneFactor, typename T>
            std::vector<Size> combinations(skepu::Mat<T>) 
            {
                return sizes2D<tuneFactor>();
            }


            ArgPerm empty_arg_combinations() {
                ArgPerm argSize;
                argSize.add(std::vector<Size>( { Size{0u,0u} }) );
                argSize.permutations();
                return argSize;
            }

            ArgPerm arg_combinations(Ultra<tuple<>>, bool permutate) {
                return empty_arg_combinations();
            }

            ArgPerm arg_combinations(Standard<tuple<>>, bool permutate) {
                return empty_arg_combinations();
            }

            template<typename... Types>
            ArgPerm arg_combinations(Ultra<tuple<Types...>>, bool permutate) {
                //size_t size = sizeof...(Types);
                ArgPerm argSize;
                context { (argSize.add(combinations<true>(Types())), 0)... };
                argSize.permutations(permutate ? Mechanism::ALL : Mechanism::SYMETRIC);
                return argSize;
            };

            template<typename... Types>
            ArgPerm arg_combinations(Standard<tuple<Types...>>, bool permutate) {
                //size_t size = sizeof...(Types);
                ArgPerm argSize;
                context { (argSize.add(combinations<false>(Types())), 0)... };
                argSize.permutations(permutate ? Mechanism::ALL : Mechanism::SYMETRIC);
                return argSize;
            };

            


            // template<typename T>
            // void resolve_complexity(Ultra<T>) {
            //     arg_combinations<T>;...
            // }


            // template<typename T>
            // void resolve_complexity(Standard<T>) {
            //     arg_combinations<T>;...
            // }


            template<typename... T>
            ArgCatPerm resolve_form(Group<T...>) {
                ArgCatPerm perm;
                context { (perm.add(arg_combinations(T(), false)), 0)... }; //assert all are of same container (Standard)
                perm.permutations(Mechanism::SYMETRIC);
                for (auto& a: perm.permutation_sequence) {
                    std::cout << "{ ";
                    for(auto& b: a) { // Arg permutations
                        std::cout << " { ";
                        for(auto& c: b) {
                            std::cout << c.x << "|" << c.y << ", ";
                        }
                        std::cout << "}";
                    }
                    std::cout << " } \n";
                }


                return perm;
            }

            template<typename T>
            ArgCatPerm resolve_form(Single<T>) {
                ArgCatPerm perm;
                context { (perm.add(arg_combinations(T(), true)), 0) }; 
                perm.permutations();
                std::cout << "CATEGORY:: \n";
                
                // Det här make:ar sense
                for (auto& a: perm.input) {
                    std::cout << "{ ";
                    for(auto& b: a) { // Arg permutations
                        std::cout << " { ";
                        for(auto& c: b) {
                            std::cout << c.x << "|" << c.y << ", ";
                        }
                        std::cout << "}";
                    }
                    std::cout << " } \n";
                }

                return perm;
            }




            template<typename... T>
            ArgSequence configured_sequence(Permutation<T...>) {
                ArgSequence seq;
                context { (seq.add(resolve_form(T())), 0)... };
                seq.permutations();

                // for (auto& a: seq.input) { // En hel sample för varje element
                //     std::cout << "{" << std::endl;
                //     for (auto& b: a) { // En hel variabel Input, output, uniform och gänget 
                //         std::cout << "\t\t {" << std::endl;
                //         for (auto& c: b)  // Size?
                //         {   
                //             std::cout << "\t\t\t\t";
                //             for (auto& d: c) {
                //                 std::cout << d.x << ", ";

                //             }

                //         }
                //         std::cout << "\n\t\t }" << std::endl;
                //     } 
                //     std::cout << "} \n\n" << std::endl;
                // }


            
            
                return seq;
            }   


            template<typename Skeleton>
            ArgSequence generate_sequence(Skeleton& skeleton) {
                //Group<Standard<typename Skeleton::ResultArg>, Standard< typename Skeleton::ElwiseArgs>>,
                
                ArgSequence argSeq = configured_sequence(
                    Permutation<
                        //Single<Ultra<typename Skeleton::ResultArg>>,
                        //Single<Ultra<typename Skeleton::ElwiseArgs>>,
                        Group<Standard<typename Skeleton::ResultArg>, Standard< typename Skeleton::ElwiseArgs>>,
                        Single<Ultra<typename Skeleton::ContainerArgs>>,
                        Single<Ultra<typename Skeleton::UniformArgs>>
                    >()
                );

                std::ofstream ostrm("olllllkk.json", std::ios::trunc);
                ostrm << "DONE SAMPLING " << argSeq.input.size() << std::endl;
                for (auto& a: argSeq.samples) 
                { // En hel sample för varje element
                    ostrm << "{" << std::endl;
                    for (auto& b: a) 
                    { // En hel variabel Input, output, uniform och gänget 
                        ostrm << "    {" << std::endl;
                        ostrm<< "\t";
                        for (auto& c: b)  // Size?
                        {
                        ostrm << c.x <<  "·|·" << c.y << ", ";
                        }
                    ostrm << "\n    }" << std::endl;
                    } 
                    ostrm << "} \n" << std::endl;
                }
                return argSeq;
            }
        }


    }

}
