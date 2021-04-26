#pragma once

#include <utility>
#include <vector>
#include <skepu3/impl/backend.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>


// template<typename T>
// struct SerachFunctor 
// {   
//     //bool sameAsBefore = false; checks if in range find the position wher > is followed be < or vice versa

//     bool operator()(const T& a, const T& b) 
//     {
        
//     }
// };

/*
    There are two possible solution I have thought of,
        1 -> Map of type:vector<Ranges> and we binary search in each
        of the type,

        2 -> Vector of ranges
*/
namespace autotuner {
    
    using namespace skepu;
    using SizeRange     = std::pair<size_t, size_t>;
    using BackendRange  = std::pair<Backend::Type, SizeRange>;
    using BackendRanges = std::vector<BackendRange>;

   
    using std::stringstream;
    using std::ostream;
    using std::string;
    using std::istream;
    using std::getline;

    class ExecutionPlan
    {
        BackendRanges ranges;

public:
        void insert(Backend::Type type, size_t size)//SizeRange range) 
        {
            /* TODO: Make sure these come sorted
                - If we add something that is bigger than the previous range we fail
            */ 
            
            size_t rangeStart = ranges.size() > 0 ? ranges.back().second.second : 0;
            ranges.push_back({type, {rangeStart, size}}); 
        }

        // Backend::Type optimalBackend(SizeRange range) 
        // {} hitta lower och upper bound?

        Backend::Type optimalBackend(size_t targetSize)
        {
            // std::binary_search(ranges.begin(), ranges.end(), targetSize,
            // [](const int& target, const BackendRange& b) {
            //     SizeRange const& range = b.second; 
            //     // lowbound is priorized, upper bound could contain marginal errors.
            //     return range.first >= target && target < range.second;
            // });
            auto range_it = std::lower_bound(ranges.begin(), ranges.end(), targetSize, 
            [](const BackendRange& br, const int& target) {
                SizeRange const& range = br.second; 
                return range.second < target;
            });
            if (range_it == ranges.end()) {
                std::cout << "SKEPU ERROR" << std::endl;
                return Backend::Type::CPU;
            } else {
                return (*range_it).first;
            }
        }
        
        friend ExecutionPlan& operator>>(ExecutionPlan& executionPlan, istream& is);
        friend ostream&       operator<<(ostream& os, ExecutionPlan const& executionPlan);
    };

    //template<typename>
    
    std::string getJsonKeyLine(istream& is) 
    {
        char quote;
        is >> quote;
        std::string key;
        getline(is, key, is.widen('"'));
        return key;
    }

    template<typename T>
    T extract(istream& is, char start, char end) 
    {
        auto clear = [&] (char& delim) -> void {
            string dummy;
            getline(is, dummy, is.widen(std::move(delim)));
        };

        if(start != '\0') { clear(start); }

        T res;
        is >> res;
        
        if(end != '\0') { clear(end); }

        return res;
    }

    ExecutionPlan& operator>>(istream& is, ExecutionPlan& executionPlan) 
    {
        std::cout << "Hallow " << std::endl;
        string line;
        getline(is, line);
        while(getline(is, line))
        {
            stringstream ios(line);

            auto key = getJsonKeyLine(is);

            std::string linepart; 
            getline(ios, linepart, ios.widen(':'));
            
            if (ios.fail()) { break; }
            
            Backend::Type backendType = Backend::typeFromString(key);
            
            size_t rangeStart = extract<size_t>(ios, '[', ',');
            size_t rangeEnd   = extract<size_t>(ios, '\0', ']'); // specialize extractions based on the 
            
            getline(ios, linepart);
            std::cout << rangeStart << "<---> "<< rangeEnd << std::endl;
        }

        return executionPlan;
    }


    ostream& operator<<(ostream& os, ExecutionPlan const& executionPlan) 
    {

        os << "{" << '\n';
        for (BackendRange const& br: executionPlan.ranges)
        {
            auto rangeStart = br.second.first;
            auto rangeEnd   = br.second.second; 
            os << '"' << br.first << '"' << ": " << "[" << rangeStart << ", " << rangeEnd << "], \n";
        }
        os << "}\n";
        return os;
    } 

}
