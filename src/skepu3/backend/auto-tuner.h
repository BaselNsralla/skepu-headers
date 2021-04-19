#include <skepu3/impl/meta_helpers.hpp>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <algorithm>

namespace autotuner
{
    // borde lösas med struktar
    template<size_t...>
    struct print {    
        static void _print() {}
    };

    template<size_t t, size_t... things>
    struct print<t, things...> {    
        static void _print() {
            std::cout << "A size: " << t << std::endl;
            print<things...>::_print();
        }
    };

    template<typename T>
    void print_impl(skepu::Matrix<T>& vec) {
        std::cout << "Matrix Print" << std::endl;
    }

    template<typename T>
    void print_impl(skepu::Vector<T>& vec) {
        std::cout << "Vector Print" << std::endl;
        for(auto& val: vec) {
            std::cout << val << std::endl;
        }
    }



    template<typename T>
    void sample_impl(skepu::Matrix<T>& vec) {
        std::cout << "Matrix Sample" << std::endl;
    }

    template<typename T>
    void sample_impl(skepu::Vector<T>& vec) {
        std::cout << "Vector Sample" << std::endl;
        vec.init(300, T{}); // Eller utan T{}
    }
    
    template<size_t Index>
    struct foreach 
    {    
        // template<typename Tuple, typename... Extra>
        // static void apply(Tuple& tup, Extra&&... extras) {
        //     sample_impl(std::get<Index>(tup), std::forward<Extra>(extras)...);
        //     foreach<Index-1>::apply(tup, std::forward<Extra>(extras)...);
        // }
        template<typename Tuple>
        static void sample(Tuple& tup) {
            sample_impl(std::get<Index - 1>(tup));
            foreach<Index-1>::sample(tup);
        }

        template<typename Tuple>
        static void print(Tuple& tup) {
            print_impl(std::get<Index - 1>(tup));
            foreach<Index-1>::print(tup);
        }
    };

    template<>
    struct foreach<0>
    {
        // template<typename Tuple, typename... Extra>
        // static void apply(Tuple& tup, Extra&&... extras) {}
        template<typename Tuple>
        static void sample(Tuple& tup) {}
        template<typename Tuple>
        static void print(Tuple& tup) {}
    };
    

    template<typename Tuple, size_t... Idx>
    void sample_all(Tuple& tup, pack_indices<Idx...> ) {
        //size_t l = { (sample_impl(std::get<Idx>(tup)), 0)..., 0 };
        std::initializer_list<size_t> indicies = {Idx...};
        //auto m = std::max(indicies, [](size_t const& a, size_t const& b) { return a < b; });
        std::cout << std::tuple_size<Tuple>::value - 1 << std::endl;
        foreach< std::template tuple_size<Tuple>::value >::sample(tup);
    }

    template<typename Tuple, size_t... Idx>
    void print_all(Tuple& tup, pack_indices<Idx...> ) {
        //size_t l = { (sample_impl(std::get<Idx>(tup)), 0)..., 0 };
        std::initializer_list<size_t> indicies = {Idx...};
        //auto m = std::max(indicies, [](size_t const& a, size_t const& b) { return a < b; });
        std::cout << std::tuple_size<Tuple>::value - 1 << std::endl;
        foreach< std::template tuple_size<Tuple>::value >::print(tup);
    }

    template<bool PrefersMatrix, typename T> 
    using PreferedContainer = typename std::conditional<PrefersMatrix, skepu::Matrix<T>, skepu::Vector<T>>::type;

    template<bool PrefersMatrix, typename TupleType>
    using ArgContainerTup = typename std::conditional<PrefersMatrix, 
                                    typename add_container_layer<skepu::template Matrix, TupleType>::type,
                                    typename add_container_layer<skepu::template Vector, TupleType>::type>::type;

    template<typename Skeleton, size_t... OI, size_t... EI, size_t... CI, size_t... UI>
    void tune(Skeleton& skeleton, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<CI...> ci, pack_indices<UI...> ui) 
    {
        // ta ut varje parameter frå call args och använd dem i implementationen gör en benchmark och
        // spara undan, kanske låta resten av programmet köra medan vi håller på här?        
        std::cout << "============" << std::endl;
        //print_them<OI...>();
        //print_them<EI...>();
        //print_them<AI...>();
        //print_them<CI...>();
        static constexpr bool pm = Skeleton::prefers_matrix;

        PreferedContainer<pm, int> hi{};
        ArgContainerTup<pm, typename Skeleton::ResultArg> resultArg;
        ArgContainerTup<pm, typename Skeleton::ElwiseArgs> elwiseArg;
        ArgContainerTup<pm, typename Skeleton::ContainerArgs> containerArg;
        ArgContainerTup<pm, typename Skeleton::UniformArgs> uniArg;
        sample_all(resultArg, oi);
        sample_all(elwiseArg, ei);
        sample_all(containerArg, ci);
        sample_all(uniArg, ui);
        //sample_all(resultArg, oi);
        //sample_all(elwiseArg, ei);
        //sample_all(containerArg, ci);
        //sample_all(uniArg, ui);
        print<EI...>::_print();
        std::cout << "============" << std::endl;
    }

    template<typename Skeleton>
    void tuneWrapper(Skeleton&  skeleton) {
        tune(
            skeleton,
            typename make_pack_indices<std::tuple_size<typename Skeleton::ResultArg>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ElwiseArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::ContainerArgs>::value>::type(),
            typename make_pack_indices<std::tuple_size<typename Skeleton::UniformArgs>::value>::type()
        );
    }
} // namespace autotuner
