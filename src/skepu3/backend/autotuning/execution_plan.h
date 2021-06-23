#pragma once

#include <utility>
#include <vector>
#include <skepu3/impl/backend.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>

#include <skepu3/external/json.hpp>
#include <skepu3/backend/autotuning/arg_sequence.h>
#include <skepu3/backend/dispatch_size.h>
// for convenience
using json = nlohmann::json;

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

    static constexpr size_t EQUALITY_TOLERANCE = 1;
   
    using std::stringstream;
    using std::ostream;
    using std::string;
    using std::istream;
    using std::getline;

    struct SizeModel 
    {
        Backend::Type backend;
        int64_t time_count;
        SampleVec sample;

        SizeModel() {}

        SizeModel(Backend::Type backend, int64_t time_count, SampleVec& sample) 
            : backend{backend}, time_count{time_count}, sample{sample}
        {}

        bool inbound(std::vector<Size>& a, std::vector<Size>& b) {
            bool equal = true;
            for (auto ita = a.begin(), itb = b.begin(); ita != a.end() && itb != b.end(); ++ita, ++itb)
            {
                // problem med olika längdeR?
                Size& a_size = *ita;
                Size& b_size = *itb;
                bool local_equal = a_size.equal(b_size, EQUALITY_TOLERANCE);
                equal = equal && local_equal;
  
            }
            //std::cout << " " << std::endl;
            return equal;
        }

        bool operator==(SampleVec& other) 
        {
            bool reql = inbound(other[0], sample[0]);
            bool eeql = inbound(other[1], sample[1]);
            bool ceql = inbound(other[2], sample[2]);
            bool ueql = inbound(other[3], sample[3]);
            return eeql && eeql && ceql && ueql;
        }

        bool operator==(DispatchSize& other) 
        {
            bool reql = inbound(other.outputSize, sample[0]);
            bool eeql = inbound(other.elwiseSize, sample[1]);
            bool ceql = inbound(other.containerSize, sample[2]);
            bool ueql = inbound(other.uniformSize, sample[3]);
            return eeql && eeql && ceql && ueql;
        }

    };

    using ModelVec = std::vector<SizeModel>;


    class ExecutionPlan
    {
        //BackendRanges ranges;
        ModelVec models;
        string filename;
        
public:
        static bool exist(ExecutionPlan& plan, string compileId,  string tuneId);
        static void persist(ExecutionPlan& plan,  string tuneId);

        string id;

        void clear() {
            models.clear();
            id.clear();
        }

        /*
         TODO: 
            Insertions Should be able to merge with the previous insert if they
            share the same optimal backend.  
        */
        void insert(SizeModel&& model)//SizeRange range) 
        {
            /* TODO: Make sure these come sorted
                - If we add something that is bigger than the previous range we fail
            */ 
            
            //size_t rangeStart = ranges.size() > 0 ? ranges.back().second.second : 0;
            models.push_back(std::move(model)); 
        }

        // Backend::Type optimalBackend(SizeRange range) 
        // {} hitta lower och upper bound?

        BackendSpec optimalBackend(DispatchSize& targetSize)
        {
            // NOTE: Dispatched size kommer ha samma vector size på varje argument.
            std::cout << "INSERTED DATA " << std::endl;
            for (auto& s: targetSize.outputSize) {
                std::cout << s.x << "|" << s.y << "  ,"; 
            }
            std::cout << std::endl;

            for (auto& s: targetSize.elwiseSize) {
                std::cout << s.x << "|" << s.y << "  ,"; 
            }
            std::cout << std::endl;

            for (auto& s: targetSize.containerSize) {
                std::cout << s.x << "|" << s.y << "  ,"; 
            }
            std::cout << std::endl;
            
            for (auto& s: targetSize.uniformSize) {
                std::cout << s.x << "|" << s.y << "  ,"; 
            }
            std::cout << std::endl;

            // std::binary_search(ranges.begin(), ranges.end(), targetSize,
            // [](const int& target, const BackendRange& b) {
            //     SizeRange const& range = b.second; 
            //     // lowbound is priorized, upper bound could contain marginal errors.
            //     return range.first >= target && target < range.second;
            // });
            /*
            auto range_it = std::lower_bound(ranges.begin(), ranges.end(), targetSize, 
            [](const BackendRange& br, const int& target) {
                SizeRange const& range = br.second; 
                return range.second < target;
            });
            if (range_it == ranges.end()) {
                std::cout << "SKEPU ERROR" << std::endl;
                return BackendSpec(Backend::Type::CPU);
            } else {
                return BackendSpec((*range_it).first);
            }
            */
            for (auto& model: models) 
            {
                if(model == targetSize) 
                {
                    std::cout << "########### FOUND IT ###########" << std::endl;
                    return BackendSpec(model.backend);
                }
            }
            return BackendSpec(Backend::Type::CPU);
        }
        
        friend ExecutionPlan& operator>>(istream& is, ExecutionPlan& executionPlan);
        friend ostream&       operator<<(ostream& os, ExecutionPlan const& executionPlan);
        friend void           to_json(json& j,        ExecutionPlan const& ep);
        friend void           from_json(json const& j, autotuner::ExecutionPlan& ep);
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
        // BackendRanges ranges; 
        // string line;

        // getline(is, line); // {
        
        // extract<string>(is, '"',':'); // id key
        // auto id = extract<string>(is, '"', '"'); // "16ced-213-cdf8123"
        // extract<string>(is, '\0', '\0'); // extract to the end

        // while(getline(is, line))
        // {
        //     stringstream ios(line);

        //     auto rangeStart = extract<size_t>(ios, '"',  ':'); // "123:33" => 123
            
        //     auto rangeEnd   = extract<size_t>(ios, '\0', '"');
            
        //     if (ios.fail()) { break; }
            
        //     std::string linepart; 
        //     getline(ios, linepart, ios.widen(':'));
            
        //     auto key = extract<std::string>(ios, '"', '"');

        //     getline(ios, linepart);

        //     ranges.emplace_back<Backend::Type, SizeRange>(Backend::typeFromString(key), {rangeStart, rangeEnd});
        // }

        // executionPlan.id     = std::move(id);
        // executionPlan.ranges = std::move(ranges);
        // std::cout << executionPlan << std::endl;
        json j;
        is >> j;
        from_json(j, executionPlan);
        //ExecutionPlan::persist(executionPlan, "4444444");
        return executionPlan;
    }


    ostream& operator<<(ostream& os, ExecutionPlan const& executionPlan) 
    {

        //os << "{" << '\n';

        // auto output = [&](BackendRange const& br, std::string&& sep) {
        //     auto rangeStart = br.second.first;
        //     auto rangeEnd   = br.second.second; 
        //     os 
        //     << '"' << rangeStart << ':' << rangeEnd << '"' // "123:500"
        //     << ": " 
        //     << '"' << br.first << '"'
        //     << std::move(sep); 
        // };
        
        // if (!executionPlan.ranges.empty())
        // {
        //     os << '"' << "id" << '"' << ':' << '"' << executionPlan.id << '"' << ',' << '\n';
        //     for (auto it = executionPlan.ranges.begin(); 
        //         it != executionPlan.ranges.end() - 1; 
        //         std::advance(it, 1))
        //     {
        //         output(*it, ",\n");
        //     }

        //     output(*(executionPlan.ranges.end() - 1), "\n");
        // }

        //os << "}\n";
        json j = executionPlan;
        os << std::setw(4) << j;
        return os;
    } 

    bool ExecutionPlan::exist(ExecutionPlan& plan, string compileId, string tuneId) 
    {
        string filename = compileId + tuneId; 
        std::ifstream sfile("/home/lized/Skrivbord/test/" + compileId + tuneId + ".json");
        
        if(sfile) 
        {
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
            std::cout << "########### FLUSH OK: " << tuneId << std::endl;
        }
    }

    void from_json(json const& j, SizeModel& sm)
    {   
        sm.backend       = Backend::typeFromString(j["backend"]);
        SampleVec sample = j["sample"];
        sm.sample        = std::move(sample); //j["sample"];
        sm.time_count    = j["time"];
        
    } 
    
    void from_json(json const& j, ExecutionPlan& ep)
    {
        ep.id     = j["compilationId"];
        std::vector<SizeModel> models;
        for (auto& modelJson : j["models"]) 
        {
            SizeModel model;
            from_json(modelJson, model);
            models.push_back(model);
        } 

        ep.models = std::move(models);
    } 

    void to_json(json& j,  SizeModel const& sm) 
    {
        std::ostringstream oss;
        oss << sm.backend;

        j = json{{"backend", oss.str()}, {"time", sm.time_count} ,{"sample", sm.sample}};
    }

    void to_json(json& j, ExecutionPlan const& ep) 
    {
        j = json{{"compilationId", ep.id}, {"models", ep.models}};
    }

}


