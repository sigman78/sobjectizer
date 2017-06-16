/*
 * A test for simple_not_mtsafe_st_env_infastructure with call to stop in
 * init_fn function.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[]( so_5::environment_t & env ) {
						env.stop();
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_not_mtsafe::factory() );
					} );
			},
			5,
			"stop() in init_fn for simple_not_mtsafe_st_env_infrastructure" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

