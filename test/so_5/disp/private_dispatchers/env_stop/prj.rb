require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.disp.private_dispatchers.env_stop'

	cpp_source 'main.cpp'
}

