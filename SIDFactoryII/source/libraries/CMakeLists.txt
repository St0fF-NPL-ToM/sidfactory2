################################################################################
#
# SIDFactoryII ... "libraries" folder
#
################################################################################
SF2_ADD_FILES( ${CMAKE_CURRENT_SOURCE_DIR}
	ghc/filesystem.h
	ghc/fs_fwd.h
	ghc/fs_impl.h
	ghc/fs_std.h
	ghc/fs_std_fwd.h
	ghc/fs_std_impl.h
	miniz/miniz.c
	picopng/picopng.h						picopng/picopng.cpp
	residfp/version.cc
	residfp/array.h
	residfp/Dac.h							residfp/Dac.cpp
	residfp/EnvelopeGenerator.h				residfp/EnvelopeGenerator.cpp
	residfp/ExternalFilter.h				residfp/ExternalFilter.cpp
	residfp/Filter.h						residfp/Filter.cpp
	residfp/Filter6581.h					residfp/Filter6581.cpp
	residfp/Filter8580.h					residfp/Filter8580.cpp
	residfp/FilterModelConfig.h				residfp/FilterModelConfig.cpp
	residfp/FilterModelConfig8580.h			residfp/FilterModelConfig8580.cpp
	residfp/Integrator.h					residfp/Integrator.cpp
	residfp/Integrator8580.h				residfp/Integrator8580.cpp
	residfp/OpAmp.h							residfp/OpAmp.cpp
	residfp/Potentiometer.h
	residfp/SID.h							residfp/SID.cpp
	residfp/siddefs-fp.h
	residfp/Spline.h						residfp/Spline.cpp
	residfp/Voice.h
	residfp/WaveformCalculator.h			residfp/WaveformCalculator.cpp
	residfp/WaveformGenerator.h				residfp/WaveformGenerator.cpp
	residfp/resample/Resampler.h
	residfp/resample/SincResampler.h		residfp/resample/SincResampler.cpp
	residfp/resample/TwoPassSincResampler.h
	residfp/resample/ZeroOrderResampler.h
)