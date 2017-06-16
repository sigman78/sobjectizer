/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A template class for simplification of writting 
 * of proxy_dispatcher classes.
 *
 * \since
 * v.5.5.18
 */

#pragma once

#include <memory>
#include <string>
#include <utility>

#include <so_5/rt/stats/impl/h/activity_tracking.hpp>

#include <so_5/rt/impl/h/internal_env_iface.hpp>

namespace so_5 {

namespace disp {

namespace reuse {

/*!
 * \brief A helper template for simplification of implementation
 * of proxy dispatchers.
 *
 * Holds dispatchers params, base name for data sources and pointer
 * to actual dispatcher implementation. Actual dispatcher must be
 * created in start() method (this method must be implemented in
 * a derived class):
 * \code
using proxy_dispatcher_base_t =
		so_5::disp::reuse::proxy_dispatcher_template_t<
				actual_disp_iface_t,
				disp_params_t >;

class proxy_dispatcher_t : public proxy_dispatcher_base_t
	{
	public:
		proxy_dispatcher_t( disp_params_t params )
			:	proxy_dispatcher_base_t( std::move(params) )
			{}

		virtual void
		start( environment_t & env ) override
			{
				using dispatcher_no_activity_tracking_t =
						dispatcher_template_t< work_thread_no_activity_tracking_t >;

				using dispatcher_with_activity_tracking_t =
						dispatcher_template_t<
								work_thread_with_activity_tracking_t >;

				make_actual_dispatcher<
							dispatcher_no_activity_tracking_t,
							dispatcher_with_activity_tracking_t >(
						env,
						m_disp_params.thread_count(),
						m_disp_params.queue_params() );
			}
		...
	};

 * \endcode
 *
 * \tparam DISP_IFACE_TYPE type of actual dispatcher interface.
 * \tparam DISP_PARAMS_TYPE type of dispatcher parameters.
 *
 * \since
 * v.5.5.18
 */
template< typename DISP_IFACE_TYPE, typename DISP_PARAMS_TYPE >
class proxy_dispatcher_template_t : public DISP_IFACE_TYPE
	{
	public :
		proxy_dispatcher_template_t( DISP_PARAMS_TYPE disp_params )
			:	m_disp_params( std::move(disp_params) )
			{}

		virtual void
		start( so_5::environment_t & env ) override
			{
				this->modify_disp_params( env );
				this->do_actual_start( env );
			}

		virtual void
		shutdown() override
			{
				m_disp->shutdown();
			}

		virtual void
		wait() override
			{
				m_disp->wait();

				m_disp.reset();
			}

		virtual void
		set_data_sources_name_base(
			const std::string & name_base ) override
			{
				m_data_source_base_name = name_base;
			}

	protected :
		//! Actual dispatcher instance.
		std::unique_ptr< DISP_IFACE_TYPE > m_disp;

		//! Parameters for actual dispatcher.
		DISP_PARAMS_TYPE m_disp_params;

		//! Base name for data sources.
		std::string m_data_source_base_name;

		//! A method for actual start of real dispatcher.
		/*!
		 * A derived class must implement creation and launching
		 * of actual dispatcher inside this method.
		 */
		virtual void
		do_actual_start( so_5::environment_t & env ) = 0;

		//! Finish initialization of actual dispatcher and run it.
		/*!
		 * \tparam DISP_NO_TRACKING type of dispatcher without activity tracking.
		 * \tparam DISP_WITH_TRACKING type of dispatcher with activity tracking.
		 * \tparam ENV just a short alias for so_5::environment_t.
		 * \tparam ARGS a list of additional arguments for dispatcher's constructor.
		 */
		template<
			typename DISP_NO_TRACKING,
			typename DISP_WITH_TRACKING,
			typename ENV,
			typename... ARGS >
		void
		make_actual_dispatcher(
			ENV & env,
			ARGS && ...args )
			{
				using so_5::stats::activity_tracking_stuff::create_appropriate_disp;
				auto disp = create_appropriate_disp<
						DISP_IFACE_TYPE,
						DISP_NO_TRACKING,
						DISP_WITH_TRACKING >(
					env,
					m_disp_params,
					std::forward<ARGS>(args)... );

				disp->set_data_sources_name_base( m_data_source_base_name );
				disp->start( env );

				m_disp = std::move( disp );
			}

	private :
		void
		modify_disp_params( so_5::environment_t & env )
			{
				auto lf = m_disp_params.queue_params().lock_factory();
				if( !lf )
				{
					auto queue_params = m_disp_params.queue_params();
					queue_params.lock_factory(
							so_5::impl::default_lock_factory( env, lf ) );
					m_disp_params.set_queue_params( std::move(queue_params) );
				}
			}
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

