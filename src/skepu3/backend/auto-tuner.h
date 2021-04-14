#include <skepu3/impl/meta_helpers.hpp>
#include <tuple>
#include <iostream>
#include <type_traits>

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

    
    // template<size_t... Ns, typename std::enable_if<sizeof...(Ns) == 0>::type>
    // void print_them() {}

    // template<size_t a>
    // void print_them() {
    //     std::cout << a << std::endl;
    // }

    // template<size_t a, size_t... Ns, typename std::enable_if<sizeof...(Ns) != 0>::type>
    // void print_them() {
    //     std::cout << a << std::endl;
    //     print_them<Ns...>();
    // }

    template<bool PrefersMatrix, typename T> 
    using PreferedContainer = typename std::conditional<PrefersMatrix, skepu::Matrix<T>, skepu::Vector<T>>::type;



    template<typename Skeleton, size_t... OI, size_t... EI, size_t... AI, size_t... CI>
    void tune(Skeleton& skeleton, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<AI...> ai, pack_indices<CI...> ci) 
    {
        // ta ut varje parameter frå call args och använd dem i implementationen gör en benchmark och
        // spara undan, kanske låta resten av programmet köra medan vi håller på här?        
        std::cout << "============" << std::endl;
        // std::cout << OI... << std::endl;
        // std::cout << EI... << std::endl;
        // std::cout << AI... << std::endl;
        // std::cout << CI... << std::endl;
        //print_them<OI...>();
        //print_them<EI...>();
        //print_them<AI...>();
        //print_them<CI...>();
        PreferedContainer<Skeleton::prefers_matrix, int> hi{};
        
        print<EI...>::_print();
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
