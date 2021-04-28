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
        /*
         TODO: 
            Insertions Should be able to merge with the previous insert if they
            share the same optimal backend.  
        */
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
        
        friend ExecutionPlan& operator>>(istream& is, ExecutionPlan& executionPlan);
        friend ostream&       operator<<(ostream& os, ExecutionPlan const& executionPlan);
    };

    
    template<typename T>
    T extract(istream& is, char start, char end) // specialize T is string?
    {
        auto step = [&] (char&& delim) -> void {
            string dummy;
            getline(is, dummy, is.widen(std::move(delim)));
        };

        if(start != '\0') { step(std::move(start)); }

        T res;

        std::string linepart;
        getline(is, linepart, is.widen(std::move(end == '\0' ? '\n' : end)));
        stringstream ios(linepart);
        ios >> res;
        return res;
    }

    ExecutionPlan& operator>>(istream& is, ExecutionPlan& executionPlan) 
    {
        BackendRanges ranges; 
        string line;
        getline(is, line);
        while(getline(is, line))
        {
            stringstream ios(line);
        
            auto rangeStart = extract<size_t>(ios, '"',  ':');
            auto rangeEnd   = extract<size_t>(ios, '\0', '"');

            if (ios.fail()) { break; }

            std::string linepart; 
            getline(ios, linepart, ios.widen(':'));
            
            auto key = extract<std::string>(ios, '"', '"');

            getline(ios, linepart);

            ranges.emplace_back<Backend::Type, SizeRange>(Backend::typeFromString(key), {rangeStart, rangeEnd});
        }

        executionPlan.ranges = std::move(ranges);
        std::cout << executionPlan << std::endl;

        return executionPlan;
    }


    ostream& operator<<(ostream& os, ExecutionPlan const& executionPlan) 
    {

        os << "{" << '\n';

        auto output = [&](BackendRange const& br, std::string&& sep) {
            auto rangeStart = br.second.first;
            auto rangeEnd   = br.second.second; 
            os 
            << '"' << rangeStart << ':' << rangeEnd << '"'
            << ": " 
            << '"' << br.first << '"'
            << std::move(sep); 
        };

        for (auto it = executionPlan.ranges.begin(); 
            it != executionPlan.ranges.end() - 1; 
            std::advance(it, 1))
        {
            output(*it, ",\n");
        }

        output(*(executionPlan.ranges.end() - 1), "\n");

        os << "}\n";
        return os;
    } 

}
