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
    
    ExecutionPlan& operator>>(ExecutionPlan& executionPlan, istream& is) 
    {
        string line;
        getline(is, line);

        auto extract = [](istream& is, char&& delim) -> size_t { // should be templated to be used in all getlines in the while loop
            string dummy;
            getline(is, dummy, is.widen(std::move(delim)));
            size_t res;
            is >> res;
            return res;
        };

        while(getline(is, line))
        {
            stringstream ios(line);

            std::string linepart;
            getline(ios, linepart, ios.widen(':'));
            
            if (ios.fail()) { break; }
            
            Backend::Type backendType = Backend::typeFromString(linepart);
            
            int rangeStart = extract(ios, '[');
            int rangeEnd   = extract(ios, ',');
            
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
