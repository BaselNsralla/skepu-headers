#pragma once
#include <cstddef>

namespace autotuner {
    struct Size 
    {
        size_t x;
        size_t y;

        bool equal(Size& other, size_t tolerance) {
            auto t_equal = [&] (size_t& a, size_t& b) {
                return (a == b) || ((a + tolerance) == b) || (a == (b+tolerance));
            };
             
            return t_equal(other.x, x) && t_equal(other.y, y); 
        }
    };
}