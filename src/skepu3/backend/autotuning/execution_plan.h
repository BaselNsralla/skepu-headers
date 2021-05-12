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
        string filename;
public:
        
        static bool isReady(ExecutionPlan& plan, string compileId,  string tuneId);
        static void persist(ExecutionPlan& plan,  string tuneId);

        string id;

        void clear() {
            ranges.clear();
            id.clear();
        }

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

    
    /*
        Extracts the string in between `start` and `end`
    */
    template<typename T>
    T extract(istream& is, char start, char end) // specialize T is string?
    {
        auto extractUntill = [&] (char&& delim) -> string {
            string dummy;
            getline(is, dummy, is.widen(std::move(delim)));
            return dummy;
        };

        // Bypass what is before start if there is a start marker
        if(start != '\0') { extractUntill(std::move(start)); }

        T res;

        string linepart = extractUntill(end == '\0' ? '\n' : end);

        stringstream ios(linepart);
        ios >> res;
        return res;
    }

    ExecutionPlan& operator>>(istream& is, ExecutionPlan& executionPlan) 
    {
        BackendRanges ranges; 
        string line;

        getline(is, line); // {
        
        extract<string>(is, '"',':'); // id key
        auto id = extract<string>(is, '"', '"'); // "16ced-213-cdf8123"
        extract<string>(is, '\0', '\0'); // extract to the end

        while(getline(is, line))
        {
            stringstream ios(line);

            auto rangeStart = extract<size_t>(ios, '"',  ':'); // "123:33" => 123
            
            auto rangeEnd   = extract<size_t>(ios, '\0', '"');
            
            if (ios.fail()) { break; }
            
            std::string linepart; 
            getline(ios, linepart, ios.widen(':'));
            
            auto key = extract<std::string>(ios, '"', '"');

            getline(ios, linepart);

            ranges.emplace_back<Backend::Type, SizeRange>(Backend::typeFromString(key), {rangeStart, rangeEnd});
        }

        executionPlan.id     = std::move(id);
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
            << '"' << rangeStart << ':' << rangeEnd << '"' // "123:500"
            << ": " 
            << '"' << br.first << '"'
            << std::move(sep); 
        };
        
        if (!executionPlan.ranges.empty())
        {
            os << '"' << "id" << '"' << ':' << '"' << executionPlan.id << '"' << ',' << '\n';
            for (auto it = executionPlan.ranges.begin(); 
                it != executionPlan.ranges.end() - 1; 
                std::advance(it, 1))
            {
                output(*it, ",\n");
            }

            output(*(executionPlan.ranges.end() - 1), "\n");
        }

        os << "}\n";
        return os;
    } 

    bool ExecutionPlan::isReady(ExecutionPlan& plan, string compileId, string tuneId) 
    {
        string filename = compileId + tuneId; 
        std::ifstream sfile("/home/lized/Skrivbord/test/" + compileId + tuneId + ".json");
        if(sfile) {
            sfile >> plan;
            if (plan.id == compileId) 
            {
                return true;
            } else {
                plan.clear();
            }
        }
        plan.id = compileId;
        return false;     
    }

    void ExecutionPlan::persist(ExecutionPlan& plan, string tuneId) 
    {
        std::ofstream file("/home/lized/Skrivbord/test/" + plan.id + tuneId + ".json"); 
        if(file) 
        {
            file << plan << std::flush;
            std::cout << "###########OK" << std::endl;
        }
    }


}

