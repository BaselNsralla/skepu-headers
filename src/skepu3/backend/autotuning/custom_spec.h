#pragma once
#include <skepu3/impl/backend.hpp>
#include <skepu3/external/json.hpp>
#ifdef SKEPU_OPENMP
#include <omp.h>
#endif
using json = nlohmann::json;

namespace skepu 
{
    namespace backend 
    {
        namespace autotune 
        {
            struct CustomSpec 
            {
                static BackendSpec toBackendSpec(CustomSpec& cspec)
                {
                    BackendSpec spec{cspec.backend};
                    spec.setCPUPartitionRatio(cspec.cpuPartitionRatio);
                    spec.setCPUChunkSize(cspec.openmpChunk);
                    spec.setCPUThreads(cspec.cpuThreads);
                    spec.setDevices(cspec.devices);
                    spec.setGPUBlocks(cspec.blocks);
                    spec.setGPUThreads(cspec.gpuThreads);
                    spec.setSchedulingMode(cspec.openmpScheduling);
                    return spec;
                } 

                Backend::Type backend;
                size_t devices    {BackendSpec::defaultNumDevices};
                size_t gpuThreads {BackendSpec::defaultGPUThreads};
                size_t blocks     {BackendSpec::defaultGPUBlocks};
                #ifdef SKEPU_OPENMP
                size_t cpuThreads = (size_t)omp_get_max_threads();
                #else
                size_t cpuThreads {BackendSpec::defaultCPUThreads};
                #endif
                size_t openmpChunk{Backend::chunkSizeDefault};
                Backend::Scheduling openmpScheduling = Backend::Scheduling::Static;
                float cpuPartitionRatio{BackendSpec::defaultCPUPartitionRatio};
            };

            // TODO: Borde sättas direkt till BacknedSpec

            void from_json(json const& j, CustomSpec& spec)
            {
                //std::string type;
                //j.at("backend").get_to(type);
                std::string type = j["backend"].get<std::string>();
                spec.backend = skepu::Backend::typeFromString(type);
                switch (spec.backend)
                {
                case Backend::Type::OpenCL:
                case Backend::Type::CUDA:
   
                    if(j.find("devices")    != j.end())  j["devices"].get_to(spec.devices);
                    if(j.find("gpuThreads") != j.end())  j["gpuThreads"].get_to(spec.gpuThreads);
                    if(j.find("blocks")     != j.end())  j["blocks"].get_to(spec.gpuThreads);
                    
                break;
                case Backend::Type::OpenMP: 
                {
                    if(j.find("openmpScheduling") != j.end())
                    {        
                        std::string type;
                        j.at("opempScheduling").get_to(type);
                        spec.openmpScheduling = Backend::schedulingTypeFromString(type);
                    } 
                }
                case Backend::Type::CPU:
                    //spec.cpuThreads  = j.find("cpuThreads") != j.end() ? j["cpuThreads"].get<size_t>()
                    if(j.find("cpuThreads") != j.end()) j.get_to(spec.cpuThreads);
                break;

                case Backend::Type::Hybrid:
                    if(j.find("cpuPartitionRatio") != j.end()) j.get_to(spec.cpuPartitionRatio);
                break;
                default:
                    break;
                }


                // spec.devices     = j.find("devices")    != j.end() ? j["devices"].get<size_t>()    : BackendSpec::defaultNumDevices;
                // spec.gpuThreads  = j.find("gpuThreads") != j.end() ? j["gpuThreads"].get<size_t>() : BackendSpec::defaultGPUThreads;
                // spec.blocks      = j.find("blocks")     != j.end() ? j["blocks"].get<size_t>()     : BackendSpec::defaultGPUBlocks;
                // if(j.find("cpuThreads") != j.end())
                // {
                //     spec.cpuThreads =  j["cpuThreads"].get<size_t>();
                // } else 
                // {
                //     #ifdef SKEPU_OPENMP
                //         size_t m_CPUThreads {(size_t)omp_get_max_threads()};
                //     #else
                //         size_t m_CPUThreads {BackendSpec::defaultCPUThreads};
                //     #endif
                // }

                // if(j.find("openmpScheduling") != j.end())
                // {        
                //     std::string type;
                //     j.at("opempScheduling").get_to(type);
                //     spec.openmpScheduling = Backend::schedulingTypeFromString(type);
                // } else 
                // {
                //     spec.openmpScheduling = Backend::Scheduling::Static;
                // }

                // spec.openmpChunk = j.find("openmpChunk") != j.end() ? j["openmpChunk"].get<size_t>() : Backend::chunkSizeDefault;

            }

        }

    }
}