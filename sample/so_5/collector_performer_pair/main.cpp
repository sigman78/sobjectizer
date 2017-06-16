/*
 * A simple example of work load generation and simple form
 * of overload control by using collector+performer idiom.
 */

#include <iostream>
#include <random>
#include <deque>

#include <so_5/all.hpp>

// A helpers for trace messages.
std::mutex g_trace_mutex;
class locker_t
{
public :
	locker_t( std::mutex & m ) : m_lock( m ) {}
	locker_t( locker_t && o ) : m_lock( std::move( o.m_lock ) ) {}
	operator bool() const { return true; }
private :
	std::unique_lock< std::mutex > m_lock;
};

#define TRACE() \
if( auto l = locker_t{ g_trace_mutex } ) std::cout  

// Helper class with fatilities to random numbers generation.
class random_generator_mixin_t
{
public :
	random_generator_mixin_t()
	{
		m_random_engine.seed();
	}

	int
	random( int l, int h )
	{
		return std::uniform_int_distribution<>( l, h )( m_random_engine );
	}

private :
	// Engine for random values generation.
	std::default_random_engine m_random_engine;
};

// Message to be processed by worker agent.
struct application_request : public so_5::message_t
{
	std::string m_to;
	std::string m_from;
	std::string m_payload;
	std::string m_attributes;
	std::string m_generator;

	application_request(
		std::string to,
		std::string from,
		std::string payload,
		std::string attributes,
		std::string generator )
		:	m_to( std::move( to ) )
		,	m_from( std::move( from ) )
		,	m_payload( std::move( payload ) )
		,	m_attributes( std::move( attributes ) )
		,	m_generator( std::move( generator ) )
	{}
};

// Load generation agent.
class a_generator_t : public so_5::agent_t,
	private random_generator_mixin_t
{
public :
	a_generator_t(
		// Environment to work in.
		context_t ctx,
		// Name of generator.
		std::string name,
		// Workers.
		const std::vector< so_5::mbox_t > & workers_mboxes )
		:	so_5::agent_t( ctx )
		,	m_name( std::move( name ) )
		,	m_workers_mboxes( workers_mboxes )
	{}

	virtual void so_define_agent() override
	{
		// Just one handler in one state.
		so_default_state().event< msg_next_turn >(
				&a_generator_t::evt_next_turn );
	}

	virtual void so_evt_start() override
	{
		// Start work cycle.
		so_5::send< msg_next_turn >( *this );
	}

private :
	// Signal about start of the next turn.
	struct msg_next_turn : public so_5::signal_t {};

	// Generator name.
	const std::string m_name;
	// Workers.
	const std::vector< so_5::mbox_t > m_workers_mboxes;

	void evt_next_turn()
	{
		// How many requests will be sent on this turn.
		const int requests = random( 1, 100 );

		TRACE() << "GEN(" << m_name << ") turn started, requests="
				<< requests << std::endl;

		// We need copy of workers list to modify it if
		// some worker rejects our requests.
		std::vector< so_5::mbox_t > live_workers( m_workers_mboxes );
		int sent = 0;
		// If there is no active workers there is no need to continue.
		while( sent < requests && !live_workers.empty() )
		{
			if( generate_next_request( live_workers ) )
				++sent;
		}

		// How much to sleep until next turn.
		const auto next_turn_pause = std::chrono::milliseconds( random(0, 50) );

		TRACE() << "GEN(" << m_name << ") requests generated="
				<< sent << ", will sleep for "
				<< next_turn_pause.count() << "ms" << std::endl;

		so_5::send_delayed< msg_next_turn >( *this, next_turn_pause );
	}

	bool generate_next_request(
		std::vector< so_5::mbox_t > & workers )
	{
		auto it = workers.begin();
		if( workers.size() > 1 )
			// There are more then one live workers. Select one randomly.
			std::advance( it, random( 0, static_cast< int >(workers.size()) - 1 ) );

		// Prepare request.
		auto request = std::unique_ptr< application_request >(
				new application_request(
						"Mr.Alexander Graham Bell",
						"Mr.Thomas A. Watson",
						"Mr. Watson - Come here - I want to see you",
						"BestEffort,InMemory,NormalPriority",
						m_name ) );

		// Send it to worker.
		auto result = push_request_to_receiver( *it, std::move( request ) );
		if( !result )
			workers.erase( it );

		return result;
	}

	bool push_request_to_receiver(
		const so_5::mbox_t & to,
		std::unique_ptr< application_request > req )
	{
		// There is a plenty of room for any errors related to
		// synchronous invocation. Catch them all and treat them
		// as inabillity of worker to process request.
		try
		{
			return to->get_one< bool >()
					.wait_for( std::chrono::milliseconds( 10 ) )
					.sync_get( std::move( req ) );
		}
		catch( const std::exception & x )
		{
			TRACE()<< "GEN(" << m_name << ") failed to push request: "
					<< x.what() << std::endl;
		}

		return false;
	}
};

// Load receiver agent.
class a_collector_t : public so_5::agent_t
{
public :
	// A signal to send next request to performer.
	struct msg_select_next_job : public so_5::signal_t {};

	a_collector_t(
		// Environment to work in.
		context_t ctx,
		// Receiver's name.
		std::string name,
		// Max capacity of receiver
		std::size_t max_receiver_capacity,
		// Max count of jobs to be processed in parallel.
		std::size_t max_concurrent_jobs )
		:	so_5::agent_t( ctx )
		,	m_name( std::move( name ) )
		,	max_capacity( max_receiver_capacity )
		,	m_available_concurrent_performers( max_concurrent_jobs )
	{
	}

	void set_performer_mbox( const so_5::mbox_t & mbox )
	{
		m_performer_mbox = mbox;
	}

	virtual void so_define_agent() override
	{
		so_default_state()
			.event( &a_collector_t::evt_receive_job )
			.event< msg_select_next_job >( &a_collector_t::evt_select_next_job );
	}

private :
	// Receiver's name.
	const std::string m_name;

	// Max count of items to store between processing turns.
	const std::size_t max_capacity;

	// Storage for requests between turns.
	std::deque< application_request > m_requests;

	// Performer's mbox. All requests will be sent to that mbox.
	so_5::mbox_t m_performer_mbox;

	// Current count of available concurrent performers for parallel processing.
	std::size_t m_available_concurrent_performers;

	bool evt_receive_job( const application_request & what )
	{
		bool processed = true;

		// If there is a free slot then request must be sent to processing.
		if( m_available_concurrent_performers )
		{
			--m_available_concurrent_performers;
			so_5::send< application_request >( m_performer_mbox, what );
		}
		else if( m_requests.size() < max_capacity )
		{
			// Request can be stored for the future processing.
			m_requests.push_back( what );
		}
		else
		{
			// Request must be rejected becase there is no free slots
			// and there is no room to store.
			TRACE() << "COL(" << m_name << ") reject request from "
					<< what.m_generator << std::endl;

			processed = false;
		}

		return processed;
	}

	void evt_select_next_job()
	{
		++m_available_concurrent_performers;

		if( !m_requests.empty() )
		{
			--m_available_concurrent_performers;
			so_5::send< application_request >(
					m_performer_mbox, m_requests.front() );

			m_requests.pop_front();
		}
	}
};

// Load processor agent.
class a_performer_t : public so_5::agent_t,
	private random_generator_mixin_t
{
public :
	a_performer_t(
		// Environment to work in.
		context_t ctx,
		// Performer's name.
		std::string name,
		// Collector mbox.
		so_5::mbox_t collector_mbox )
		:	so_5::agent_t( ctx )
		,	m_name( std::move( name ) )
		,	m_collector_mbox( std::move(collector_mbox) )
	{}

	virtual void so_define_agent() override
	{
		// Just one handler in the default state.
		so_default_state().event(
				&a_performer_t::evt_perform_job,
				so_5::thread_safe );
	}

private :
	// Processor name.
	const std::string m_name;

	// Collector.
	const so_5::mbox_t m_collector_mbox;

	void evt_perform_job( const application_request & job )
	{
		process_request( job );

		so_5::send< a_collector_t::msg_select_next_job >( m_collector_mbox );
	}

	void process_request( const application_request & )
	{
		TRACE() << "PER(" << m_name << ") start processing; thread="
				<< so_5::query_current_thread_id() << std::endl;

		// Just imitation of requests processing.
		// Processing time is proportional to count of requests.
		const auto processing_time = std::chrono::microseconds(
				random( 150, 1500 ) );
		std::this_thread::sleep_for( processing_time );

		TRACE() << "PER(" << m_name << ") finish processing; thread="
				<< so_5::query_current_thread_id() << ", processing_time: "
				<< processing_time.count() / 1000.0 << "ms" << std::endl;
	}
};

std::vector< so_5::mbox_t >
create_processing_coops( so_5::environment_t & env )
{
	std::vector< so_5::mbox_t > result;

	std::size_t capacities[] = { 25, 35, 40, 15, 20 };

	const std::size_t concurrent_slots = 5;

	// There must be a dedicated dispatcher for collectors.
	auto collector_disp = so_5::disp::thread_pool::create_private_disp( env, 2 );

	int i = 0;
	for( auto c : capacities )
	{
		env.introduce_coop( [&]( so_5::coop_t & coop ) {
			// There must be a dedicated dispatcher for performer from
			// that cooperation.
			auto performer_disp = so_5::disp::adv_thread_pool::create_private_disp(
					env,
					concurrent_slots );

			auto collector = coop.make_agent_with_binder< a_collector_t >(
					collector_disp->binder( so_5::disp::thread_pool::bind_params_t{} ),
					"r" + std::to_string(i), c, concurrent_slots );

			auto collector_mbox = collector->so_direct_mbox();
			result.push_back( collector_mbox );

			auto performer = coop.make_agent_with_binder< a_performer_t >(
					performer_disp->binder( so_5::disp::adv_thread_pool::bind_params_t{} ),
					"p" + std::to_string(i), collector_mbox );

			collector->set_performer_mbox( performer->so_direct_mbox() );
		} );

		++i;
	}

	return result;
}

void init( so_5::environment_t & env )
{
	using namespace so_5::disp::thread_pool;

	auto receivers = create_processing_coops( env );

	// A private dispatcher for generators cooperation.
	auto generators_disp = create_private_disp( env, 3 );
	// Registration of generator will start example.
	env.introduce_coop(
			generators_disp->binder( []( bind_params_t & p ) {
				p.fifo( fifo_t::individual );
			} ),
			[&]( so_5::coop_t & coop ) {
				for( int i = 0; i != 3; ++i )
					coop.make_agent< a_generator_t >(
							"g" + std::to_string(i), receivers );
			} );

	// Taking some time for the agents.
	std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
	env.stop();
}

int main()
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

