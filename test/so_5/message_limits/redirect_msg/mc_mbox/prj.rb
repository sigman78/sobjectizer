require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.message_limits.redirect_msg.mc_mbox'

	cpp_source 'main.cpp'
}

