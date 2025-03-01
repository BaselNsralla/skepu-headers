/*! \file map.h
 *  \brief Contains a class declaration for the Map skeleton.
 */

#ifndef MAP_H
#define MAP_H

#include "skepu3/backend/autotuning/tuneable.h"

using namespace autotune;
namespace skepu
{
	namespace backend
	{
		/*!
		 *  \ingroup skeletons
 		 */
		/*!
		 *  \class Map
		 *
		 *  \brief A class representing the Map skeleton.
		 *
		 *  This class defines the Map skeleton, a calculation pattern where a user function is applied to each element of an input
		 *  range. Once the Map object is instantiated, it is meant to be used as a function and therefore overloading
		 *  \p operator(). There are several overloaded versions of this operator that can be used depending on how many elements
		 *  the mapping function uses (one, two or three). There are also variants which takes iterators as inputs and those that
		 *  takes whole containers (vectors, matrices). The container variants are merely wrappers for the functions which takes iterators as parameters.
		 */
		template<size_t arity, typename MapFunc, typename CUDAKernel, typename CLKernel>
		class Map : public SkeletonBase, public Tuneable<Map<arity, MapFunc, CUDAKernel, CLKernel>>
		{
			// ==========================    Type definitions   ==========================
			using TuneableT = Tuneable<Map<arity, MapFunc, CUDAKernel, CLKernel>>;
			using T = typename MapFunc::Ret;
			using F = ConditionalIndexForwarder<MapFunc::indexed, decltype(&MapFunc::CPU)>;

			// ==========================     Class members     ==========================

			static constexpr size_t outArity = MapFunc::outArity;
			static constexpr size_t numArgs = MapFunc::totalArity - (MapFunc::indexed ? 1 : 0) + outArity;
			static constexpr size_t anyArity = std::tuple_size<typename MapFunc::ContainerArgs>::value;

			// ==========================    Instance members   ==========================

			CUDAKernel m_cuda_kernel;

		public:

			static constexpr auto skeletonType = SkeletonType::Map;
			using ResultArg = std::tuple<T>;
			using ElwiseArgs = typename MapFunc::ElwiseArgs;
			using ContainerArgs = typename MapFunc::ContainerArgs;
			using UniformArgs = typename MapFunc::UniformArgs;
			static constexpr bool prefers_matrix = MapFunc::prefersMatrix;

			static constexpr typename make_pack_indices<outArity, 0>::type out_indices{};
			static constexpr typename make_pack_indices<arity + outArity, outArity>::type elwise_indices{};
			static constexpr typename make_pack_indices<arity + anyArity + outArity, arity + outArity>::type any_indices{};
			static constexpr typename make_pack_indices<numArgs, arity + anyArity + outArity>::type const_indices{};

			Map(CUDAKernel kernel) : m_cuda_kernel(kernel)
			{
#ifdef SKEPU_OPENCL
				CLKernel::initialize();
#endif
			}

			template<typename... Args>
			void tune(Args&&... args)
			{
				tuner::tune(*this, std::forward<Args>(args)...);
			}

			// =======================      Call operators      ==========================
			template<typename... CallArgs>
			auto operator()(CallArgs&&... args) -> decltype(get<0>(args...))
			{
				static_assert(sizeof...(CallArgs) == numArgs, "Number of arguments not matching Map function");
				this->backendDispatch(get<0>(args...).size(), out_indices, elwise_indices, any_indices, const_indices, args...);
				return get<0>(args...);
			}


			template<typename Iterator, typename... CallArgs, REQUIRES(is_skepu_iterator<Iterator, T>::value)>
			Iterator operator()(Iterator res, Iterator res_end, CallArgs&&... args)
			{
				static_assert(sizeof...(CallArgs) == numArgs-1, "Number of arguments not matching Map function");
				this->backendDispatch(res_end - res, out_indices, elwise_indices, any_indices, const_indices, res, args...);
				return res;
			}

		private:

			// ==========================    Implementation     ==========================

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename... CallArgs>
			void CPU(size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);


#ifdef SKEPU_OPENMP

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename ...CallArgs>
			void OMP(size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

#endif // SKEPU_OPENMP


#ifdef SKEPU_CUDA

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename... CallArgs>
			void CUDA(size_t startIdx, size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename ...CallArgs>
			void         mapSingleThread_CU(size_t deviceID, size_t startIdx, size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename ...CallArgs>
			void          mapMultiStream_CU(size_t deviceID, size_t startIdx, size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename... CallArgs>
			void mapSingleThreadMultiGPU_CU(size_t useNumGPU, size_t startIdx, size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename ...CallArgs>
			void  mapMultiStreamMultiGPU_CU(size_t useNumGPU, size_t startIdx, size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

#endif // SKEPU_CUDA


#ifdef SKEPU_OPENCL

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename... CallArgs>
			void CL(size_t startIdx, size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename... CallArgs>
			void mapNumDevices_CL(size_t startIdx, size_t numDevices, size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

#endif // SKEPU_OPENCL

#ifdef SKEPU_HYBRID

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename... CallArgs>
			void Hybrid(size_t size, pack_indices<OI...>, pack_indices<EI...>, pack_indices<AI...>, pack_indices<CI...>, CallArgs&&... args);

#endif // SKEPU_HYBRID

			template<size_t... OI, size_t... EI, size_t... AI, size_t... CI, typename... CallArgs>
			void backendDispatch(size_t size, pack_indices<OI...> oi, pack_indices<EI...> ei, pack_indices<AI...> ai, pack_indices<CI...> ci, CallArgs&&... args)
			{
			//	assert(this->m_execPlan != nullptr && this->m_execPlan->isCalibrated());

				if (disjunction((get<OI>(args...).size() < size)...))
					SKEPU_ERROR("Non-matching output container sizes");

				if (disjunction((get<EI>(args...).size() < size)...))
					SKEPU_ERROR("Non-matching input container sizes");

				
				this->finalizeTuning();
				//this->selectBackend(size);
				this->selectBackend(
					DispatchSize::Create(
						TuneableT::tune_limit(),
						size,
						args_tuple<sizeof...(OI), CallArgs...>::template value<OI...>(args...),
						args_tuple<sizeof...(EI), CallArgs...>::template value<EI...>(args...),
						args_tuple<sizeof...(AI), CallArgs...>::template value<AI...>(args...),
						args_tuple<sizeof...(CI), CallArgs...>::template value<CI...>(args...)
					)
				);

				switch (this->m_selected_spec->activateBackend())
				{
				case Backend::Type::Hybrid:
#ifdef SKEPU_HYBRID
					this->Hybrid(size, oi, ei, ai, ci, get<OI, CallArgs...>(args...).begin()..., get<EI, CallArgs...>(args...).begin()..., get<AI, CallArgs...>(args...)..., get<CI, CallArgs...>(args...)...);
					break;
#endif
				case Backend::Type::CUDA:
#ifdef SKEPU_CUDA
					this->CUDA(0, size, oi, ei, ai, ci, get<OI, CallArgs...>(args...).begin()..., get<EI, CallArgs...>(args...).begin()..., get<AI, CallArgs...>(args...)..., get<CI, CallArgs...>(args...)...);
					break;
#endif
				case Backend::Type::OpenCL:
#ifdef SKEPU_OPENCL
					this->CL(0, size, oi, ei, ai, ci, get<OI, CallArgs...>(args...).begin()..., get<EI, CallArgs...>(args...).begin()..., get<AI, CallArgs...>(args...)..., get<CI, CallArgs...>(args...)...);
					break;
#endif
				case Backend::Type::OpenMP:
#ifdef SKEPU_OPENMP
					this->OMP(size, oi, ei, ai, ci, get<OI, CallArgs...>(args...).begin()..., get<EI, CallArgs...>(args...).begin()..., get<AI, CallArgs...>(args...)..., get<CI, CallArgs...>(args...)...);
					break;
#endif
				default:
					this->CPU(size, oi, ei, ai, ci, get<OI, CallArgs...>(args...).begin()..., get<EI, CallArgs...>(args...).begin()..., get<AI, CallArgs...>(args...)..., get<CI, CallArgs...>(args...)...);
					break;
				}
			}

		}; // class Map

	} // namespace backend

#ifdef SKEPU_MERCURIUM
template<int Arity, int GivenArity, typename Ret, typename... Args>
struct MapImpl : public SeqSkeletonBase
{
	template<
		template<class> class Container,
		typename ... CallArgs,
		REQUIRES_VALUE(is_skepu_container<Container<Ret>>)>
	Container<Ret> &
	operator()(Container<Ret> &res, CallArgs&&... args);

	template<
		typename Iterator,
		typename... CallArgs,
		REQUIRES_VALUE(is_skepu_iterator<Iterator, Ret>)>
	Iterator
	operator()(Iterator res, Iterator res_end, CallArgs&&... args);
};

template<int arity = SKEPU_UNSET_ARITY, typename Ret, typename... Args>
auto
Map(Ret(*)(Args...))
-> MapImpl<
		resolve_map_arity<arity, Args...>::value,
		arity,
		Ret,
		Args...>;

template<int arity = SKEPU_UNSET_ARITY, typename Ret, typename... Args>
auto
Map(std::function<Ret(Args...)>)
-> MapImpl<
		resolve_map_arity<arity, Args...>::value,
		arity,
		Ret,
		Args...>;

template<int arity = SKEPU_UNSET_ARITY, typename T>
auto constexpr
Map(T op)
-> decltype(Map<arity>(lambda_cast(op)));

#endif // SKEPU_MERCURIUM

} // namespace skepu


#include "impl/map/map_cpu.inl"
#include "impl/map/map_omp.inl"
#include "impl/map/map_cl.inl"
#include "impl/map/map_cu.inl"
#include "impl/map/map_hy.inl"

#endif // MAP_H
