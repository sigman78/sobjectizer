require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.msg_tracing.simple_msg_count'

	cpp_source 'main.cpp'
}

