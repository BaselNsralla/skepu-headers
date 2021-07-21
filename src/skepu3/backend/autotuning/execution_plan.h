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
#include <skepu3/backend/autotuning/dispatch_size.h>
#include <skepu3/backend/logging/logger.h>
#include <skepu3/backend/autotuning/arg_dimension.h>
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

namespace skepu
{
	namespace backend 
	{

        class UniqueID {
        protected:
            static size_t nextID;
        public:
            int id;
            UniqueID() {
                id = ++nextID;
            };
            size_t get() {
                return id;
            }
        };

        size_t UniqueID::nextID = 0;

        namespace autotune
        {
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
                size_t id = UniqueID().get();

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
                        if (equal == false) {
                            break;
                        }
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
                    return reql && eeql && ceql && ueql;
                }

                bool operator==(DispatchSize& other) 
                {
                    bool reql = inbound(other.outputSize, sample[0]);
                    bool eeql = inbound(other.elwiseSize, sample[1]);
                    bool ceql = inbound(other.containerSize, sample[2]);
                    bool ueql = inbound(other.uniformSize, sample[3]);
                    return reql && eeql && ceql && ueql;
                }

            };


            using ModelVec = std::vector<SizeModel>;


            struct ExecutionPlan
            {
                //BackendRanges ranges;
//        public:
                string id;
                Dimensionality dims;
                ModelVec models;
                string filename;

                ExecutionPlan() {}
                ExecutionPlan(string id, Dimensionality dims) :  id{id}, dims{dims} {}

                static bool exist(ExecutionPlan& plan, string compileId,  string tuneId);
                static void persist(ExecutionPlan& plan,  string tuneId);


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

                // DispatchSize max(DispatchSize& ts, size_t maximum) 
                // {   
                //     std::vector<AT::Size> m_output(ts.outputSize.size());

                //     std::transform(ts.outputSize.begin(), ts.outputSize.end(), m_output.begin(), 
                //     [&maximum](AT::Size const& size) {
                //         return AT::Size{std::max(size.x, maximum), std::max(size.y, maximum)};
                //     });
                //     /*
                //     return DispatchSize { 
                //         std::max(ts.legacy, maximum),    
                //     }
                //     */
                // }

                BackendSpec* optimalBackend(DispatchSize& targetSize)
                {
                    // NOTE: Dispatched size kommer ha samma vector size på varje argument. Always right

                    targetSize.matchDimensions(dims);
                    // LOG(INFO) << "Finding a backend for the following dispatch size format:" << std::endl;
                    // for (auto& s: targetSize.outputSize) {
                    //     std::cout << s.x << "|" << s.y << "  ,"; 
                    // }
                    // std::cout << std::endl;

                    // for (auto& s: targetSize.elwiseSize) {
                    //     std::cout << s.x << "|" << s.y << "  ,"; 
                    // }
                    // std::cout << std::endl;

                    // for (auto& s: targetSize.containerSize) {
                    //     std::cout << s.x << "|" << s.y << "  ,"; 
                    // }
                    // std::cout << std::endl;
                    
                    // for (auto& s: targetSize.uniformSize) {
                    //     std::cout << s.x << "|" << s.y << "  ,"; 
                    // }
                    // std::cout << std::endl;
                    BackendSpec* backendSpec = nullptr;
                    auto timeTakenStep1 = skepu::benchmark::measureExecTime([&]
                    {

                        for (auto& model: models) 
                        {
                            if(model == targetSize) 
                            {
                                LOG(INFO) << "Backend " << model.backend << " with ID: " << model.id << " was found and will be used for the current dispatch size format!" << std::endl;
                                backendSpec = new BackendSpec(model.backend);
                                break;
                            }
                        }
                    });
                    
                    LOG(INFO) << "Time take to find a backend from execution plan " << (timeTakenStep1.count() / 10E6) << " Seconds" <<  std::endl;
                    
                    if (backendSpec == nullptr) 
                    {
                        LOG(WARN) << "Backend was not found for dispatch size format, will fallback to user backend!" << std::endl;
                    }    
                    
                    return backendSpec; //BackendSpec(Backend::Type::CPU);
                }
                
                friend ExecutionPlan& operator>>(istream& is,  ExecutionPlan& executionPlan);
                friend ostream&       operator<<(ostream& os,  ExecutionPlan const& executionPlan);
                friend void           to_json(json& j,         ExecutionPlan const& ep);
                friend void           from_json(json const& j, ExecutionPlan& ep);
            };

            
            ExecutionPlan& operator>>(istream& is, ExecutionPlan& executionPlan) 
            {
                json j;
                is >> j;
                from_json(j, executionPlan);
                //ExecutionPlan::persist(executionPlan, "4444444");
                return executionPlan;
            }


            ostream& operator<<(ostream& os, ExecutionPlan const& executionPlan) 
            {
                json j = executionPlan;
                os << std::setw(4) << j;
                return os;
            } 

            bool ExecutionPlan::exist(ExecutionPlan& plan, string compileId, string tuneId) 
            {
                string filename = compileId + tuneId; 
                std::ifstream sfile(compileId + tuneId + ".json");
                
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
                std::ofstream file(plan.id + tuneId + ".json"); 
                if(file) 
                {
                    file << plan << std::flush;
                    LOG(INFO) << "Sampling result flush to file " << "ID: " <<  tuneId << std::endl;
                }
            }

            void from_json(json const& j, SizeModel& sm)
            {   
                sm.backend       = Backend::typeFromString(j["backend"]);
                SampleVec sample = j["sample"];
                sm.sample        = std::move(sample); //j["sample"];
                sm.time_count    = j["time"];
                sm.id            = j["id"];
                
            } 

            void from_json(json const& j, Dimensionality& d)
            {
                //j = json{{"return", d.ret}, {"elementWise", d.elwise}, {"container", d.container}, {"uniform", d.uniform}};
                d.ret       = j["return"].get<typename Dimensionality::DimUnit>();
                d.elwise    = j["elementWise"].get<typename Dimensionality::DimUnit>();
                d.container = j["container"].get<typename Dimensionality::DimUnit>();
                d.uniform   = j["uniform"].get<typename Dimensionality::DimUnit>();
            }

            void from_json(json const& j, ExecutionPlan& ep)
            {
                ep.id     = j["compilationId"];
                Dimensionality dims;
                from_json(j["dimensions"], dims);
                //dims = j["dimensions"];//.get<Dimensionality>();
                ep.dims = std::move(dims);
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

                j = json{{"backend", oss.str()}, {"time", sm.time_count} , {"sample", sm.sample}, {"id", sm.id}};
            }

            void to_json(json& j, Dimensionality const& d)
            {
                j = json{{"return", d.ret}, {"elementWise", d.elwise}, {"container", d.container}, {"uniform", d.uniform}};
            }

            void to_json(json& j, ExecutionPlan const& ep) 
            {
                json jdims;
                 to_json(jdims, ep.dims);
                j = json{{"compilationId", ep.id}, {"dimensions", jdims }, {"models", ep.models}};
            }

        }
    }    
}