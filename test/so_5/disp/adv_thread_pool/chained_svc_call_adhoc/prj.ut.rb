require 'mxx_ru/binary_unittest'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"test/so_5/disp/adv_thread_pool/chained_svc_call_adhoc/prj.ut.rb",
		"test/so_5/disp/adv_thread_pool/chained_svc_call_adhoc/prj.rb" )
)
