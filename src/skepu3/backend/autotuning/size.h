#pragma once
#include <cstddef>
#include <skepu3/external/json.hpp>
#include <cmath>

using std::log2;

template<typename T>
size_t ulog2(T num) {
    return log2(num);
}

using json = nlohmann::json;

namespace autotuner {
    struct Size 
    {
        size_t x;
        size_t y;

        bool equal(Size& other, size_t tolerance)
        {
            auto t_equal = [&] (size_t& a, size_t& b) 
            {
                return (a == b) || ((a + tolerance) == b) || (a == (b+tolerance));
            };
             
            return t_equal(other.x, x) && t_equal(other.y, y); 
        }

        static Size Log2(size_t x, size_t y) 
        {
            return Size {ulog2(x), ulog2(y)}; 
        }
    };

    void to_json(json& j, const Size& size) 
    {
        j = json {{"x", size.x}, {"y", size.y}};
    }
    
    void from_json(json const& j, Size& size) 
    {
        size.x = j["x"];
        size.y = j["y"];
    }


}