/*! \file scan.h
 *  \brief Contains a class declaration for the skeleton base class.
 */

#ifndef SKELETON_BASE_H
#define SKELETON_BASE_H
#include <tuple>
#include <vector>
#include <skepu3/backend/environment.h>
#include <skepu3/backend/autotuning/execution_plan.h>
#include <skepu3/backend/autotuning/size.h>
#include <skepu3/backend/autotuning/dispatch_size.h>

namespace skepu
{
	namespace backend
	{
		class SkeletonBase
		{
		public:
			void finishAll()
			{
				this->m_environment->finishAll();
			}
			
			// Transfers ownership of ´plan´
			void setExecPlan(ExecPlan *plan)
			{
				// Clean up old plan
				if (this->m_execPlan != nullptr)
					delete this->m_execPlan;
				
				this->m_execPlan = plan;
			}

			void setTuneExecPlan(autotune::ExecutionPlan* plan) 
			{
				if (this->m_tunePlan != nullptr)
					delete this->m_tunePlan;
				this->m_tunePlan = plan;
			}

			
			void setBackend(BackendSpec const& spec)
			{
				this->m_user_spec = new BackendSpec(spec);
			}
			
			void resetBackend()
			{
				if (this->m_user_spec)
					delete this->m_user_spec;
				this->m_user_spec = nullptr;
			}
			
			const BackendSpec& selectBackend(size_t size = 0)
			{
				// if  (false) {//this->m_tunePlan) {
				// 	setBackend(this->m_tunePlan->optimalBackend(size));
				// 	this->m_selected_spec = this->m_user_spec;
				// }
				//else 
				if (this->m_user_spec)
					this->m_selected_spec = this->m_user_spec;
				else if (this->m_execPlan)
					this->m_selected_spec = &this->m_execPlan->find(size);
				else
					this->m_selected_spec = &internalGlobalBackendSpecAccessor();
				
			//	this->m_selected_spec = (this->m_user_spec != nullptr)
			//		? this->m_user_spec
			//		: &this->m_execPlan->find(size);
				return *this->m_selected_spec;
			}
			
			const BackendSpec& selectBackend(autotune::DispatchSize size)
			{

				if  (this->m_tunePlan) {//this->m_tunePlan) {
					setBackend(this->m_tunePlan->optimalBackend(size));
					this->m_selected_spec = this->m_user_spec;
				}
				else if (this->m_user_spec)
					this->m_selected_spec = this->m_user_spec;
				else if (this->m_execPlan)
					this->m_selected_spec = &this->m_execPlan->find(size.legacy);
				else
					this->m_selected_spec = &internalGlobalBackendSpecAccessor();
				
			//	this->m_selected_spec = (this->m_user_spec != nullptr)
			//		? this->m_user_spec
			//		: &this->m_execPlan->find(size);
				return *this->m_selected_spec;
				//return *this->m_selected_spec;
			}





		protected:
			SkeletonBase()
			{
				this->m_environment = Environment<int>::getInstance();
			/*
#if defined(SKEPU_OPENCL)
				BackendSpec bspec(Backend::Type::OpenCL);
				bspec.setDevices(this->m_environment->m_devices_CL.size());
				bspec.setGPUThreads(this->m_environment->m_devices_CL.at(0)->getMaxThreads());
				bspec.setGPUBlocks(this->m_environment->m_devices_CL.at(0)->getMaxBlocks());
			
#elif defined(SKEPU_CUDA)
				BackendSpec bspec(Backend::Type::CUDA);
				bspec.setDevices(this->m_environment->m_devices_CU.size());
				bspec.setGPUThreads(this->m_environment->m_devices_CU.at(0)->getMaxThreads());
				bspec.setGPUBlocks(this->m_environment->m_devices_CU.at(0)->getMaxBlocks());
			
#elif defined(SKEPU_OPENMP)
				BackendSpec bspec(Backend::Type::OpenMP);
				bspec.setCPUThreads(omp_get_max_threads());
				
#else
				BackendSpec bspec(Backend::Type::CPU);
#endif
				ExecPlan *plan = new ExecPlan();
				plan->setCalibrated();
				plan->add(1, MAX_SIZE, bspec);
				setExecPlan(plan);
			*/
			}
			
			Environment<int>* m_environment;
			
			/*! this is the pointer to execution plan that is active and should be used by implementations to check numOmpThreads and cudaBlocks etc. */
			ExecPlan *m_execPlan = nullptr;
			
			autotune::ExecutionPlan* m_tunePlan = nullptr;

			const BackendSpec *m_user_spec = nullptr;
			
			const BackendSpec *m_selected_spec = nullptr;
			
		}; // class SkeletonBase
		
	} // namespace backend
	
} // namespace skepu


#endif // SKELETON_BASE_H
