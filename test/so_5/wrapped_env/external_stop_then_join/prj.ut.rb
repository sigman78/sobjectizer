require 'mxx_ru/binary_unittest'

path = 'test/so_5/wrapped_env/external_stop_then_join'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
