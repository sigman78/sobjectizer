require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.env_infrastructure.simple_mtsafe_st.direct_env_stop'

	cpp_source 'main.cpp'
}

