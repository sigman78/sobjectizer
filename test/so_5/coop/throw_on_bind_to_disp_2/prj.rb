require 'mxx_ru/cpp'
MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.coop.throw_on_bind_to_disp_2" )

	cpp_source( "main.cpp" )
}

