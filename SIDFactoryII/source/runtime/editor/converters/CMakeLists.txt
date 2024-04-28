SF2_ADD_FILES( ${CMAKE_CURRENT_SOURCE_DIR}
	converterbase.h					converterbase.cpp

	cc/converter_cc.h				cc/converter_cc.cpp
	cc/source_ct.h					cc/source_ct.cpp

	gt/converter_gt.h				gt/converter_gt.cpp
	gt/source_sng.h					gt/source_sng.cpp

	jch/converter_jch.h				jch/converter_jch.cpp

	mod/converter_mod.h				mod/converter_mod.cpp
	mod/source_mod.h				mod/source_mod.cpp

	null/converter_null.h			null/converter_null.cpp

	utils/consoleostreambuffer.h	utils/consoleostreambuffer.cpp
	utils/misc.h					utils/misc.cpp
	utils/sf2_interface.h			utils/sf2_interface.cpp
	utils/source_utils.h			utils/source_utils.cpp
)