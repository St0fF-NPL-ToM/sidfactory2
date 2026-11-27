#[[
    Something like a partial KickAssembler-CMake-Integration
]]
cmake_minimum_required( VERSION 4.2 )


# Additional targets here: the Driver itself!
find_package( Java REQUIRED COMPONENTS Runtime )
# simple configuration: have the JAR somewhere
set( CACHE{KICKASSEMBLER_JAR} TYPE FILEPATH HELP "Where is KickAssembler's jar file?" VALUE "" )
Msg( "KickAssembler: $CACHE{KICKASSEMBLER_JAR}" )

function( add_kick_target TGT_NAME )
    if ( Java_JAVA_EXECUTABLE AND KICKASSEMBLER_JAR )
        cmake_parse_arguments( PARSE_ARGV 1 "" "" "MAIN_SOURCE;OUTPUT_NAME;DEST_DIR" "INCLUDES;DEPENDENT_TARGETS" )

        add_custom_command( OUTPUT ${_OUTPUT_NAME} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND ${Java_JAVA_EXECUTABLE} -jar ${KICKASSEMBLER_JAR} -odir "${CMAKE_CURRENT_BINARY_DIR}" ${_MAIN_SOURCE}
            MAIN_DEPENDENCY ${_MAIN_SOURCE}
            DEPENDS ${_INCLUDES}
            USES_TERMINAL COMMENT "Building ${_MAIN_SOURCE}"
        )
        add_custom_target( ${TGT_NAME} ALL DEPENDS ${_OUTPUT_NAME}
            COMMENT "Moving driver to resource/drivers folder: '${_DEST_DIR}/${_OUTPUT_NAME}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/${_OUTPUT_NAME}" "${_DEST_DIR}"
        )
        foreach( _dt IN LISTS _DEPENDENT_TARGETS )
            add_dependencies( ${_dt} ${TGT_NAME} )
        endforeach()
        Msg( "Added KickAssembler target '${TGT_NAME}', made dependent:" LIST ${_DEPENDENT_TARGETS} )
    else()
        Msg( WARN "Cannot add a kick assembler target, as it cannot be built." )
    endif()
endfunction( add_kick_target )
