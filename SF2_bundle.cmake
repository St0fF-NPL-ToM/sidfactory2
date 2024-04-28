################################################################################
#
# SIDFactoryII - bundle executable, so it can be executed/debugged
#
################################################################################
set( src ${CMAKE_ARGV4} )
set( dst ${CMAKE_ARGV5} )
message( "copy necessary files to executable..." )
file( COPY 
	${src}/SIDFactoryII/color_schemes
	${src}/SIDFactoryII/drivers 
	${src}/SIDFactoryII/music 
	${src}/SIDFactoryII/overlay 
	${src}/dist/documentation
	${src}/SIDFactoryII/config.ini
	${src}/SIDFactoryII/COPYING
	DESTINATION ${dst}/
)
