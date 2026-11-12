#[[
    Copyright 2026 Stefan <St0fF> Kaps

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
]]

#[[
    Better CMake
    ============
    Better CMake is a small extension package containing a few very helpful
    scripts enhancing your overall CMake build chain creation experience :)
    -----------------------------------------------------------------------
    This "BASE" module provides a lot of helper functionality.  Most of it will simply ease coding,
    as it shortens lines by wrapping CMake functions in a less abstract way than the originals.
    -----------------------------------------------------------------------------------------------
    Cache - Helper - Functions:
    ===========================
    Problem: I personally whitnessed situations, where direct Cache variable modification was
    visible during the specific config run, but obviously not written back to the Cache file, as
    if other functions than "set|unset" were unaware of Cache existence.

    Solution: wrapper functions for often used patterns. A direct "write to Cache" var wrapper
    comes in addition to "round up" the package.
    →   Cache_Math()
    →   Cache_List()
    →   Cache_Set()
    -----------------------------------------------------------------------------------------------
    External process execution - Helper Functions:
    ==============================================
    Some people may not know, or not have experienced problems, here, at all.  Anyhow, a fact is:
    → CMake's "CWD" is not necessarily the folder, where you think a script is executed.
    → normally, you'd use CMAKE_SOURCE_DIR and CMAKE_BINARY_DIR as some kind of root folders

    In project mode, those are set internally and you can assume execute_process() to run inside
    the project's CMAKE_CURRENT_SOURCE_DIR folder, if no explicit workdir is given.
    In script mode, CMAKE_SOURCE_DIR and CMAKE_BINARY_DIR are most likely equal and point to the
    CurrentWorkingDirectory, from within your script was run - which might not be what you expect!

    FUNC: cmd( "command to execute" [WORKDIR "<folder>"] … ) → see func-definition for further info

    This function will secure CWD for you.  In Script mode, where CMAKE_SOURCE_DIR will be set ini-
    tially, you can simply manipulate the variable to point to your desired default WORKDIR.
    In any case: if WORKDIR is not given, it will be taken from CMAKE_SOURCE_DIR and passed on to
    execute_process().
 ]]
cmake_minimum_required( VERSION 4.2 )
include_guard( GLOBAL )
#[[ determine_role - simply create a global variable `CMAKE_SCRIPT` ]]
 if ( CMAKE_SCRIPT_MODE_FILE ) # actually, this is a "bool" call, but it's not yet defined.
    set( CACHE{CMAKE_SCRIPT} TYPE INTERNAL FORCE VALUE 1 ) # pushed into the CACHE, so it
 else()
    set( CACHE{CMAKE_SCRIPT} TYPE INTERNAL FORCE VALUE 0 ) # is always ROOT variable level
 endif()
#[[ Cache modification helpers - to simply shorten your code …

    Cache_Set    does what its name says
    Cache_Math   does math on a CACHE variable by performing
                the math on a temporary and writing it back …
    Cache_List   same as with Cache_Math - list ops do not "safely" write back
                into the CACHE, at least I whitnessed a few situations, when
                direct manipulation was not written back to the cache file.
 ]]
function( Cache_Set VARNAME value )
    set( CACHE{${VARNAME}} ${ARGN} FORCE VALUE "${value}" )
 endfunction( Cache_Set )
function( Cache_Math VARNAME math )
    math( EXPR _v "$CACHE{${VARNAME}}${math}" )
    Cache_Set( ${VARNAME} "${_v}" )
 endfunction( Cache_Math )
function( Cache_List ACTION VARNAME )
    set( _v "$CACHE{${VARNAME}}" )
    list( ${ACTION} _v ${ARGN} )
    Cache_Set( ${VARNAME} "${_v}" )
 endfunction( Cache_List )

#[[ helper for execute_process, as an unset CWD may be futile, this always sets a sensible default

    Parameters: OUT_VAR, ERR_VAR, RES_VAR  → should be self-explanatory. These will carry the
                                                respective returns of the CMD execution.
                WORKDIR                       → as noted above, unset defaults to CMAKE_SOURCE_DIR
                FAILS                         → lead-in for error_variable in case of a failure
                                                result (and in case CMD did not return error text)
                ERR_LIST                      → cumulate error variables in a list
                OUT_LIST                      → cumulate output variables in a list
                CUMULATE_ALL_ERROR_VARIABLES  → switch to collecting ERR_LIST in any case when ERR
                                                is non-empty (regardless of cmd result)
    Cumulating results makes sense if you have to work with external tools in chains.  Only having
    to check if an error list contains entries can be very helpful.

    Default behaviour: in case ERR_LIST is set and the cmd result is non-zero (failure), FAILS and
    the content of ERR are appended to the ERR_LIST.

    Cumulate_all: this makes population of ERR_LIST not depend on the result, rather on it
    containing contents.  So any ERR content will be fed into ERR_LIST.  FAILS will only prepended
    in case of a real failure.
 ]]
 function( cmd )
    set( EP_OPT "OUTPUT_QUIET;ERROR_QUIET;OUTPUT_STRIP_TRAILING_WHITESPACE;ERROR_STRIP_TRAILING_WHITESPACE;ECHO_OUTPUT_VARIABLE;ECHO_ERROR_VARIABLE" )
    set( EP_SGL "WORKING_DIRECTORY;TIMEOUT;INPUT_FILE;OUTPUT_FILE;ERROR_FILE;COMMAND_ECHO;ENCODING;COMMAND_ERROR_IS_FATAL" )
    set( EP_VAR "OUTPUT_VARIABLE;ERROR_VARIABLE;RESULT_VARIABLE" )
    # first, grab OUR arguments
    cmake_parse_arguments( PARSE_ARGV 0 "CMD"
        "CUMULATE_ALL_ERROR_VARIABLES;OUTPUT_STRIP_TRAILING_WHITESPACE;ERROR_STRIP_TRAILING_WHITESPACE"
        "OUT_VAR;ERR_VAR;RES_VAR;WORKDIR;ERR_LIST;OUT_LIST;FAILS;RESULTS_VARIABLE;${EP_VAR}" "DO;COMMAND" )
    # now, if "DO" is set, the keyword was given and anything behind DO is captured.  In this
    # case: UNPARSED_ARGUMENTS will contain stuff from "before do" - these can be forwarded
    # directly, without checking. If no "DO" is set, we have to check all UNPARSED_ARGUMENTS
    # against forwarding.
    # This works by applying cmake_parse_arguments with execute_process' signatures on those.
    if ( CMD_COMMAND )
        Msg( FATAL "Please check your cmd()-invocation. DO NOT USE 'COMMAND' with cmd()!" )
    endif()
    foreach( i IN LISTS EP_VAR ) # capture var names given via execute_process-style params
        if ( DEFINED CMD_${i} )
            string( SUBSTRING "${i}" 0 3 _f )
            set( CMD_${_f}_VAR "${CMD_${i}}" )
        endif()
    endforeach()
    if ( CMD_DO )
        set( _c "${CMD_DO}" )
        set( _f "${CMD_UNPARSED_ARGUMENTS}" )
    else()
        set( _c "${CMD_UNPARSED_ARGUMENTS}" )
        unset( _f )
    endif()
    cmake_parse_arguments( "EP" "${EP_OPT}" "${EP_SGL}" "" ${_c} )
    foreach( i IN LISTS _c )
        if ( NOT "${i}" IN_LIST EP_UNPARSED_ARGUMENTS )
            list( APPEND _f "${i}" )
        endif()
    endforeach()
    set( CMD_DO "${EP_UNPARSED_ARGUMENTS}" )
    set( CMD_UNPARSED_ARGUMENTS "${_f}" )
    list( LENGTH CMD_DO _c )
    if ( _c GREATER 1 )
        set( CMD "${CMD_DO}" )      # bereits tokenisiert - direkt verwenden
    else()
        separate_arguments( CMD UNIX_COMMAND PROGRAM SEPARATE_ARGS "${CMD_DO}" )
    endif()
    if ( NOT EP_WORKING_DIRECTORY )
        if ( CMD_WORKDIR )
            if ( DEFINED ${CMD_WORKDIR} ) # in case a variable name was given
                set( CMD_WORKDIR "${${CMD_WORKDIR}}" )
            endif()
            set( CMD_WORKDIR "WORKING_DIRECTORY;${CMD_WORKDIR}" )
        else()
            set( CMD_WORKDIR "WORKING_DIRECTORY;${CMAKE_SOURCE_DIR}" )
        endif()
    else()
        unset( CMD_WORKDIR )
    endif()
    execute_process( ${CMD_WORKDIR}
        OUTPUT_VARIABLE OUT OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE ERR ERROR_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE RES COMMAND ${CMD}
        ${CMD_UNPARSED_ARGUMENTS}
    )
    if ( NOT RES EQUAL 0 ) # failure?
        if ( NOT CMD_FAILS )
            list( GET CMD 0 CMD_FAILS )
            cmake_path( GET CMD_FAILS FILENAME CMD_FAILS )
            string( APPEND CMD_FAILS " failed:" )
        endif()
        set( ERR "${CMD_FAILS} ${ERR}" )
    endif()
    if ( CMD_ERR_VAR )
        set( ${CMD_ERR_VAR} "${ERR}" )
    endif()
    if ( CMD_OUT_VAR )
        set( ${CMD_OUT_VAR} "${OUT}" )
    endif()
    if ( CMD_ERR_LIST AND ERR AND ( CMD_CUMULATE_ALL_ERROR_VARIABLES OR NOT RES EQUAL 0 )  )
        list( APPEND ${CMD_ERR_LIST} "${ERR}" )
    endif()
    if ( CMD_OUT_LIST )
        list( APPEND ${CMD_OUT_LIST} "${OUT}" )
    endif()
    if ( CMD_RES_VAR )
        set( ${CMD_RES_VAR} "${RES}" )
    endif()
    return( PROPAGATE ${CMD_OUT_VAR} ${CMD_ERR_VAR} ${CMD_RES_VAR} ${CMD_OUT_LIST} ${CMD_ERR_LIST} )
endfunction( cmd )

#[[ VERSION variable helper functions - produces|uses version variable "objects"
    see internal docs for details
 ]]
function( Version_parse versionVarOrString )
    set( vp_STYLES BuildNum CMake RPM GitDescribe SemVer )
    cmake_parse_arguments( PARSE_ARGV 1 "vp" "" "TO;STYLE" "SEMVER_PRERELEASE_ORDER" )
    # step 1: is it a string or a variable?
    if ( DEFINED ${versionVarOrString} )
        set( vp_S "${${versionVarOrString}}" )
     else()
        set( vp_S "${versionVarOrString}" )
    endif()
    while ( vp_UNPARSED_ARGUMENTS AND ( NOT DEFINED vp_TO OR NOT DEFINED vp_STYLE ) )
        list( POP_FRONT vp_UNPARSED_ARGUMENTS vp_a )
        if ( "${vp_a}" IN_LIST vp_STYLES )
            if ( NOT DEFINED vp_STYLE )
                set( vp_STYLE "${vp_a}" )
            endif()
        elseif( NOT DEFINED vp_TO )
            set( vp_TO "${vp_a}" )
        endif()
    endwhile()
    if ( DEFINED ${versionVarOrString} AND NOT vp_TO )
        set( vp_TO "${versionVarOrString}" )
    endif()
    # now, vp_TO should be set in any case. If not: undefined behaviour.
    if( DEFINED vp_TO )
        # variable list for this object…
        set( ${vp_TO}_VARS MAJOR MINOR PATCH TWEAK MMP STAGE META FULL )
        foreach( i IN LISTS ${vp_TO}_VARS )
            unset( ${vp_TO}_${i} )
        endforeach()
        list ( APPEND ${vp_TO}_VARS STYLE VARS )
        list( TRANSFORM ${vp_TO}_VARS PREPEND "${vp_TO}_" )
        list( PREPEND ${vp_TO}_VARS "${vp_TO}" )
        if ( NOT vp_STYLE ) # try to autodetect style
            if ( vp_SEMVER_PRERELEASE_ORDER )
                set( ${vp_TO}_STYLE ${vp_SEMVER_PRERELEASE_ORDER} )
            elseif ( "${vp_S}" MATCHES "^[0-9]+$" )
                set( ${vp_TO}_STYLE BuildNum )
            elseif( "${vp_S}" MATCHES "^[0-9]+([.][0-9]+([.][0-9]+([.][0-9]+)?)?)?$" )
                set( ${vp_TO}_STYLE CMake )
            elseif( "${vp_S}" MATCHES "^[0-9]+[.][0-9]+([.][0-9]+([-][0-9]+)?)?$" )
                set( ${vp_TO}_STYLE RPM )
            elseif( "${vp_S}" MATCHES "(.*)(-g[0-9a-f]+)$" )
                # make git-describe a RPM style with a special META
                set( vp_S "${CMAKE_MATCH_1}" )
                set( ${vp_TO}_META "${CMAKE_MATCH_2}" )
                set( ${vp_TO}_STYLE GitDescribe )
            else() # try to match SemVer
                set( ${vp_TO}_STYLE alpha beta rc )
            endif()
         else()
            set( ${vp_TO}_STYLE "${vp_STYLE}" )
            if ( ${vp_STYLE} STREQUAL SemVer )
                if ( vp_SEMVER_PRERELEASE_ORDER )
                    set( ${vp_TO}_STYLE ${vp_SEMVER_PRERELEASE_ORDER} )
                 else()
                    set( ${vp_TO}_STYLE alpha beta rc )
                endif()
            endif()
        endif()
        if ( "${${vp_TO}_STYLE}" STREQUAL CMake )
            string( REGEX MATCH "^([0-9]+)([.]([0-9]+)([.]([0-9]+)([-.]([0-9]+))?)?)?" brot "${vp_S}" )
            set( ${vp_TO}_MAJOR "${CMAKE_MATCH_1}" )
            set( ${vp_TO}_MINOR "${CMAKE_MATCH_3}" )
            set( ${vp_TO}_PATCH "${CMAKE_MATCH_5}" )
            set( ${vp_TO}_TWEAK "${CMAKE_MATCH_7}" )
            set( vp_CLR STAGE META )
        elseif ( "${${vp_TO}_STYLE}" STREQUAL BuildNum )
            set( vp_CLR MAJOR MINOR PATCH STAGE META )
            set( ${vp_TO}_TWEAK "${vp_S}" )
        elseif ( "${${vp_TO}_STYLE}" MATCHES "^(RPM|GitDescribe)$" )
            string( REGEX MATCH "^([0-9]+)[.]([0-9]+)([.]([0-9]+)(-([0-9]+))?)?$" brot "${vp_S}" )
            set( ${vp_TO}_MAJOR "${CMAKE_MATCH_1}" )
            set( ${vp_TO}_MINOR "${CMAKE_MATCH_2}" )
            set( ${vp_TO}_PATCH "${CMAKE_MATCH_4}" )
            set( ${vp_TO}_TWEAK "${CMAKE_MATCH_6}" )
            if ( "${${vp_TO}_STYLE}" STREQUAL GitDescribe )
                set( vp_CLR STAGE )
             else()
                set( vp_CLR STAGE META )
            endif()
        else()
            string( REGEX MATCH "^([0-9]+)[.]([0-9]+)[.]([0-9]+)(-([0-9A-Za-z-]+)([.]([0-9]+))?)?([-+]?(.+))?$" brot "${vp_S}" )
            set( ${vp_TO}_MAJOR "${CMAKE_MATCH_1}" )
            set( ${vp_TO}_MINOR "${CMAKE_MATCH_2}" )
            set( ${vp_TO}_PATCH "${CMAKE_MATCH_3}" )
            set( ${vp_TO}_STAGE "${CMAKE_MATCH_5}" )
            set( ${vp_TO}_TWEAK "${CMAKE_MATCH_7}" )
            if ( CMAKE_MATCH_COUNT GREATER 7 )
                set( ${vp_TO}_META "${CMAKE_MATCH_8}" )
            endif()
            # Now, there is the point in SemVer, that the "stage" denotes a "major.minor.patch
            # are not yet reached!" - the whole point in pre-Releases. Thus, if "stage" was captured,
            # we will most likely have to borrow from a previous major.minor.patch, as that given
            # version is not yet reached, an should compare VERSION_LESS.
            if ( ${vp_TO}_STAGE )
                if ( NOT ${vp_TO}_TWEAK )
                    set( ${vp_TO}_TWEAK "0" )
                endif()
                list( FIND ${vp_TO}_STYLE "${${vp_TO}_STAGE}" _p )
                calc( ${vp_TO}_TWEAK "((${_p} + 1) << 24) + ${${vp_TO}_TWEAK}" )
                foreach( _p PATCH MINOR MAJOR )
                    if( ${vp_TO}_${_p} ) # truthiness means not "0"
                        dec( ${vp_TO}_${_p} )
                        break()
                    else()
                        calc( ${vp_TO}_${_p} "(1<<24)" )
                    endif()
                endforeach()
            endif()
        endif()
        foreach( _v IN LISTS vp_CLR )
            unset( ${vp_TO}_${_v} )
        endforeach()
        Version_render( ${vp_TO} ) # var name
        return( PROPAGATE ${${vp_TO}_VARS} )
    endif()
 endfunction( Version_parse )
function( Version_render vr_versionVar )
    if ( "${${vr_versionVar}_STYLE}" STREQUAL CMake )
        set( ${vr_versionVar} "${${vr_versionVar}_MAJOR}" )
        set( ${vr_versionVar}_MMP "${${vr_versionVar}}" )
        if ( NOT "${${vr_versionVar}_MINOR}" STREQUAL "" )
            set( ${vr_versionVar} "${${vr_versionVar}}.${${vr_versionVar}_MINOR}" )
            set( ${vr_versionVar}_MMP "${${vr_versionVar}}" )
            if ( NOT "${${vr_versionVar}_PATCH}" STREQUAL "" )
                set( ${vr_versionVar} "${${vr_versionVar}}.${${vr_versionVar}_PATCH}" )
                set( ${vr_versionVar}_MMP "${${vr_versionVar}}" )
                if ( NOT "${${vr_versionVar}_TWEAK}" STREQUAL "" )
                    set( ${vr_versionVar} "${${vr_versionVar}}.${${vr_versionVar}_TWEAK}" )
                endif()
            endif()
        endif()
        set( ${vr_versionVar}_FULL "${${vr_versionVar}}" )
    elseif ( "${${vr_versionVar}_STYLE}" STREQUAL BuildNum )
        set( ${vr_versionVar}_FULL "${${vr_versionVar}_TWEAK}" )
        set( ${vr_versionVar}_MMP "0.0.${${vr_versionVar}_FULL}" ) # MMP carries the number: comparability!
        # PLEASE NOTE: one shouldn't version-compare _MMP with full version variables …
        set( ${vr_versionVar} "0.${${vr_versionVar}_MMP}" )
        unset( ${vr_versionVar}_MAJOR )
        unset( ${vr_versionVar}_MINOR )
        unset( ${vr_versionVar}_PATCH )
    elseif ( "${${vr_versionVar}_STYLE}" MATCHES "^(RPM|GitDescribe)$" )
        set( ${vr_versionVar}_MMP "${${vr_versionVar}_MAJOR}.${${vr_versionVar}_MINOR}.${${vr_versionVar}_PATCH}" )
        set( ${vr_versionVar} "${${vr_versionVar}_MMP}.${${vr_versionVar}_TWEAK}" )
        set( ${vr_versionVar}_FULL "${${vr_versionVar}_MMP}-${${vr_versionVar}_TWEAK}" )
        if ( "${${vr_versionVar}_STYLE}" STREQUAL GitDescribe )
            string( APPEND ${vr_versionVar}_FULL "${${vr_versionVar}_META}" )
        endif()
    else() # SemVer
        # CMake-comparables:
        set( ${vr_versionVar}_MMP "${${vr_versionVar}_MAJOR}.${${vr_versionVar}_MINOR}.${${vr_versionVar}_PATCH}" )
        set( ${vr_versionVar} "${${vr_versionVar}_MMP}" )
        if ( NOT "${${vr_versionVar}_TWEAK}" STREQUAL "" )
            set( ${vr_versionVar} "${${vr_versionVar}_MMP}.${${vr_versionVar}_TWEAK}" )
        endif()
        # Original SemVer string:
        if ( ${vr_versionVar}_STAGE ) # prerelease-Version
            set( n 0 )
            foreach( _p PATCH MINOR MAJOR )
                if ( ${vr_versionVar}_${_p} GREATER_EQUAL 16777216 ) # (1 << 24)
                    set( _${_p} 0 )
                else()
                    inc( ${vr_versionVar}_${_p} TO _${_p} )
                    break()
                endif()
                inc( n )    # (n==0)→_PATCH, (n==1)→_PATCH+_MINOR …
            endforeach()
            if ( n GREATER 1 )
                set( ${vr_versionVar}_FULL "${_MAJOR}.${_MINOR}.${_PATCH}" )
            elseif( n EQUAL 1 )
                set( ${vr_versionVar}_FULL "${${vr_versionVar}_MAJOR}.${_MINOR}.${_PATCH}" )
            else()
                set( ${vr_versionVar}_FULL "${${vr_versionVar}_MAJOR}.${${vr_versionVar}_MINOR}.${_PATCH}" )
            endif()
            calc( _T "${${vr_versionVar}_TWEAK} & 0xffffff" )
            if ( NOT _T )
                unset( _T )
             else()
                string( PREPEND _T . )
            endif()
            string( APPEND ${vr_versionVar}_FULL "-${${vr_versionVar}_STAGE}${_T}" )
        else()
            set( ${vr_versionVar}_FULL "${${vr_versionVar}_MMP}" )
            if ( ${vr_versionVar}_TWEAK )
                string( APPEND ${vr_versionVar}_FULL ".${${vr_versionVar}_TWEAK}" )
            endif()
        endif()
        string( APPEND ${vr_versionVar}_FULL "${${vr_versionVar}_META}" )
    endif()
    return( PROPAGATE ${${vr_versionVar}_VARS} )
 endfunction( Version_render )

#[[ query current repository's version number according to TAGs and a few user-defined parameters
    1st positional parameter: Version base variable name.
    TWEAK (i.e. 4th version value) handling:
    → NO_TWEAK: version numbering only covers major, minor, and patch
    → TWEAK_IS_COMMITCOUNT: version numbering follows a 2-piece-scheme, MMP as version base, but tweak as
                            a rolling number counting all commits of the main/current repo branch
    → TWEAK_SINCE_LAST_TAG: counts the commits since the last tag set, thus with each MMP-bump, it gets
                            reset to "0"
 ]]
 function( Version_from_Git VersionBaseVar )
    init_Git()
    cmake_parse_arguments( PARSE_ARGV 1 "gg" "TWEAK_IS_COMMITCOUNT;TWEAK_SINCE_LAST_TAG;NO_TWEAK" "" "" )
    cmd( DO ${GIT_EXECUTABLE} fetch --tags --prune --prune-tags --force )
    cmd( OUT_VAR ltl DO ${GIT_EXECUTABLE} describe --tags --long )
    Version_parse( "${ltl}" TO ${VersionBaseVar}  )
    if ( gg_TWEAK_IS_COMMITCOUNT )
        cmd( OUT_VAR ccnt DO ${GIT_EXECUTABLE} rev-list --count HEAD )
        set( ${VersionBaseVar}_TWEAK "${ccnt}" )
        Version_render( ${VersionBaseVar} )
    elseif( gg_NO_TWEAK )
        set( ${VersionBaseVar} "${${VersionBaseVar}_MMP}")
        unset( ${VersionBaseVar}_TWEAK )
    endif()
    if ( NOT ${VersionBaseVar} )
        if ( gg_NO_TWEAK )
            Version_parse( "0.0.1" TO ${VersionBaseVar} )
         else()
            Version_parse( "0.0.1-0" TO ${VersionBaseVar} )
        endif()
    endif()
    return( PROPAGATE ${${VersionBaseVar}_VARS} )
endfunction( Version_from_Git )

#[[ init_Git - internal function that pulls in Git via find_package as a dependency! ]]
 function( init_Git )
    if( NOT DEFINED GIT_EXECUTABLE )
        find_package( Git QUIET REQUIRED )
    endif()
endfunction( init_Git )

#[[ find_Git_root - find a repository root from a specified working directory. ]]
 function( find_Git_root tgtVar )
    init_Git()
    cmake_parse_arguments( PARSE_ARGV 1 "fpgr" "CACHE" "TREE" "" )
    if ( fpgr_TREE )
        set( fpgr_TREE "WORKDIR;${fpgr_TREE}" )
    endif()
    cmd( ${fpgr_TREE} OUT_VAR _rr ERR_VAR _err DO ${GIT_EXECUTABLE} rev-parse --show-toplevel )
    if ( NOT _err )
        Msg( "Found out project GIT root: '${_rr}'." )
        if ( fpgr_CACHE )
            set( CACHE{${tgtVar}} FORCE VALUE ${_rr} )
        else()
            set( ${tgtVar} ${_rr} )
        endif()
    else()
        set( ${tgtVar} "Could not determine project git repository root…" )
    endif()
    return( PROPAGATE ${tgtVar} )
endfunction( find_Git_root tgtVar )

#[[ string JSON extensions with defaults instead of error handling ]]
function( JSON QUERY tgtvar member default json )
     string( JSON ${tgtvar} ERROR_VARIABLE _err ${QUERY} "${json}" ${member} )
     if ( _err )
         set( ${tgtvar} "${default}" )
     endif()
     return( PROPAGATE ${tgtvar} )
 endfunction( JSON )
function( JSON_list listVar member json )
    JSON( LENGTH _ll "${member}" 0 "${json}" )
    set( _i 0 )
    while( _i LESS _ll )
        string( JSON _j GET "${json}" ${member} ${_i} )
        list( APPEND ${listVar} "${_j}" )
        inc( _i )
    endwhile()
    return( PROPAGATE ${listVar} )
 endfunction( JSON_list )

#[[
    Maths - Helper - Functions:
    ===========================
    FUNCs: calc() / ifCalc() wrap around CMake's `math()`.  While `calc()` simply saves typing that
            ubiquitous EXPR, ifCalc() shortens ternary ops a little, saving you if() else() endf().

    FUNC: bool( <variableName> )
        -   use it on variables you expect to contain a boolean value
        -   converts all "false"-states to "0" and all "true"-states to "1"
        →   so you can use these variables in computations afterwards

    FUNCS: inc|dec( <variableName> [TO <targetVariable>] [BY <valueIsteadOf1>] )
        -   what the name says - in-/decrements either the variable itself (no "TO" given),
            or the TO-target by the amount given as "BY", or "1" if "BY" was not given.
 ]]

#[[ BOOL - simple, yet very effective it you need 0/1 for calculations … ]]
 function( bool varName )
    if ( DEFINED ${varName} AND ${varName} )
        set( ${varName} 1 )
     else()
        set( ${varName} 0 )
    endif()
    return( PROPAGATE ${varName} )
endfunction( bool varName )

#[[ CALC - simple wrapper around math() ]]
 function( calc tgtVar )
    math( EXPR ${tgtVar} "${ARGN}" )
    return( PROPAGATE ${tgtVar} )
endfunction( calc tgtVar )

#[[ ifCalc - not so simple wrapper as a ternary-replacement ]]
 function( ifCalc )
    cmake_parse_arguments( PARSE_ARGV 0 "" "" "YES;NO;VAR" "" )
    if ( NOT ( DEFINED _YES AND DEFINED _NO AND DEFINED _VAR ) )
        message( FATAL_ERROR "ifCalc() why use ifCalc without two calculations and a target variable? '${ARGV}'" )
    else()
        if ( ${_UNPARSED_ARGUMENTS} )
            if ( DEFINED _YES )
                math( EXPR ${_VAR} "${_YES}" )
            endif() # no change if no _yes given
        else()
            if ( DEFINED _NO )
                math( EXPR ${_VAR} "${_NO}" )
            endif() # no change if no _yes given
        endif()
    endif()
    return( PROPAGATE ${_VAR} )
endfunction( ifCalc )

#[[ INC/DEC Helper-functions
    - simple:   inc( var )                  → increment variable, if numeric, by one.
    - add:      inc( var <count> )          → increment variable, if numeric, by count.
    - copy-inc: inc( var tgtVar [<count>] ) → take var's value, if numeric, increment by
                                              count or 1, store to tgtVar
    - named arguments: "TO <tgtVar>", "BY <count>"
    → same options apply to dec
 ]]
 function( incdec_getArgs )
    cmake_parse_arguments( PARSE_ARGV 0 "C" "" "TO;BY" "")
    # ARGV0 is supposed to be the source variable name - thus always "unparsed"
    list( POP_FRONT C_UNPARSED_ARGUMENTS C_FROM )
    while( C_UNPARSED_ARGUMENTS )
        list( POP_FRONT C_UNPARSED_ARGUMENTS C )
        if ( "${C}" MATCHES "[0-9]+" AND NOT DEFINED C_BY )
            set( C_BY "${C}" )
        elseif( "${C}" MATCHES "[A-Za-z_][A-Za-z_0-9]*" AND NOT DEFINED C_TO )
            set( C_TO "${C}" )
        endif()
    endwhile()
    if ( NOT DEFINED C_TO )
        set( C_TO ${C_FROM} )
    endif()
    if ( NOT DEFINED C_BY )
        set( C_BY 1 )
    elseif( NOT "${C_BY}" MATCHES "^[0-9]+$" )
        set( C_BY "${${C_BY}}" )
    endif()
    return( PROPAGATE C_BY C_TO C_FROM )
 endfunction( incdec_getArgs )
function( inc numVar )
    incdec_getArgs( ${ARGV} )
    math( EXPR ${C_TO} "${${C_FROM}} + ${C_BY}" )
    return( PROPAGATE ${C_TO} )
 endfunction( inc numVar )
 function( dec numVar )
    incdec_getArgs( ${ARGV} )
    math( EXPR ${C_TO} "${${C_FROM}} - ${C_BY}" )
    return( PROPAGATE ${C_TO} )
endfunction( dec numVar )

#[[
    Better CMake lists support …
    ============================
    Better CMake is a small extension package containing a few very helpful
    scripts enhancing your overall CMake build chain creation experience :)
    ╔════════════════════════════════════════════════════════════════════════════════════╗
    ║                                                                                    ║
    ║ BetterCMake - X-lists to tackle CMake's nice list delimiter fixation for plaintext ║
    ║                                                                                    ║
    ╚════════════════════════════════════════════════════════════════════════════════════╝
    Basic ideas:    → introducing a command X_list() to operate on such lists

    The delimiter restriction of CMake is well known. But it is not the only restriction,
    that can mess up your lists in certain cases.

    *"Individual elements may not contain an unequal number of [ and ] characters, and may
     not end in a backslash (\). See semicolon-separated lists for details."*
    [quote CMake docs, list.html]

    Though well documented, I actually - working with CMake since decades - was not aware
    of the implications of this line of documentation, until I ran across it.

    So - plain text must not only escape the default delimiters of CMake, but also opening
    and closing braces.

    These are "delimiter characters" - DEFINED on include as:
    → X_SEMI - semicolon replacement char
    → X_OBKT - opening brace replacement
    → X_CBKT - closing brace replacement

    Using CMake regular expressions on `X_list`s, please be aware that to match semicoli or
    braces, you'd have to use above variables - see it positively: no more unreadable escaping
    of braces, simply a readable variable that tells you the name of the character to match.

 ]]
 string( ASCII 1 X_SEMI )
 string( ASCII 2 X_OBKT )
 string( ASCII 3 X_CBKT )
 set( X_single "REVERSE;SORT;REMOVE_DUPLICATES;FROM_STRING;SEP;XLIST" )
 set( X_multi  "SUBLIST;LENGTH;GET;SET;JOIN;FIND;APPEND;PREPEND;INSERT;POP_FRONT;POP_BACK;REMOVE_ITEM;REMOVE_AT;FILTER;TRANSFORM" )
function( X_delimit )
    # first delimit ";" with X_SEMI
    string( REPLACE ";" "${X_SEMI}" ${ARGV0} "${${ARGV0}}" )
    # now delimit "[]" with X_OBKT|X_CBKT
    string( REPLACE "[" "${X_OBKT}" ${ARGV0} "${${ARGV0}}" )
    string( REPLACE "]" "${X_CBKT}" ${ARGV0} "${${ARGV0}}" )
    # done
    return( PROPAGATE ${ARGV0} )
  endfunction( X_delimit )
  function( X_unlimit )
    string( REPLACE "${X_SEMI}" ";" ${ARGV0} "${${ARGV0}}" )
    string( REPLACE "${X_OBKT}" "[" ${ARGV0} "${${ARGV0}}" )
    string( REPLACE "${X_CBKT}" "]" ${ARGV0} "${${ARGV0}}" )
    # done
    return( PROPAGATE ${ARGV0} )
endfunction( X_unlimit )

function( X_list )
    cmake_parse_arguments( PARSE_ARGV 0 "x" "" "${X_single}" "${X_multi}" )
    # which calls can be passed-thru or executed out of line?
    # - LENGTH, SUBLIST, FROM_STRING
    if( x_SUBLIST OR x_LENGTH OR x_REVERSE OR x_SORT OR x_REMOVE_DUPLICATES )
        # Variations to forward and propagate target variable
        list( ${ARGV} )             # target variables: always last param,
        if ( x_SORT )               # except for sort: can have unparsed args,
            set( t ${x_SORT} )      # … so the x_SORT arg is the list variable
         else()
            dec( ARGC TO p )
            set( t "${ARGV${p}}" )  # all others give the target as last param
        endif()
    elseif ( x_FROM_STRING )
        set( s "${ARGV0}" ) # X_list( listVar FROM_STRING <string> )
        X_delimit( x_FROM_STRING )
        if ( NOT x_SEP )
            set( x_SEP "\n" )
        endif()
        string( REPLACE "${x_SEP}" ";" ${s} "${x_FROM_STRING}" )
        # everything happened on ${s} in place → will be propagated!
    elseif ( x_GET )
        list( POP_FRONT x_GET s )   # list: first param
        list( POP_BACK x_GET t )    # target: last param
        list( ${ARGV} )             # forward call for extraction
        list( LENGTH x_GET l )      # how many <index> arguments?
        if ( l EQUAL 1 )            # only ONE!  Result is a string
            X_unlimit( ${t} )       # needs unlimiting
        endif()                     # otherwise result is an X_list
        unset( s )                  # no write back
    elseif ( x_SET )
        # staying in the calling convention style of CMake, here:
        # X_list( SET <listVar> <index> <elementString> )
        list( POP_FRONT x_SET t )   # target is the list: 1st param
        list( POP_BACK x_SET s )    # string as source
        X_delimit( s )                # X-it!
        if ( DEFINED ${x_SET} )     # varName or Number?
            set( x_SET "${${x_SET}}" )  # now there's a position
        endif()
        list( LENGTH ${t} l )
        if ( x_SET LESS l AND x_SET GREATER 0 )
            list( REMOVE_AT ${t} ${x_SET} )
            list( INSERT ${t} ${x_SET} "${s}" )
        elseif( x_SET EQUAL 0 )
            list( PREPEND ${t} ${s} )
        else()
            list( APPEND ${t} ${s} )
        endif()
        unset( s )
    elseif ( x_JOIN )
        list( POP_BACK x_JOIN t )
        list( ${ARGV} )
        X_unlimit( ${t} )
    elseif ( x_FIND )
        list( POP_BACK x_FIND t )
        list( POP_BACK x_FIND s )
        X_delimit( s )
        list( FIND ${x_FIND} "${s}" ${t} )
        unset( s )
    elseif ( x_APPEND OR x_PREPEND OR x_INSERT )
        if ( x_APPEND )
            set( s "${x_APPEND}" )
            set( x "APPEND" )
        elseif( x_PREPEND )
            set( s "${x_PREPEND}" )
            set( x "PREPEND" )
        elseif( x_INSERT )
            set( s "${x_INSERT}" )
            list( GET s 1 p )
            list( REMOVE_AT s 1 )
        endif()
        list( POP_FRONT s t )
        if ( x_XLIST ) # special case: X_list-to-X_list
            set( s "${${x_XLIST}}") # make s the x_list'content
         else()
            # now, we need to check if escaping needs to happen: same procedure as with GET
            # ACCORDING to CMake docs: s now contains [element...] - thus, ALL need escaping!
            foreach( v IN LISTS s )
                X_delimit( v )
                list( APPEND _s "${v}" )
            endforeach()
            set( s "${_s}" )
        endif()
        if ( x )
            list( ${x} ${t} ${s} )
        else()
            list( INSERT ${t} ${p} ${s} )
        endif()
        unset( s ) # only propagate t!
    elseif ( x_POP_FRONT OR x_POP_BACK )
        list( ${ARGV} )         # execute action
        if ( x_POP_FRONT )      # retrieve variable names
            set( t "${x_POP_FRONT}" )
        else()
            set( t "${x_POP_BACK}" )
        endif()
        list( POP_FRONT t s )   # list needs propagation (in_place mod.)
        if ( t )   # are there "returns?"
            foreach( v IN LISTS t )
                X_unlimit( ${v} )
            endforeach()
        endif()
    elseif ( x_REMOVE_ITEM )
        list( POP_FRONT x_REMOVE_ITEM s )
        foreach( v IN LISTS x_REMOVE_ITEM )
            X_delimit( v )
            list( REMOVE_ITEM ${s} "${v}" )
        endforeach()
    elseif ( x_REMOVE_AT OR x_FILTER )
        list( ${ARGV} )
        set( s ${ARGV1} )
    elseif( x_TRANSFORM )
        list( GET x_TRANSFORM 0 s )
        list( GET x_TRANSFORM 1 v )
        cmake_parse_arguments( "t" "" "OUTPUT_VARIABLE" "" ${x_TRANSFORM} )
        if ( t_OUTPUT_VARIABLE )
            set( t "${t_OUTPUT_VARIABLE}" )
        endif()
        if ( "${v}" IN_LIST [APPEND PREPEND REPLACE] )
            set( p 2 )
            if ( "${v}" STREQUAL REPLACE )
                inc( p )
            endif()
            list( GET x_TRANSFORM ${p} v )
            X_delimit( v )
            list_replace( x_TRANSFORM ${p} "${v}" )
            list( TRANSFORM ${x_TRANSFORM} )
         else() # simple pass-thru, source and target were already recorded.
            list( ${ARGV} )
        endif()
    endif()
    # EPILOG: → set target variable to result value, if necessary
    if ( DEFINED t AND DEFINED ret )
        set( ${t} "${ret}" )
    endif()
    # → propagate target and source (in case of in_place modifications)
    return( PROPAGATE ${s} ${t} )
endfunction( X_list )

#[[ list splicer - accommodates for missing CMake functionality.

    In|Out list:                    1st positional Parameter
    Positional numeric Parameters:  <begin> <end> <step> <count>
    Switches:                       INCLUDE | EXCLUDE, RANGE | EXTENT
    Input Parameters:               BEGIN, COUNT, END | END_INCL | END_EXCL, STEP
    Output Parameters:              SPLICE_VAR

    Except for the list to splice itself, all parameters are optional.  The function builds
    its minimum necessary set of parameters from those options given and does not take
    action, if insufficient parameters are given.

    INCLUDE | EXCLUDE operation:    default splicing means EXCLUDE patterned items from list
                                    in-place.  SPLICE_VAR will contain the excluded items.
 ]]
 macro( splice_list_chk )
    # from minimals, prepare "what we know"
    if ( DEFINED SL_BEGIN AND DEFINED SL_END )
        calc( SL_COUNT "${SL_END} - ${SL_BEGIN}" )
    elseif ( DEFINED SL_BEGIN AND DEFINED SL_COUNT )
        calc( SL_END "${SL_BEGIN} + ${SL_COUNT}" )
    elseif ( DEFINED SL_END AND DEFINED SL_COUNT )
        calc( SL_BEGIN "${SL_END} - ${SL_COUNT}" )
    endif()
    if (( DEFINED SL_BEGIN AND DEFINED SL_COUNT ) OR
        ( DEFINED SL_BEGIN AND DEFINED SL_END ) OR
        ( DEFINED SL_COUNT AND DEFINED SL_END ) )
        set( _canSplice 1 )
    else()
        set( _canSplice 0 )
    endif()
    list( LENGTH SL_UNPARSED_ARGUMENTS _ac )
 endmacro( splice_list_chk )

 function( splice_list listName )
    cmake_parse_arguments( PARSE_ARGV 1 "SL" "INCLUDE;EXCLUDE;RANGE;EXTENT" "BEGIN;COUNT;END;END_INCL;END_EXCL;SPLICE_VAR" "" )
    if ( NOT SL_INCLUDE ) # preset to default
        set( SL_EXCLUDE 1 )
        unset( SL_INCLUDE )
    endif()
    if ( NOT SL_EXTENT ) # preset to default
        set( SL_RANGE 1 )
        unset( SL_EXTENT )
    else()
        set( SL_RANGE 0 )
    endif()
    # Welche Kombinationen machen Sinn?
    # BEGIN;COUNT → MINIMUM
    # BEGIN;END*  → MINIMUM, END_* setzt zusätzlich Zählmodus
    # COUNT;END*  → MINIMUM, END_* setzt zusätzlich Zählmodus
    # Positionals: BEGIN END
    if ( DEFINED SL_END_INCL )
        set( SL_END ${SL_END_INCL} )
        set( SL_RANGE 1 )
    elseif( DEFINED SL_END_EXCL )
        set( SL_END ${SL_END_EXCL} )
        set( SL_RANGE 0 )
    endif()
    splice_list_chk()
    if ( NOT _canSplice OR _ac GREATER 0 ) # Die minimal-Konfiguration ist nicht erfüllt, oder es gibt Positionals
        while ( _ac GREATER 0 )
            if( NOT DEFINED SL_BEGIN )
                list( POP_FRONT SL_UNPARSED_ARGUMENTS SL_BEGIN )
            elseif( NOT DEFINED SL_END )
                list( POP_FRONT SL_UNPARSED_ARGUMENTS SL_END )
            else()
                set( SL_UNPARSED_ARGUMENTS "" )
            endif()
            splice_list_chk()
        endwhile()
    endif()
    if ( _canSplice )
        list( LENGTH ${listName} _ll )
        inc( SL_END BY SL_RANGE )
        inc( SL_COUNT BY SL_RANGE )
        # produce sublists
        list( SUBLIST ${listName} 0 ${SL_BEGIN} _tls )
        if ( SL_BEGIN LESS _ll )
            list( SUBLIST ${listName} ${SL_BEGIN} ${SL_COUNT} _tlm )
        else()
            unset( _tlm )
        endif()
        if ( SL_END LESS _ll )
            list( SUBLIST ${listName} ${SL_END} -1 _tle )
        else()
            unset( _tle )
        endif()
        list( APPEND _tls ${_tle} )
        # apply splicing:
        if ( SL_EXCLUDE )
            set( ${listName} "${_tls}" PARENT_SCOPE )
            if ( DEFINED SL_SPLICE_VAR )
                set( ${SL_SPLICE_VAR} "${_tlm}" PARENT_SCOPE )
            endif()
        else()
            set( ${listName} "${_tlm}" PARENT_SCOPE )
            if ( DEFINED SL_SPLICE_VAR )
                set( ${SL_SPLICE_VAR} "${_tls}" PARENT_SCOPE )
            endif()
        endif()
        # done splicing.
    else()
        Msg( WARN "splice_list needs at least 2 final positional parameters. Start, End_incl." )
    endif()
endfunction( splice_list listName )

#[[ list_replace: missing in cmake since forever … simple "replace list item at position" ]]
 function( list_replace listVar position replacementItem )
    # just for those who like to not read docs, make it kind of "safer"
    cmake_parse_arguments( PARSE_ARGV 1 "" "" "AT" "" )
    if ( DEFINED _AT )
        set( position "${_AT}" )
        set( replacementItem "${_UNPARSED_ARGUMENTS}" )
    endif()
    list( REMOVE_AT ${listVar} ${position} )
    list( INSERT ${listVar} ${position} "${replacementItem}" )
    return( PROPAGATE ${listVar} )
endfunction( list_replace listVar position replacementItem )

#[[ Better CMake file paths
    =======================
    Why did they name a command "cmake_path"? It does not return the path to CMake, does it?

    Anyhow … that out of the way … "path" is a weak word in IT.  Can mean anything.
    Uri, file path, network traffic routing, you name things that take some path …

    So FILEPATHS are the real target - CMake's implementation allows for everything,
    but sometimes it's just awkward to use, needs multiple calls and such…

    file_path() tries to be a wrapper around the cmake_path()-command - as such a direct
    replacement in your code - with additional (partially redundand, but sometimes
    readability should win over redundancy) parameters.

    Additionally, file_path() unifies clear/child-parent-relationship checks that
    cmake_path alone needs multiple calls to decide (see the ReadMe's "nitpicking
    scale" complaint) - one call, one clear answer, instead of composing it yourself
    every single time you need it.

    Scope of this module: everything that answers "what IS this path / how does it
    relate to that other path" - not file I/O, not globbing, not real filesystem
    access.  cmake_path() stays the actual workhorse underneath; file_path() just
    stops you from writing the same three-line dance around it, over and over.
 ]]
 #[[ LOAD TIME ACTIONS:
    prepare some helper variables for quick operation.  We need to propagate the <path_var>
    and respective <out_var>, thus, we need to know where to find those parameters.

    Goal: PATH_CMDS_PV1 →   cmake_path operation args, taking <path_var> following the operation
                            identifier
          PATH_CMDS_OVL →   same, but ops taking the <out_var> unconditionally as last argument
          PATH_CMDS_OVC →   same, but ops taking the <out_var> CONDITIONALLY as last argument
                            (e.g. prepended by "OUTPUT_VARIABLE")
 ]]
 set( PATH_CMDS_PV1 "GET;HAS_ROOT_NAME;HAS_ROOT_DIRECTORY;HAS_ROOT_PATH;HAS_FILENAME" )
 list( APPEND PATH_CMDS_PV1 HAS_EXTENSION HAS_STEM HAS_RELATIVE_PART HAS_PARENT_PATH IS_ABSOLUTE
        IS_RELATIVE IS_PREFIX NATIVE_PATH HASH )
 # output variable is unconditionally the last argument:
 set( PATH_CMDS_OVL "${PATH_CMDS_PV1};COMPARE" )
 # output variable is conditionally the last argument, prepended by "OUTPUT_VARIABLE"
 set( PATH_CMDS_OVC "APPEND;APPEND_STRING;REMOVE_FILENAME;REPLACE_FILENAME;REMOVE_EXTENSION" )
 list( APPEND PATH_CMDS_OVC REPLACE_EXTENSION NORMAL_PATH RELATIVE_PATH ABSOLUTE_PATH )
 # refine definitions: fill missing "1st arg is path_var"
 list( APPEND PATH_CMDS_PV1 SET ${PATH_CMDS_OVC} )

function( file_path )
    set( P_SV "CHILD_OF;PARENT_OF;HAS_CHILD;HAS_PARENT;HAS_RELATIVE;RELATIVE_LIST;FILENAME_LIST" )
    cmake_parse_arguments( PARSE_ARGV 0 "P" "" "${P_SV}" "" )
    list( LENGTH P_UNPARSED_ARGUMENTS _i )
    # Our implementations expect 4 args and the use of one of the above operators as ARGV1.
    # basic extended syntax: file_path( <path_var> EXTRA_OPERATOR <cmpPath[Var]> <resultVar> )
    # MEANING: ARGS 0 and 3 are definitely variable names.  Arg#2 needs to be checked.
    if ( ${ARGV1} IN_LIST P_SV )
        set( P_A ${${ARGV0}} ) # variable name given, path needed
        if ( P_FILENAME_LIST )
            set( P_R ${ARGV2} )
         else()
            set( P_R ${ARGV3} ) # variable name given and needed
        endif()
        if ( P_RELATIVE_LIST OR P_FILENAME_LIST )
            foreach( p IN LISTS ${ARGV0} )
                if ( P_RELATIVE_LIST )
                    cmake_path( RELATIVE_PATH p BASE_DIRECTORY ${P_RELATIVE_LIST} OUTPUT_VARIABLE p )
                else()
                    cmake_path( GET p FILENAME p )
                endif()
                list( APPEND pl ${p} )
            endforeach()
            set( ${P_R} ${pl} )
        else()
            set( ${P_R} 0 )     # result == false.
            if ( DEFINED ${ARGV2} ) # maybe varname or path: make it content!
                set( P_B "${${ARGV2}}" )
            else()
                set( P_B ${ARGV2} )
            endif()
            if ( P_A PATH_EQUAL P_B )
                set( P_EQ on )
            endif()
            if ( P_HAS_RELATIVE )
                # Path-comparison alike string-comparison "ENDS_WITH"
                # a ENDS_WITH b ⇒ a HAS_RELATIVE(_ENDING_LIKE) b
                if ( NOT P_EQ ) # no equality  -  Paths are: exclusive!
                    # using X_lists to allow semicoli and brackets in file paths
                    # 1-element-X_lists are escaped strings - so comparing two X_lists
                    # as strings works just fine, as the ";"-delimiter is a simple char.
                    X_list( P_A FROM_STRING "${P_A}" SEP "/" )
                    X_list( P_B FROM_STRING "${P_B}" SEP "/" )
                    while( NOT "${P_A}" STREQUAL "" )
                        list( POP_FRONT P_A ) # explicit "must be relative"
                        if ( "${P_A}" STREQUAL "${P_B}" )
                            set( ${P_R} 1 )
                            break()
                        endif()
                    endwhile()
                endif()
            elseif( P_A AND P_B )
                cmake_path( IS_PREFIX P_A ${P_B} P_0P2 )
                cmake_path( IS_PREFIX P_B ${P_A} P_2P0 )
                if ( NOT P_EQ AND (
                            ( ( P_CHILD_OF OR P_HAS_PARENT ) AND P_2P0 )
                        OR ( ( P_PARENT_OF OR P_HAS_CHILD ) AND P_0P2 ) ) )
                    set( ${P_R} 1 )
                endif()
            endif()
        endif()
        return( PROPAGATE ${P_R} )
    else() # find out <path_var> and <out_var> (if any), then propagate
        dec( _i )
        dec( _i TO _j )
        list( GET P_UNPARSED_ARGUMENTS 0 _it )
        unset( P_PV )
        if ( _it IN_LIST PATH_CMDS_PV1 )
            set( P_PV ${ARGV1} ) # only ARGV1 carries <path_var> in any kind of invocation.
        endif()
        # now find out <out_var>, if given / required
        unset( P_OV )
        if( _it IN_LIST PATH_CMDS_OVL )
            set( P_OV ${ARGV${_i}} )
        elseif( _it IN_LIST PATH_CMDS_OVC AND _j GREATER -1 )
            list( GET P_UNPARSED_ARGUMENTS ${_j} _j )
            if ( "${_j}" STREQUAL "OUTPUT_VARIABLE" )
                set( P_OV ${ARGV${_i}} )
            endif()
        endif()
        # finally, we know the variables that may change, we can pass the whole call on…
        cmake_path( ${ARGV} )
        return( PROPAGATE ${P_PV} ${P_OV} )
    endif()
endfunction( file_path )

#[[ path_list - batch multiple per-item transformations into a single call
    ---------------------------------------------------------------------
    Purpose: apply several operations to every item of a path list in one pass,
    instead of looping over the list yourself for each transformation.

    Operation order per item (fixed, not user-selectable):
      1. MAKE_RELATIVE <dir>   - re-base the path relative to <dir> (cmake_path
                                  RELATIVE_PATH - genuinely path-aware)
      2. PREPEND_STRING <txt>  - raw text prefix (NOT a path-segment join!)
      3. APPEND_STRING  <txt>  - raw text suffix (NOT a path-segment join!)

    PREPEND_STRING/APPEND_STRING are plain string(PREPEND|APPEND) - deliberately,
    since this command's original (and so far only) use case is building
    .gitignore-block lines, where prefixes like "!" and suffixes like "/**" are
    gitignore-pattern syntax, not filesystem path segments.  Using cmake_path's
    path-aware APPEND here would silently corrupt those patterns by inserting a
    separator that doesn't belong there.

    Combinations: all three may be given together; they always apply in the
    fixed order above, never combined into path-aware joins.

    ⚠ subject to further enhancement: only the operations actually needed so far
    are implemented. No generic operation-parser exists (and isn't currently
    planned) - extend this function's fixed sequence deliberately if a new
    per-item operation is ever needed, rather than assuming arbitrary
    combinations/ordering are supported.
 ]]
 function( path_list listVar )
    cmake_parse_arguments( PARSE_ARGV 1 "PL" "" "MAKE_RELATIVE;APPEND_STRING;PREPEND_STRING;OUTPUT_VARIABLE" "" )
    if ( PL_UNPARSED_ARGUMENTS )
        message( FATAL_ERROR "path_list called with bad arguments: '${PL_UNPARSED_ARGUMENTS}'" )
    elseif( PL_MAKE_RELATIVE AND NOT IS_DIRECTORY ${PL_MAKE_RELATIVE} )
        message( FATAL_ERROR "path_list MAKE_RELATIVE called, but target '${PL_MAKE_RELATIVE}' is no folder!" )
    else()
        foreach( _p IN LISTS ${listVar} )
            if ( PL_MAKE_RELATIVE ) # make all list-entries relative to a folder
                cmake_path( RELATIVE_PATH _p BASE_DIRECTORY ${PL_MAKE_RELATIVE} )
            endif()
            if ( PL_PREPEND_STRING )
                string( PREPEND _p "${PL_PREPEND_STRING}" )
            endif()
            if ( PL_APPEND_STRING )
                string( APPEND _p "${PL_APPEND_STRING}" )
            endif()
            list( APPEND _pl "${_p}" )
        endforeach( _p )
        if ( PL_OUTPUT_VARIABLE )
            set( ${PL_OUTPUT_VARIABLE} "${_pl}" PARENT_SCOPE )
        else()
            set( ${listVar} "${_pl}" PARENT_SCOPE )
        endif()
    endif()
endfunction( path_list )

#[[ Messages - Module
    =================
    THIS IS NOT A REPLACEMENT FOR CMAKE's `message()` COMMAND!  It is an enhanced
    version for those "message()" - output modes, that are mostly used, as well
    as a replacement for the … CHECK_START|_PASS|_FAIL … things …

    Enhancements:
    - replaces the CHECK-functionalities with START|DONE and even FORCE
      for beginning and ending indentation.
      - the "START" messages are delayed if possible, to catch non-outputting
        "blocks" and continue with the "DONE" - message on the same line
      - only works right when "Msg()" is used - there is no hook to actually
        overwrite "message()" or get notified when it is called.
    - size-filtered and aligned, nearly human readable list output
      - during dev of most projects, there comes the point where you abstract
        your own list output, because it was just never added to CMake.
      - Msg can do:
        - "PRE"     - initiating text for list output
        - "POST"    - closing output after list
        - "LIST"    - give it a list, and it will be output nicely aligned
        - "HIDE_EMPTY" … what it says …

    Msg also handles list output in block-starts or -endings:
    e.g.:   Msg( START PRE "TODO:" LIST
                 "CMake verbessern;enhance CMake;ja nje snaju pa russki"
                 POST "starting …" )
    or      Msg( DONE PRE "results" LIST ${listVar} POST "cmake enhanced!" )
    ---------------------------------------------------------------------------
    Messages - Progress Reporting
    =============================
    Some actions take time, so it'd be nice to be able to show how many "action"
    steps remain - or simply "progress reporting" in some way.

    This "problem" will be tackled by a "progress report indentation".  The very first
    indent will be the progress.  Thus, it needs some design-items, to fit basic UX
    requirements.
    To display progress, we need to know how many steps were added to a list of actions,
    and how many steps have already been popped. So it is a count of actions and a count
    of accomplished actions which divide into the "DONE" percentage.
    To beautify a progress report, there needs to be a starting and a terminal line to
    glue the progress itself together, a rule how to create the progress item and how to
    fit it into those beautifications.  The beautifications: simple block building items.

    begin_progress( TOTAL       addedVar    CURRENT         poppedVar
                    LEAD        "DONE"      EXPR            "@DONE@%"
                    LEFT        "│;╭;╰;├;─"      NUM_WIDTH       3
                    RIGHT       "│;╮;╯;┤;┊"      INDENT_AFTER    2           )

    show_progress_info( LEAD "step-text (length: LEAD-length)" TEXT "message to output" )

    end_progress(   LEAD        "────"      "text to output before removing
                                             the progress indentations"     )

    One piece is missing: how to "make progress" - well, actually not - as the progress
    reporting takes YOUR variables, you have to keep them up to data.  Every Msg() in
    between `begin_progress()` and `end_progress()` whill have the current EXPR result
    embedded in the first indentation.
    `show_progress_info` on the other hand, will act like a "small break" showing given
    "LEAD" or "LEAD_INCL" instead of your EXPR's result as that indent.

    EXPR: the expression may contain the variables "@DONE@", "@CURRENT@", and "@TOTAL@"
    It will be used to string( CONFIGURE ) the progress info.  Those numbers will be
    extended to "NUM_WIDTH" characters of length, so it can be easily embedded with
    LEFT, RIGHT and INDENT_AFTER.

    Building Blocks: LEFT/RIGHT: list of characters
    → normal-vertical, top-start, bottom-end, intersection
    The 5th list elements differs on both lists:
    → LEFT[4] is "horizontal-element"
    → RIGHT[4] is "vertical-ellipsis-element"
    These 5th elements are only used with or instead of intersections (depending on the
    text length) of show_progress_info() - leads.
 ]]
 # This messaging module must use the Cache for internal storage, as there seems
 # to be no other way to ensure userspace propagation of our Variables.
 Cache_Set( MSG_preIndTx "" TYPE INTERNAL )
 Cache_Set( MSG_lineMaxXDef 180 TYPE INTERNAL )
 Cache_Set( MSG_indentXDef 4 TYPE INTERNAL )
 Cache_Set( MSG_listMaxYDef 5 TYPE INTERNAL )
 Cache_Set( MSG_breakAfterNlinesDef 3 TYPE INTERNAL )

 Cache_Set( MSG_progTV "" TYPE INTERNAL )
 Cache_Set( MSG_progCV "" TYPE INTERNAL )
 Cache_Set( MSG_progEX "@DONE@%" TYPE INTERNAL ) # sensible defaults - LEAD is 4 chars
 Cache_Set( MSG_progL "│;╭;╰;├;┈" TYPE INTERNAL ) # if the output shall be "embedded", set
 Cache_Set( MSG_progR "│;╮;╯;┤;┆" TYPE INTERNAL ) # left and right embedding char_lists
 Cache_Set( MSG_progNW "3" TYPE INTERNAL )  # number width
 Cache_Set( MSG_progEW "0" TYPE INTERNAL )  # width of EXPR output (calculated once)
 Cache_Set( MSG_prog "0" TYPE INTERNAL )    # progress_enabled: n "spaces after"

#[[ Msg() - the heart of Messages …
    ===============================
    by default, all messages are output to the STATUS log level. FATAL and WARN
    are overridden parameters - those are most helpful in debugging your build
    system for outputting LISTs inside FATALs or WARNINGs.

    Parameters (in Order of importance, I guess):
    TEXT        to explicitly mark the following parameter as the string to output
    WARN        use WARNING level for output
    FATAL       use FATAL_ERROR level for output
    START       set given text as heading, then indent (no text: simply indent)
    FORCE       DO NOT delay output of the heading, as standard "message()" - calls
                will be expected
    DONE        unindent, output finishing text
    HIDE_EMPTY  hides empty list output (PRE+LIST+POST)

    Following parameters apply to list output only:
    LIST    uses the following parameter as a CMake list of strings to output
    PRE     introductory text
    POST    extroductory text
    LEAD    character to enclose list elements in, e.g. "'" (the default)
    JOIN    character to connect multiple list elements, e.g. "," (also default)
    MAXX    maximum line with in characters - defaults to CACHE{MSG_lineMaxXDef}
            → so the actually used default value can be preset at script level!
    MAXY    maximum lines to fill with items (ellipsis in the center, if list
            contains too many items) (config: CACHE{MSG_listMaxYDef})

    Internal Option:
    PROG_OK     a currently active progress meter is already "current"
 ]]
function( Msg )
    cmake_parse_arguments( PARSE_ARGV 0 "A" "START;FORCE;DONE;FATAL;WARN;HIDE_EMPTY;PROG_OK" "TEXT;PRE;POST;LEAD;JOIN;MAXX;MAXY" "LIST" )
    if ( DEFINED A_UNPARSED_ARGUMENTS AND NOT DEFINED A_TEXT )
        set( A_TEXT "${A_UNPARSED_ARGUMENTS}" )
    endif()
    if ( A_FATAL OR A_WARNING )
        if ( A_START )
            set( A_FORCE on )
        endif()
    endif()
    if ( A_FORCE )
        set( A_START on )   # Implied!
    endif()
    # check if progress reporting is enabled - and if so, update!
    if ( MSG_prog GREATER 0 AND NOT A_PROG_OK )
        calculate_progress_done()   # replaces the first indent with current progress
    endif()
    # prepare list lines
    if ( A_LIST OR LIST IN_LIST A_KEYWORDS_MISSING_VALUES )
        # list output requested → too much, call a function inheriting the scope!
        msg_prepareList()   # returns: A_TEXT|A_LIST changed to the correct output lines.
    endif()

    if ( ( A_FATAL OR A_WARN ) AND A_LIST )
        if ( A_TEXT ) # put out list, first, if text was given, too
            msg_put_out( "${A_LIST}" )
        else() # otherwise push list into text
            list( JOIN CMAKE_MESSAGE_INDENT "" _rpl )
            string( REPLACE "\n" "\n${_rpl}" A_TEXT "${A_LIST}" )
        endif()
    endif()
    if ( A_FATAL OR A_WARN )
        if ( A_DONE )
            msg_put_out()
            list( POP_BACK CMAKE_MESSAGE_INDENT )
        endif()
        if ( A_FATAL )
            message( FATAL_ERROR ${A_TEXT} )
        elseif( A_WARN )
            message( WARNING ${A_TEXT} )
        endif()
    else()
        if ( A_START )
            if ( A_TEXT )
                msg_preindent( A_TEXT )
            endif()
            if ( A_LIST )
                msg_put_out( "${A_LIST}" )
            endif()
        elseif( A_DONE )
            if ( A_LIST )
                msg_put_out( "${A_LIST}" )
                list( POP_BACK CMAKE_MESSAGE_INDENT )
                msg_put_out( "… ${A_TEXT}" )
            else()
                if ( MSG_preIndTx )
                    set( A_TEXT "$CACHE{MSG_preIndTx} … ${A_TEXT}" )
                    Cache_Set( MSG_preIndTx "" )
                endif()
                list( POP_BACK CMAKE_MESSAGE_INDENT )
                if ( A_TEXT )
                    msg_put_out( "${A_TEXT}" )
                endif()
            endif()
        else()
            if ( A_LIST )
                msg_put_out( "${A_LIST}" )
            endif()
            if ( A_TEXT )
                msg_put_out( "${A_TEXT}" )
            endif()
        endif()
    endif()
    return( PROPAGATE CMAKE_MESSAGE_INDENT )
 endfunction( Msg )

# internal helpers
function( msg_put_out )
    if ( MSG_preIndTx )
        set( _t "${MSG_preIndTx}" )
        Cache_Set( MSG_preIndTx "" )
        list( POP_BACK CMAKE_MESSAGE_INDENT _ni )
        msg_put_out( "${_t} …" )
        list( APPEND CMAKE_MESSAGE_INDENT "${_ni}" )
    endif()
    # Die Ausgabefunktion kann Leerzeilen entfernen und anfügen - deshalb die Kapselung.
    if ( "${ARGN}" MATCHES ".+\n.+" )
        string( REPLACE "\n" ";" _txt "${ARGN}" )
        set( A_l 0 )
        foreach( _tx IN LISTS _txt )
            string( STRIP "${_tx}" _tt )
            if ( _tt )
                message( STATUS "${_tx}" )
                math( EXPR A_l "${A_l} + 1" )
            endif()
        endforeach()
        if( A_l GREATER MSG_breakAfterNlinesDef AND NOT A_DONE )
            message( STATUS "" )
        endif()
    elseif( ARGN ) # nur eine Zeile → direkt ausgeben!
        message( STATUS "${ARGN}" )         # leere Zeilen haben auch Gründe, sie zu übergehen!
    endif()
 endfunction( msg_put_out )
function( msg_preindent textVar )
    if ( MSG_preIndTx )
        msg_put_out() # output empty: should "deprocess" prev. indent
    endif()
    string( REPEAT " " ${MSG_indentXDef} _ni )
    Cache_Set( MSG_preIndTx "${${textVar}}" )
    unset( ${textVar} )
    list( APPEND CMAKE_MESSAGE_INDENT "${_ni}" )
    set( A_START off )
    return( PROPAGATE CMAKE_MESSAGE_INDENT A_START ${textVar} )
 endfunction( msg_preindent )
# list output helpers
function( msg_prepareList )
    if ( NOT DEFINED A_MAXX )
        set( A_MAXX $CACHE{MSG_lineMaxXDef} )
    endif()
    if ( NOT DEFINED A_MAXY )
        set( A_MAXY $CACHE{MSG_listMaxYDef} )
    endif()
    if ( NOT DEFINED A_LEAD )
        set( A_LEAD "'" )
    endif()
    if ( NOT DEFINED A_JOIN )
        set( A_JOIN "," )
    endif()
    # Jetzt wird's spaßig :)  Ausgabestrategie:
    # - bestimme die Maximalbreite eines Elementes
    # - setze sie ins Verhältnis zur Ausgabebreite
    # - bestimme die Anzahl der Zeilen anhand aller gegebenen Breiten und TabWidth
    # → sind es <= MAXY → der Reihe nach zusammenbauen und ausgeben.
    #   → ansonsten: bis zur Hälfte ausgeben, Ellipsis, Index-Offset berechnen und weiter ausgeben
    list( LENGTH A_LIST A_ll )                  # Listen-Länge
    if ( A_ll GREATER 0 )                       # nur "wahres Treatment", wenn auch Elemente existieren
        # Im Falle von Elementen in der Liste ist immer klar: entweder PRE ist gegeben, oder der TEXT ist PRE
        if ( NOT A_PRE AND A_TEXT )
            set( A_PRE "${A_TEXT}" )
            unset( A_TEXT )
        endif()
        string( REPEAT " " $CACHE{MSG_indentXDef} A_indent )
        # NUN: zur eigentlichen Liste …
        dec( A_ll TO A_listLst )                    # Listen-MaxItem
        list( JOIN CMAKE_MESSAGE_INDENT "" _ind )   # current indent besorgen
        string( LENGTH "${_ind}" A_indLen )         # diese Zeichen werden jeder Zeile an MAXX fehlen
        calc( A_MAXX "${A_MAXX} - ${A_indLen}" )
        # Bestimme die Maximalbreite eines Items, dann die Item-Breite in den Zeilen
        msg_adjustWidth()
        # das "PRE" der ersten Zeile sollte vorab "aligned" werden!
        string( LENGTH "${A_PRE}" A_preLen )
        ifCalc( A_preLen LESS_EQUAL ${MSG_indentXDef} VAR corr_cnt
            YES "${MSG_indentXDef} - ${A_preLen}"
            NO "((${A_preLen} - ${MSG_indentXDef}) / ${A_maxW} + 1) * ${A_maxW} - (${A_preLen} - ${MSG_indentXDef})" )
        string( REPEAT " " ${corr_cnt} corr )
        string( APPEND A_PRE "${corr}" )
        string( LENGTH "${A_PRE}" A_preLen )
        string( LENGTH "${A_POST}" A_postLen )
        calc( A_line_len "${A_MAXX} - ${MSG_indentXDef}" )
        if ( A_postLen GREATER 0 )
            calc( A_last_line_len "${A_MAXX} - ${MSG_indentXDef} - 1 - ${A_postLen}" )
            set( A_postLen 1 ) # make it a factor for calculations!
        else()
            set( A_last_line_len ${A_line_len} )
        endif()
        # how many items are left …
        calc( _i "${A_ll} - (${A_MAXX}-${A_preLen})/${A_maxW}" )
        if ( _i GREATER 0 )
            calc( _i "${_i} - ( ${A_last_line_len} / ${A_maxW} )" )
            ifCalc( _i GREATER 0 VAR A_lineCnt
                YES "( ${_i} / ( ${A_line_len} / ${A_maxW} ) ) + 2"
                NO 2 )
        else()
            set( A_lineCnt 1 )
        endif()
        # Zeilenanzahl bestimmt - ab jetzt wird es einfach
        unset( lines )
        set( i 0 )
        set( line "${A_PRE}" )
        msg_pushListItems( i line ${A_maxW} ${A_MAXX} true )
        if ( A_lineCnt LESS_EQUAL A_MAXY )
            set( _i 0 )
            while( _i LESS A_lineCnt )
                list( APPEND lines "${line}" )
                inc( _i )
                set( line "${A_indent}" )
                if ( _i LESS A_lineCnt )
                    msg_pushListItems( i line ${A_maxW} ${A_MAXX} true )
                else()
                    msg_pushListItems( i line ${A_maxW} ${A_last_line_len} true )
                endif()
            endwhile()
        else()  # zu viele Elemente - wir müssen 2-Schienig fahren. Die erste Zeile "steht schon"
            calc( half_lc "(${A_MAXY} >> 1)" )
            set( lc 1 )
            while( lc LESS half_lc )
                list( APPEND lines "${line}" )
                set( line "${A_indent}" )
                msg_pushListItems( i line ${A_maxW} ${A_MAXX} true )
                inc( lc )
            endwhile()
            list( APPEND lines "${line}" )
            calc( corr_cnt "${A_maxW} - 5" )
            string( REPEAT " " ${corr_cnt} line )
            string( PREPEND line "${A_indent}[ … ]" )
            # offset bzw. neues i berechnen - wie viele Items werden übersprungen?
            if ( per_line GREATER 1 )
                # wieviele Items passen in die restl. Zeilen?  Die erste hat schon mal 1 weniger.
                # letzte Zeile hat ggf. weniger Items … am Ende ist das die Anzahl der Items,
                # die wir jetzt noch schaffen werden …
                calc( full_lines_left "${A_MAXY} - ${half_lc} - ${A_postLen}" )
                calc( last_line_cnt "( ${A_line_len} - ${A_last_line_len} - 1 ) / ${A_maxW}" )
                calc( left_item_cnt "${full_lines_left} * ${per_line} - 1 + ${A_postLen} * ${last_line_cnt}" )
            else()
                # Lange items, nur eins pro Zeile - easy-peasy?
                calc( left_item_cnt "${A_MAXY} - ${half_lc} - ${A_postLen} - 1" )
            endif()
            calc( i "${A_ll} - ${left_item_cnt}" )
            msg_pushListItems( i line ${A_maxW} ${A_MAXX} true )
            # Rest zusammenbauen
            while ( lc LESS A_MAXY )
                list( APPEND lines "${line}" )
                inc( lc )
                set( line "${A_indent}" )
                if ( lc LESS A_MAXY )
                    msg_pushListItems( i line ${A_maxW} ${A_MAXX} true )
                else()
                    msg_pushListItems( i line ${A_maxW} ${A_last_line_len} true )
                endif()
            endwhile()
        endif()
        list( APPEND lines "${line} ${A_POST}" )
        list( REMOVE_ITEM lines "${A_indent}" ) # remove empty lines from output
        if ( A_TEXT )
            list( JOIN lines "\n" A_LIST )
        else()
            list( JOIN lines "\n" A_TEXT )
            unset( A_LIST )
        endif()
    else()
        # EMPTY LIST → prepare some output, most likely this should result in a single line.
        # But we also added A_HIDE_EMPTY!
        if ( NOT A_HIDE_EMPTY )
            if ( DEFINED A_PRE )
                if ( DEFINED A_POST )
                    set( _TEXT "${A_PRE} … (empty list) … ${A_POST}" )
                else()
                    set( _TEXT "${A_PRE} … (empty list)" )
                endif()
            elseif( DEFINED A_POST )
                set( _TEXT "(empty list) … ${A_POST}" )
            else()
                set( _TEXT "(empty list)" )
            endif()
            # decide what to do with this output
            string( LENGTH "${_TEXT}" _ttl )
            string( LENGTH "${A_TEXT}" _atl )
            calc( _ttl "${_ttl}+${_atl} - ${A_MAXX}" )
            if ( NOT A_TEXT ) # no A_TEXT → replace, let upstream manage the rest
                set( A_TEXT "${_TEXT}" )
            elseif( _ttl LESS 1 )
                set( A_TEXT "${A_TEXT} – ${_TEXT}" ) # upstream manage the rest
            else() # longer empty list notice - keep Order as User seems to expect
                if ( A_START )
                    msg_preindent( A_TEXT )
                endif()
                set( A_TEXT ${_TEXT} )
            endif()
        endif() # HIDE_EMPTY: no changes to A_TEXT, return empty A_LIST
        unset( A_LIST )
    endif()
    return( PROPAGATE A_LIST A_TEXT A_DONE A_START A_FORCE CMAKE_MESSAGE_INDENT )
 endfunction( msg_prepareList )

function( msg_pushListItems iVar ln itemW lineW fwd )
    string( LENGTH "${A_JOIN}" jl )
    string( REPEAT " " ${jl} jspc )
    while( ${${iVar}} LESS_EQUAL A_listLst AND ${${iVar}} GREATER_EQUAL 0 )
        string( LENGTH "${${ln}}" sl )
        calc( left "${lineW} - ${sl} - ${itemW} + $CACHE{MSG_indentXDef}" )
        if ( left GREATER 0 )
            list( GET A_LIST ${${iVar}} item )
            string( LENGTH "${A_LEAD}${item}${A_LEAD}${A_JOIN}" il )
            dec( itemW BY il TO corr_cnt )
            string( REPEAT " " ${corr_cnt} spcs )
            if ( fwd )
                if ( ${${iVar}} EQUAL A_listLst )
                    string( APPEND ${ln} "${A_LEAD}${item}${A_LEAD}${jspc}${spcs}" )
                else()
                    string( APPEND ${ln} "${A_LEAD}${item}${A_LEAD}${A_JOIN}${spcs}" )
                endif()
                inc( ${iVar} )
            else()
                if ( ${${iVar}} EQUAL A_listLst )
                    string( PREPEND ${ln} "${A_LEAD}${item}${A_LEAD}${jspc}${spcs}" )
                else()
                    string( PREPEND ${ln} "${A_LEAD}${item}${A_LEAD}${A_JOIN}${spcs}" )
                endif()
                dec( ${iVar} )
            endif()
        else()
            break()
        endif()
    endwhile()
    return( PROPAGATE ${iVar} ${ln} )
 endfunction()

function( msg_adjustWidth )
    set( A_maxW 0 )
    foreach( i IN LISTS A_LIST )
        string( LENGTH "${i}" il )
        if ( il GREATER A_maxW )
            set( A_maxW "${il}" )
        endif()
    endforeach()
    string( LENGTH "${A_LEAD}${A_LEAD}${A_JOIN}" lj ) # Item-Verknüpfung
    # Maximallbedarf eines Items
    inc( A_maxW BY lj )
    # Element-Breite bestimmt: die Zeilenanzahl kann berechnet werden.
    dec( A_MAXX BY $CACHE{MSG_indentXDef} TO lineswid )
    calc( per_line "${lineswid} / ${A_maxW}" )
    if ( per_line GREATER 1 )
        calc( item_wid "${lineswid} / ${per_line}" )
        calc( item_wid "${item_wid} - (${item_wid} % $CACHE{MSG_indentXDef})" )
        if ( item_wid GREATER_EQUAL A_maxW )
            set( A_maxW ${item_wid} )
        else()
            inc( item_wid BY $CACHE{MSG_indentXDef} TO A_maxW )
        endif()
    elseif( lineswid LESS_EQUAL A_maxW )
        # nur ein (oder gar kein) Element passt in die Zeilenbreite → A_maxW bedarf keiner Anpassung.
        inc( A_maxW BY $CACHE{MSG_indentXDef} TO A_MAXX )
        set( per_line 1 )
    endif()
    return( PROPAGATE A_maxW A_MAXX per_line )
 endfunction( msg_adjustWidth )

# progress reporting helpers
function( begin_progress )
    cmake_parse_arguments( PARSE_ARGV 0 "pr" ""
        "LEAD;LEAD_INCL;LEAD_CHAR;LEFT;RIGHT;TOTAL;CURRENT;EXPR;NUM_WIDTH;INDENT_AFTER" "" )
    if ( DEFINED "${pr_TOTAL}" AND DEFINED "${pr_CURRENT}" AND ( DEFINED pr_LEAD OR DEFINED pr_LEAD_INCL OR DEFINED pr_LEAD_CHAR ) )
        Cache_Set( MSG_progTV "${pr_TOTAL}" )
        Cache_Set( MSG_progCV "${pr_CURRENT}" )
        if ( DEFINED pr_LEFT )
            Cache_Set( MSG_progL "${pr_LEFT}" )
         endif()
         if ( DEFINED pr_RIGHT )
            Cache_Set( MSG_progR "${pr_RIGHT}" )
         endif()
         if ( DEFINED pr_EXPR )
            Cache_Set( MSG_progEX "${pr_EXPR}" )
         endif()
         if ( DEFINED pr_NUM_WIDTH )
            Cache_Set( MSG_progNW "${pr_NUM_WIDTH}" )
         endif()
         if ( NOT DEFINED pr_INDENT_AFTER )
            set( pr_INDENT_AFTER "2" )
        endif()
        # Calculate width in characters of EXPR output (LEAD width)
        set( DONE 1 )                   # good for autoresize: get width
        get_current_lead( pr_pl )
        string( LENGTH "${pr_pl}" pr_pl )   # got width.
        Cache_Set( MSG_progEW ${pr_pl} )    # fixated it.
        Cache_Set( MSG_prog "${pr_INDENT_AFTER}" )
        # Variables prepared … now prepend indentation
        if ( DEFINED pr_LEAD_INCL )
            set( pr_LEAD "${pr_LEAD_INCL}" )
         else()
            set( pr_LEAD_ID 1 )
            if( DEFINED pr_LEAD_CHAR )
                string( REPEAT "${pr_LEAD_CHAR}" ${MSG_progEW} pr_LEAD )
             else()
                pad_lead( pr_LEAD pr_LEAD_ID )
            endif()
            embed_progress( ${pr_LEAD_ID} pr_LEAD ${pr_INDENT_AFTER} )
        endif()
        list( PREPEND CMAKE_MESSAGE_INDENT "${pr_LEAD}" )
        Msg( PROG_OK ${pr_UNPARSED_ARGUMENTS} )
    else()
        Msg( WARN "begin_progress() reporting failed. Bad arguments." )
    endif()
    return( PROPAGATE CMAKE_MESSAGE_INDENT )
 endfunction( begin_progress )

function( show_progress_info )
    # this special case is handled as a one-liner with no further keywords or switches allowed!
    cmake_parse_arguments( PARSE_ARGV 0 "pr" "_" "LEAD;LEAD_INCL;LEAD_CHAR" "" )
    if ( pr_LEAD_INCL )
        set( pr_LEAD "${pr_LEAD_INCL}" )
     else()
        if ( pr__ )
            set( pr__ 2 )
         else()
            set( pr__ 3 )
        endif()
        if( DEFINED pr_LEAD_CHAR )
            string( REPEAT "${pr_LEAD_CHAR}" ${MSG_progEW} pr_LEAD )
         else()
            pad_lead( pr_LEAD pr__ )
        endif()
        embed_progress( ${pr__} pr_LEAD )
    endif()
    list( POP_FRONT CMAKE_MESSAGE_INDENT )
    list( PREPEND CMAKE_MESSAGE_INDENT ${pr_LEAD} )
    Msg( PROG_OK ${pr_UNPARSED_ARGUMENTS} )
    return( PROPAGATE CMAKE_MESSAGE_INDENT )
 endfunction( show_progress_info )

function( end_progress )
    cmake_parse_arguments( PARSE_ARGV 0 "" "" "LEAD;LEAD_INCL;LEAD_CHAR" "" )
    if ( DEFINED _LEAD )
        show_progress_info( _ LEAD "${_LEAD}" ${_UNPARSED_ARGUMENTS} ) # sorry, trick #17.23.42
    elseif( DEFINED _LEAD_INCL )
        show_progress_info( _ LEAD_INCL "${_LEAD_INCL}" ${_UNPARSED_ARGUMENTS} ) # sorry, trick #17.23.42
    else()
        show_progress_info( _ LEAD_CHAR "${_LEAD_CHAR}" ${_UNPARSED_ARGUMENTS} ) # sorry, trick #17.23.42
    endif()
    list( POP_FRONT CMAKE_MESSAGE_INDENT )
    Cache_Set( MSG_prog 0 )
    return( PROPAGATE CMAKE_MESSAGE_INDENT )
 endfunction( end_progress )

# internal progress helpers → called by Msg():
 function( get_current_lead gcl_LEAD )
    pad_number( ${MSG_progNW} DONE )
    pad_number( ${MSG_progNW} ${MSG_progCV} CURRENT )
    pad_number( ${MSG_progNW} ${MSG_progTV} TOTAL )
    string( CONFIGURE "${MSG_progEX}" ${gcl_LEAD} @ONLY )
    return( PROPAGATE ${gcl_LEAD} )
 endfunction()
 function( calculate_progress_done )
    # calculate current value (in case there is a solution)
    ifCalc( ${${MSG_progTV}} VAR DONE YES "( 100 * ${${MSG_progCV}} ) / ${${MSG_progTV}}" NO "0" )
    get_current_lead( _cpdt )
    embed_progress( 0 _cpdt )
    list( POP_FRONT CMAKE_MESSAGE_INDENT )
    list( PREPEND CMAKE_MESSAGE_INDENT "${_cpdt}" )
    return( PROPAGATE CMAKE_MESSAGE_INDENT )
endfunction( calculate_progress_done )

# internal building UI helpers
 function( pad_number width srcVar )
    if ( ARGC LESS 3 )
        set( tgtVar ${srcVar} )
     else()
        set( tgtVar ${ARGV2} )
    endif()
    if ( width GREATER 2 AND DEFINED ${srcVar} )
        string( LENGTH "${${srcVar}}" _l )
        if ( _l LESS width )
            calc( _l "${width} - ${_l}" )
            string( REPEAT " " ${_l} _l )
            set( ${tgtVar} "${_l}${${srcVar}}" )
        else()
            set( ${tgtVar} "${${srcVar}}" )
        endif()
    endif() # no action on weird arguments.
    return( PROPAGATE ${tgtVar} )
 endfunction( pad_number )
 function( pad_lead pl_LEAD pl_LEAD_ID ) # autoresize LEAD string
    string( LENGTH "${${pl_LEAD}}" _pLl )
    if ( _pLl GREATER MSG_progEW )
        dec( MSG_progEW TO _epml )
        string( SUBSTRING "${${pl_LEAD}}" 0 ${_epml} ${pl_LEAD} )
        string( CONCAT ${pl_LEAD} "${${pl_LEAD}}…" )
        if ( ${pl_LEAD_ID} EQUAL 3 ) # subinfo → gets id right-4
            set( ${pl_LEAD_ID} 4 )   # in any other cases, id stays.
        endif()
    else()
        list( GET MSG_progL 4 _pll )
        calc( _eplp "( ( ${MSG_progEW} - ${_pLl} ) >> 1 ) - 1" )
        calc( _eprp "${MSG_progEW} - ( ${_eplp} + 1 + ${_pLl} ) - 1" )
        if ( _eprp GREATER_EQUAL 0 )
            string( REPEAT "${_pll}" ${_eprp} _epr )
            string( PREPEND _epr " " )
        else()
            unset( _epr )
        endif()
        if ( _eplp GREATER_EQUAL 0 )
            string( REPEAT "${_pll}" ${_eplp} _pll )
            string( APPEND _pll " " )
        else()
            unset( _pll )
        endif()
        string( PREPEND ${pl_LEAD} "${_pll}" )
        string( APPEND ${pl_LEAD} "${_epr}" )
    endif()
    return( PROPAGATE ${pl_LEAD} ${pl_LEAD_ID} )
 endfunction( pad_lead )
 function( embed_progress id textVar )
    if( ARGC LESS 3 )
        set( _epIA ${MSG_prog} )
    else()
        set( _epIA ${ARGV2} )
    endif()
    if ( MSG_progL AND MSG_progR ) # empty lists are false
        if ( ${id} EQUAL 4 )
            list( GET MSG_progR ${id} _epl )
         else()
            list( GET MSG_progL ${id} _epl )
        endif()
        string( PREPEND ${textVar} "${_epl}" )
        list( GET MSG_progR ${id} _epr )
        string( APPEND ${textVar} "${_epr}" )
    endif()
    string( REPEAT " " ${_epIA} _epIA )
    string( APPEND ${textVar} "${_epIA}" )
    return( PROPAGATE ${textVar} )
endfunction( embed_progress )

#[[ List filtering is implemented in CMake.

    There was just no filter lists by lists.
    And, well, multi-level filtering in one command?

    → filter_list ←         filter one list by other lists according to
    ===============         different logically combineable criteria

    Input list variable:    1st positional parameter! (srcVar)
    Switches:               INCLUDE (default) | EXCLUDE, IN_PLACE
    Output variable names:  MATCHED_LIST, MISSED_LIST, MATCHED_CNT, MISSED_CNT, FIND_INDEX
    Filtering criteria:
        - string-based      BEGINS_WITH ENDS_WITH ANY_OF CONTAINS_ANY CONTAINS_ALL
                            MATCHES_ANY MATCHES_ALL
        - filepath-based    CHILD_OF_ANY CHILD_OF_ALL PARENT_OF_ANY PARENT_OF_ALL
                            ANY_PATH_OF HAS_ANY_RELATIVE
    Logical combining:      AND OR ( )

    INCLUDE|EXCLUDE operates just as expected.  The "wanted" and "unwanted" lists will
    contain the expected results, this switch is mainly interesting for in-place modification,
    as in_place finally puts the "matched list" back into the source variable. With EXCLUDE
    given, you simply turn around "matches vs. misses".

    IN_PLACE means: do not touch the source list. Otherwise, the source variable will be modified
    in-place to finally contain the matched list.
 ]]
 function( filter_list srcVar )
    list( APPEND FL_CRITS BEGINS_WITH ENDS_WITH ANY_OF ANY_PATH_OF PARENT_OF_ANY PARENT_OF_ALL CHILD_OF_ANY
                 CHILD_OF_ALL HAS_ANY_RELATIVE CONTAINS_ANY CONTAINS_ALL MATCHES_ANY MATCHES_ALL )
    cmake_parse_arguments( PARSE_ARGV 1 "FL"
        "AND;OR;INCLUDE;EXCLUDE;IN_PLACE"
        "MATCHED_LIST;MISSED_LIST;MATCH_CNT;MISS_CNT;FIND_INDEX"
        "${FL_CRITS}" )
    if ( FL_AND OR FL_OR OR "(" IN_LIST FL_UNPARSED_ARGUMENTS )
        filter_list_parse_result_fomula( ${ARGN} )
    else()
        filter_list_build_def_result()
    endif()
    # Are we operational?
    if ( "${FL_INCLUDE}" STREQUAL "${FL_EXCLUDE}" AND NOT FL_FIND_INDEX )
        message( FATAL_ERROR "filter list: cannot include and exclude at the same time!" )
    else() # commit the job …
        if ( FL_INCLUDE )
            set( match "matches" )
            set( miss "misses" )
         else()
            set( miss "matches" )
            set( match "misses" )
         endif()
        if ( FL_FIND_INDEX )
            set( ${FL_FIND_INDEX} "-1" )
         endif()
        list( LENGTH ${srcVar} _cnt )
        set( i 0 )
        set( matches "" )
        set( misses "" )
        while( i LESS _cnt )
            list( GET ${srcVar} ${i} _it )
            foreach( brot RES_BEGINS_WITH RES_ENDS_WITH RES_ANY_OF RES_ANY_PATH_OF
                          RES_PARENT_OF_ANY RES_PARENT_OF_ALL RES_CHILD_OF_ANY RES_CHILD_OF_ALL
                          RES_HAS_ANY_RELATIVE RES_CONTAINS_ANY RES_CONTAINS_ALL RES_MATCHES_ANY
                          RES_MATCHES_ALL )
                if ( "${brot}" MATCHES ".*_ALL" )
                    set( ${brot} 1 )
                else()
                    set( ${brot} 0 )
                endif()
            endforeach()
            if ( FL_BEGINS_WITH )
                foreach( _b IN LISTS FL_BEGINS_WITH )
                    string( FIND "${_it}" "${_b}" brot )
                    if ( brot EQUAL 0 )
                        set( RES_BEGINS_WITH 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if( FL_ENDS_WITH )
                foreach( _b IN LISTS FL_ENDS_WITH )
                    string( LENGTH "${_it}" _sl )
                    string( FIND "${_it}" "${_b}" brot REVERSE )
                    string( LENGTH "${_b}" _b )
                    math( EXPR brot "${_sl}-${_b}-${brot}" )
                    if ( brot EQUAL 0 )
                        set( RES_ENDS_WITH 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if( FL_ANY_OF )
                foreach( _b IN LISTS FL_ANY_OF )
                    if ( "${_b}" STREQUAL "${_it}" )
                        set( RES_ANY_OF 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if( FL_ANY_PATH_OF )
                foreach( _b IN LISTS FL_ANY_PATH_OF )
                    if ( _b PATH_EQUAL _it )
                        set( RES_ANY_PATH_OF 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_PARENT_OF_ANY )
                foreach( _b IN LISTS FL_PARENT_OF_ANY )
                    file_path( _it PARENT_OF _b brot )
                    if ( brot )
                        set( RES_PARENT_OF_ANY 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_PARENT_OF_ALL )
                foreach( _b IN LISTS FL_PARENT_OF_ALL )
                    file_path( _it PARENT_OF _b brot )
                    if ( NOT brot )
                        set( RES_PARENT_OF_ALL 0 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_CHILD_OF_ANY )
                foreach( _b IN LISTS FL_CHILD_OF_ANY )
                    file_path( _it CHILD_OF _b brot )
                    if ( brot )
                        set( RES_CHILD_OF_ANY 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_CHILD_OF_ALL )
                foreach( _b IN LISTS FL_CHILD_OF_ALL )
                    file_path( _it CHILD_OF _b brot )
                    if ( NOT brot )
                        set( RES_CHILD_OF_ALL 0 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_CONTAINS_ANY )
                foreach( _b IN LISTS FL_CONTAINS_ANY )
                    string( FIND "${_it}" "${_b}" matsch )
                    if( matsch GREATER -1 )
                        set( RES_CONTAINS_ANY 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_CONTAINS_ALL )
                foreach( _b IN LISTS FL_CONTAINS_ALL )
                    string( FIND "${_it}" "${_b}" matsch )
                    if( matsch LESS 0 )
                        set( RES_CONTAINS_ALL 0 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_MATCHES_ANY )
                foreach( _b IN LISTS FL_MATCHES_ANY )
                    string( REGEX MATCH "${_b}" matsch "${_it}" )
                    if( matsch )
                        set( RES_MATCHES_ANY 1 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_MATCHES_ALL )
                foreach( _b IN LISTS FL_MATCHES_ALL )
                    string( REGEX MATCH "${_b}" matsch "${_it}" )
                    if( NOT matsch )
                        set( RES_MATCHES_ALL 0 )
                        break()
                    endif()
                endforeach()
             endif()
            if ( FL_HAS_ANY_RELATIVE )
                foreach( _b IN LISTS FL_HAS_ANY_RELATIVE )
                    file_path( _it HAS_RELATIVE _b brot )
                    if ( brot )
                        set( RES_HAS_ANY_RELATIVE 1 )
                        break()
                    endif()
                endforeach()
             endif()
            # now there comes the magic … ahem … math …
            string( CONFIGURE "${FL_RESULT}" magic )
            calc( magic "${magic}" )
            if ( magic )
                if ( FL_FIND_INDEX )
                    set( ${FL_FIND_INDEX} "${i}" )
                    break()
                else()
                    list( APPEND ${match} ${_it} )
                endif()
            else()
                list( APPEND ${miss} ${_it} )
            endif()
            inc( i )
        endwhile()
        if ( NOT FL_FIND_INDEX )
            if ( FL_MATCH_CNT )
                list( LENGTH matches ${FL_MATCH_CNT} )
            endif()
            if ( FL_MISS_CNT )
                list( LENGTH misses ${FL_MISS_CNT} )
            endif()
            if ( FL_MISSED_LIST )
                set( ${FL_MISSED_LIST} ${misses} )
            endif()
            if ( FL_MATCHED_LIST )
                set( ${FL_MATCHED_LIST} "${matches}" )
            endif()
            if ( FL_IN_PLACE )
                set( ${srcVar} "${matches}" )
            endif()
        endif()
    endif()
    return( PROPAGATE ${srcVar} ${FL_MATCHED_LIST} ${FL_MISSED_LIST} ${FL_MATCH_CNT} ${FL_MISS_CNT} ${FL_FIND_INDEX} )
endfunction( filter_list )

#[[ filter_list helper functions.
    → build_def_math: build a math-string, evaluating the results of all declared operators
 ]]
 function( filter_list_build_def_result )
    set( FL_RESULT "0" )
    if ( FL_BEGINS_WITH )
        string( APPEND FL_RESULT [=[ | ${RES_BEGINS_WITH}]=])
    endif()
    if ( FL_ENDS_WITH )
        string( APPEND FL_RESULT [=[ | ${RES_ENDS_WITH}]=])
    endif()
    if ( FL_ANY_OF )
        string( APPEND FL_RESULT [=[ | ${RES_ANY_OF}]=])
    endif()
    if ( FL_ANY_PATH_OF )
        string( APPEND FL_RESULT [=[ | ${RES_ANY_PATH_OF}]=])
    endif()
    if ( FL_PARENT_OF_ANY )
        string( APPEND FL_RESULT [=[ | ${RES_PARENT_OF_ANY}]=])
    endif()
    if ( FL_PARENT_OF_ALL )
        string( APPEND FL_RESULT [=[ | ${RES_PARENT_OF_ALL}]=])
    endif()
    if ( FL_CHILD_OF_ANY )
        string( APPEND FL_RESULT [=[ | ${RES_CHILD_OF_ANY}]=])
    endif()
    if ( FL_CHILD_OF_ALL )
        string( APPEND FL_RESULT [=[ | ${RES_CHILD_OF_ALL}]=])
    endif()
    if ( FL_CONTAINS_ANY )
        string( APPEND FL_RESULT [=[ | ${RES_CONTAINS_ANY}]=])
    endif()
    if ( FL_CONTAINS_ALL )
        string( APPEND FL_RESULT [=[ | ${RES_CONTAINS_ALL}]=])
    endif()
    if ( FL_MATCHES_ANY )
        string( APPEND FL_RESULT [=[ | ${RES_MATCHES_ANY}]=])
    endif()
    if ( FL_MATCHES_ALL )
        string( APPEND FL_RESULT [=[ | ${RES_MATCHES_ALL}]=])
    endif()
    if ( FL_HAS_ANY_RELATIVE )
        string( APPEND FL_RESULT [=[ | ${RES_HAS_ANY_RELATIVE}]=])
    endif()
    string( REPLACE "0 |" "" FL_RESULT "${FL_RESULT}" )
    return( PROPAGATE FL_RESULT )
 endfunction( filter_list_build_def_result )
 function( filter_list_parse_nxt_arg )
    if ( ARGC EQUAL 1 )
        list( POP_FRONT FL_UNPARSED_ARGUMENTS ${ARGV0} )
    else()
        list( POP_FRONT FL_UNPARSED_ARGUMENTS )
    endif()
    if ( FL_UNPARSED_ARGUMENTS )
        list( GET FL_UNPARSED_ARGUMENTS 0 nxtArg )
    else()
        set( nxtArg "" )
    endif()
    return( PROPAGATE FL_UNPARSED_ARGUMENTS nxtArg ${ARGV0} )
 endfunction( filter_list_parse_nxt_arg )
 function( filter_list_parse_result_fomula )
    cmake_parse_arguments( PARSE_ARGV 0 "FL"
        "INCLUDE;EXCLUDE;IN_PLACE"
        "MATCH_CNT;MISS_CNT;MATCHED_LIST;MISSED_LIST;FIND_INDEX"
        "" )
    # FL_UNPARSED_ARGUMENTS should contain a list of
    # anything having to do with the filter parameters.
    set( propagate FL_RESULT )
    set( ao "AND;OR" )
    set( aoo "${ao};(")
    set( aoc "${ao};)")
    set( braces 0 )
    set( FL_RESULT "" )
    set( lstArg "" )
    while( FL_UNPARSED_ARGUMENTS )
        filter_list_parse_nxt_arg( arg )
        # Check token
        if ( "${arg}" STREQUAL "(" )
            # a Criterion should follow in any case, or another brace!
            if ( NOT ( ( "${nxtArg}" STREQUAL "(" ) OR ( nxtArg IN_LIST FL_CRITS ) ) )
                message( FATAL_ERROR "list_filter: argument parse error - opening brace not followed by a criterion." )
            else()
                string( APPEND FL_RESULT "( " )
                inc( braces )
            endif()
        elseif( "${arg}" STREQUAL ")" )
            # closing braces should either be last, or followed by either a criterion,
            # or a concatenation, or another closing brace.
            if ( NOT ( ( NOT nxtArg ) OR ( nxtArg IN_LIST FL_CRITS ) OR ( nxtArg IN_LIST aoc ) ) )
                message( FATAL_ERROR "list_filter: argument parse error - closinging brace neither last, nor followed by a criterion, operator, or another closing brace." )
            else()
                string( APPEND FL_RESULT " )" )
                dec( braces )
            endif()
        elseif( arg IN_LIST FL_CRITS )
            # previous: empty, op, opening_brace - following: list element (ignored)
            if ( NOT ( ( NOT lstArg ) OR ( lstArg IN_LIST aoo ) ) )
                Msg( FATAL "list_filter: argument parse error - criterion '${arg}' misplaced." )
            else()
                string( APPEND FL_RESULT "\$\{RES_${arg}\}" )
                while ( nxtArg AND NOT ( nxtArg IN_LIST aoc ) )
                    filter_list_parse_nxt_arg()
                endwhile()
                if ( nxtArg AND ( "${nxtArg}" STREQUAL ")" ) )
                    list( POP_BACK FL_${arg} )
                    list( APPEND propagate FL_${arg} )
                endif()
            endif()
        elseif ( arg IN_LIST ao ) # check both sides: prev := ")", CRIT, nxt := "(", CRIT
            if ( NOT ( ( lstArg IN_LIST FL_CRITS OR "${lstArg}" STREQUAL ")" ) AND
                       ( nxtArg IN_LIST FL_CRITS OR "${nxtArg}" STREQUAL "(" ) ) )
                message( FATAL_ERROR "list_filter: argument parse error - operator '${arg}' misplaced." )
            else()
                if( "${arg}" STREQUAL "AND" )
                    string( APPEND FL_RESULT " & " )
                elseif( "${arg}" STREQUAL "OR" )
                    string( APPEND FL_RESULT " | " )
                endif()
            endif()
        endif()
        set( lstArg "${arg}" )
    endwhile()
    if ( braces )
        mesage( FATAL_ERROR "list_filter: argument parse error - uneven number of opening and closing braces." )
    endif()
    return( PROPAGATE ${propagate} )
endfunction( filter_list_parse_result_fomula )

#[[
    JSON-based project-external git repository maintenance for CMake projects
    =========================================================================
    C0de: Stefan <St0fF> Kaps, 2026.
    LICENSE: APACHE-2.0
    -------------------------------------------------------------------------
    Maintenance is part of the "better.cmake" project.
    =========================================================================
    Uses all .json-files in LISTDIR for external repo maintenance.
 ]]
 set( CACHE{MAINTAIN_NEW_PATCHFILE} TYPE FILEPATH HELP "Set this cache entry to one filename at a time to append a patch to a defined repository. Then enable Maintenance Mode to apply!" VALUE "" )
 set( CACHE{MAINTAIN_UPSTREAM} TYPE BOOL HELP "Set this cache entry to 'on' for a forced upstream Repo-Maintenance." VALUE "off" )
 set( CACHE{MAINTENANCE_PROJECT_REPO_ROOT} TYPE INTERNAL VALUE "" )
 set( CACHE{MAINTENANCE_PROGRESS_STYLE} TYPE INTERNAL VALUE 1 )
#[[ MaintainSourceFromRepolist
   ============================
    ROOTDIR:    root folder for sources
    LISTDIR:    folder containing the repository JSON's
 ]]
 function( MaintainSourceFromRepolist )
    cmake_parse_arguments( PARSE_ARGV 0 "MTN" "" "ROOTDIR;LISTDIR" "" )
    Maintenance_init( "MaintainSourceFromRepolist" ) # DOES THE START MSG!!!
    if( MTN_HAS_CONFLICTS )
        Msg( DONE WARN LIST "MaintainSourceFromRepolist will not run until YOU REPAIRED THE MERGE CONFLICTS!;See '\${workspaceFolder}/Maintenance-Merge-Conflicts/' for backups!" )
    elseif( MTN_ERRORS )
        Msg( DONE FATAL PRE "MaintainSourceFromRepolist cannot run for these reasons:" LIST ${MTN_ERRORS} )
    elseif ( MTN_ACTIONS )
        Maintenance_work( "MaintainSourceFromRepolist" )
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( MaintainSourceFromRepolist )
#[[
    the following functionality is to be considered internal.  Changes without notice!

    ATTN: due to CMake requirements, the public script section remains at the bottom.
 ]]
function( Maintenance_init modeTxt )
    # prepare internal state transport
    set( MTN_VARS "MTN_VARS;MTN_ROOTDIR;MTN_LISTDIR;MTN_ERRORS;MTN_HAS_CONFLICTS;MTN_REPOFILE" )
    list( APPEND MTN_VARS CMAKE_MESSAGE_INDENT MTN_REPOS MTN_ACTIONS MTN_ACTION MTN_ACTIONS_ADDED
                 MTN_ACTIONS_POPPED MTN_PERCENT_DONE MTN_REASON MTN_REPO MTN_SUBACTION_CNT
                 MTN_ACTION_MODE MTN_IMPORTS MTN_IMPORTSl MTN_PATCHES MTN_PATCHESl MTN_BTFL
                 MTN_BTFLl MTN_IMPORTS MTN_PATCHES MAINTENANCE_MERGE_CONFLICT_FOLDER
                 MTN_REPO_FINISHED_ACTIONS )
    set( MTN_ERRORS "${MTN_ERRORS}" ) # import any previous state e.g. from script mode
    unset( MTN_ACTIONS )
    if ( NOT MAINTENANCE_PROJECT_REPO_ROOT )
        find_Git_root( MAINTENANCE_PROJECT_REPO_ROOT TREE "${MTN_ROOTDIR}" CACHE )
        if ( NOT MAINTENANCE_PROJECT_REPO_ROOT OR NOT EXISTS "${MAINTENANCE_PROJECT_REPO_ROOT}" )
            set( MAINTENANCE_PROJECT_REPO_ROOT "${CMAKE_SOURCE_DIR}" ) # fallback reporoot for .gitignore handling
        endif()
    endif()
    Maintenance_checkRemainingConflicts()
    if ( NOT ( MTN_ROOTDIR AND MTN_LISTDIR ) )
        list( APPEND MTN_ERRORS "maintenance NEEDs 'ROOTDIR' and 'LISTDIR' parameters!" )
     elseif ( NOT EXISTS "${MTN_LISTDIR}" )
        list( APPEND MTN_ERRORS "LISTDIR '${MTN_LISTDIR}' doesn't exist!" )
    endif()
    # begin pre-init, finding and loading repo files, quietly
    set( MTN_REPOS "" )
    file( GLOB repofiles "${MTN_LISTDIR}/*.json" )
    list( FILTER repofiles EXCLUDE REGEX "[.]schema[.]" )
    # read all repofiles found - produce content variables.
    foreach( MTN_REPOFILE IN LISTS repofiles )
        unset( repo_vars )
        Maintenance_LoadRepo( repo_vars )
        # remember repo names
        list( APPEND MTN_REPOS ${MTN_REPO} )
        # remember repo variables, also prepare repo gitignore-block-vars
        list( APPEND MTN_VARS ${repo_vars} )
    endforeach()
    if ( NOT MTN_REPOS )
        list( APPEND MTN_ERRORS "could not find any repository.json files inside ${MTN_LISTDIR}." )
    endif()
    # fish out "Action invocations", output init statement
    if ( MAINTAIN_NEW_PATCHFILE OR MAINTAIN_UPSTREAM OR MTN_RESET )
        if ( MAINTAIN_NEW_PATCHFILE ) # will be reset when actually consumed.
            cmake_path( GET MAINTAIN_NEW_PATCHFILE FILENAME _fn )
            list( APPEND MTN_ACTIONS "new patch ${_fn} added" )
        endif()
        if ( MAINTAIN_UPSTREAM )   # will be reset when actually consumed.
            list( APPEND MTN_ACTIONS "upstream check requested")
        endif()
        if ( MTN_RESET )
            if ( MTN_ONLY_REPO )
                list( APPEND MTN_ACTIONS "reset build-tree regarding repo '${MTN_ONLY_REPO}' requested" )
            else()
                list( APPEND MTN_ACTIONS "reset build-tree according to repo files requested" )
            endif()
        endif()
        Msg( START POST "… revising initial actions list" PRE "${modeTxt}: starting forced operation(s):" LEAD "\"" LIST ${MTN_ACTIONS} )
        unset( MTN_ACTIONS )
     else()
        Msg( START "${modeTxt} - preparing initial actions list" )
    endif()
    Msg( PRE "→ found repositories:" LIST ${MTN_REPOS} )
    Maintenance_LoadGI()
    # Now prepare internal data, so Maintenance operation scales to large repositories …
        # to not do too many globberations … read the build-tree ONCE, absolute paths list!
        file( GLOB_RECURSE MTN_BTFL "${MTN_ROOTDIR}/*" )
        list( LENGTH MTN_BTFL MTN_BTFLl )
        Msg( "→ found ${MTN_BTFLl} files in current build-tree, filtering imports …" )
        set( MTN_IMPORTS "" ) # list of all repo files
        set( MTN_PATCHES "" ) # list of all repo patched files
        foreach( MTN_REPO IN LISTS MTN_REPOS )
            string( TOUPPER "${MTN_REPO}" R )
            Msg( START PRE "→ Loaded:" LEAD "" LIST ${${R}_LOADED} )
            Maintenance_BeginImports()
            while( RIS_ACTIVE )
                while( RIS_SET_ACTIVE )
                    if ( ${_fn} IN_LIST ${R}_PATCHED )
                        list( APPEND MTN_PATCHES ${_ft} )
                        list( APPEND MTN_BTFL ${_fp} ) # add Folder, so it does not get deleted, even if patch file got lost...
                    else()
                        if( MAINTAIN_NEW_PATCHFILE AND ( "${MAINTAIN_NEW_PATCHFILE}" STREQUAL "${_ft}") )
                            Msg( "… found patch (${_fn}) to apply" )
                            addAction( ${MTN_REPO} ACTaddPF "add patched file '${_fn}'" )
                        endif()
                        list( APPEND MTN_IMPORTS ${_ft} ) # all targets added
                        list( REMOVE_ITEM MTN_BTFL "${_ft}" ) # imported files filtered out
                    endif()
                    Maintenance_NextImportItem()
                endwhile()
                Maintenance_NextImportSet()
            endwhile()
            Msg( DONE )
        endforeach()
        list( REMOVE_DUPLICATES MTN_BTFL )
        list( LENGTH MTN_BTFL MTN_BTFLl )
        list( LENGTH MTN_IMPORTS MTN_IMPORTSl )
        list( LENGTH MTN_PATCHES MTN_PATCHESl )
    Msg( "→ ${MTN_BTFLl} native build tree files and folders, ${MTN_PATCHESl} patched imports, and ${MTN_IMPORTSl} imported files" )
    # find out explicit invocations, populate list further
        set( pa "${MTN_ACTIONS}" )
        foreach( MTN_REPO IN LISTS MTN_REPOS )
            string( TOUPPER "${MTN_REPO}" R )
            if ( MTN_RESET OR MAINTAIN_UPSTREAM )
                if ( MTN_RESET AND ( ( NOT MTN_ONLY_REPO ) OR ( MTN_ONLY_REPO AND "${MTN_REPO}" STREQUAL "${MTN_ONLY_REPO}" ) ) )
                    addAction( ${MTN_REPO} ACTreset "reset repo requested" )
                endif()
                if( MAINTAIN_UPSTREAM )
                    addAction( ${MTN_REPO} ACTchkUP "upstream check requested" )
                endif()
            elseif( NOT pa ) # no prior actions (add patch)
                addAction( ${MTN_REPO} ACTcheck "maintain build tree, find problems" )
            endif()
        endforeach()
    set( CACHE{MAINTAIN_UPSTREAM} FORCE VALUE off ) # reset here, actions added!
    Msg( DONE "${modeTxt} initialized, ${MTN_ACTIONS_ADDED} initial action-steps queued.\n " )
    return( PROPAGATE ${MTN_VARS} )
 endfunction( Maintenance_init )

function( Maintenance_work modeTxt )
    startActions( ${modeTxt} )
    set( PREP "" ) # previous repository …
    while( MTN_ACTION GREATER -1 )
        # test if current repository changed
        if( NOT "${MTN_REPO}" IN_LIST PREP )
            if ( NOT "${MTN_REPO}" STREQUAL "ALL_REPOS" )
                string( TOUPPER "${MTN_REPO}" R )
                string( CONCAT MTN_REPOFILE "${MTN_LISTDIR}/${MTN_REPO}.json" )
                set( MTN_REPODIR "${CMAKE_CURRENT_BINARY_DIR}/${MTN_REPO}" )
                Msg( START "Repo maintenance starts on '${MTN_REPO}'" )
            endif()
            set( MTN_REPO_FINISHED_ACTIONS "" )
            set( PREP "${MTN_REPO}" )
        endif()
        list( GET MTN_ACTION_NAMES ${MTN_ACTION} nam )
        Msg( START "${nam} – reason(s):" LIST ${MTN_REASON} )
        # execute requested action
            if ( "${MTN_ACTION}" IN_LIST ACTcheck )
            checkNeededActions()
        elseif ( "${MTN_ACTION}" IN_LIST ACTreset )
            resetRepo()
        elseif ( "${MTN_ACTION}" IN_LIST ACTresGI )
            removeGIblock()
        elseif ( "${MTN_ACTION}" IN_LIST ACTresRT )
            resetKnownTag()
        elseif ( "${MTN_ACTION}" IN_LIST ACTaddPF )
            addPatchFile()
        elseif ( "${MTN_ACTION}" IN_LIST ACTchkUP )
            checkUpstream()
        elseif ( "${MTN_ACTION}" IN_LIST ACTsynUP )
            syncUpstream()
        elseif ( "${MTN_ACTION}" IN_LIST ACTwrtBT )
            writeBuildTree()
        elseif ( "${MTN_ACTION}" IN_LIST ACTupdCA )
            updateCache()
        elseif ( "${MTN_ACTION}" IN_LIST ACTupdGI )
            updateGIblock()
        elseif ( "${MTN_ACTION}" IN_LIST ACTupdRF )
            updateRepoState()
        elseif ( "${MTN_ACTION}" IN_LIST ACTwrtGI )
            Maintenance_SaveGI()
        endif()
        list( APPEND MTN_REPO_FINISHED_ACTIONS "${MTN_ACTION}" )
        nextAction( DONE "completed." )
        if( MTN_REPO AND NOT ( "${MTN_REPO}" IN_LIST PREP ) )
            Msg( DONE "Repo '${PREP}' maintained." )
        endif()
    endwhile()
    return( PROPAGATE ${MTN_VARS} )
 endfunction( Maintenance_work )

#[[ MaintenanceActions :: Provides the "actions" for the Maintenance.cmake script.
    ===============================================================================
    MTN_ACTIONS:        action-queue, X_list of lists

    Actions themselves are lists
        - which repository                  → MTN_REPO
        - which action                      → MTN_ACTION (numeric id or unset)
        - how many substeps are expected    → MTN_SUBSTEP_CNT
        - why the heck do it?               → MTN_REASON (a list of reasons)
 ]]
 # reset on include (top scope)
 set( MTN_ACTIONS_ADDED 0 )
 set( MTN_ACTIONS_POPPED 0 )
 set( MTN_SUBACTION_CNT 0 )
 set( MTN_ACTION_MODE "" )
#[[
    Action-IDs and their expected substep-count (as a formula)
    as well as a list of action names by id.
 ]]
 set( ACTcheck 0;\${\${r}_IMPORT_CNT}+2 ) # check repository state, record necessary actions
 set( ACTreset 1;\${\${r}_IMPORT_CNT}+3 ) # reset repository: erase imports and folders
 set( ACTresGI 2;0+1; ) # reset repo gitignore entry
 set( ACTresRT 3;0+1; ) # reset repo last known tag
 set( ACTaddPF 4;0+1; ) # add patch file to repo
 set( ACTchkUP 5;1+1; ) # check upstream for new tag
 set( ACTsynUP 6;\${\${r}_SCRIPT}+1; ) # sync upstream (one tick extra, maybe take it away later)
 set( ACTwrtBT 7;\${\${r}_IMPORT_CNT}+2*\${\${r}_IS_PATCHED};ACTsynUP )
 set( ACTupdCA 8;\${\${r}_IMPORT_NSCNT}+1;ACTsynUP ) # update cache-entries of this repo (imported named sets, updateScript)
 set( ACTupdGI 9;2+2; ) # update .gitignore-block of repo
 set( ACTupdRF 10;3+0; ) # save repo json file in case of changes
 set( ACTwrtGI 11;2+0; ) # write .gitignore file
 list( APPEND MTN_ACTION_NAMES "check build tree state" "reset repository state and imports"
    "remove gitignore block" "reset last known tag" "add new patch file" "check upstream tag"
    "sync from upstream" "produce build tree" "update cache entries" "update gitignore block"
    "update repo-file" "update gitignore file" )

#[[ building an action list: addAction(     <repo> <actionId> <reason> )
                             addActions(    <repo> <actionId>... <reason> )
 ]]
 function( addActions repo )
    cmake_parse_arguments( PARSE_ARGV 1 "" "" "" "" )
    list( POP_BACK _UNPARSED_ARGUMENTS reason )
    foreach( _it IN LISTS _UNPARSED_ARGUMENTS )
        addAction( ${repo} ${_it} "${reason}" )
    endforeach()
    return( PROPAGATE ${MTN_VARS} )
 endfunction( addActions )
 function( addAction repo idName reason )
    # deduplicate on insert - collect multiple reasons, but any action is made to be executed ONCE
    list( GET ${idName} 0 id ) # no X_ needed
    filter_list( MTN_ACTIONS FIND_INDEX existing BEGINS_WITH "${repo}${X_SEMI}${id}" )
    if ( existing GREATER -1 ) # existing action means: no dependency check necessary!
        X_list( GET MTN_ACTIONS ${existing} exact ) # unescape the action, so a list is read!
        list( SUBLIST exact 3 -1 reasons )
        list( SUBLIST exact 0 3 exact )
        if ( NOT "${reasons}" MATCHES ".*${reason}.*" )
            list( APPEND reasons "${reason}" )   # dedup messages, also.
        endif()
        list( APPEND exact "${reasons}" )
        X_delimit( exact )
        list_replace( MTN_ACTIONS "${exact}" AT ${existing} )
    else() # Action does not exist
        if ( "${repo}" STREQUAL "ALL_REPOS" )
            set( repoAct INCLUDE;PREPEND )
         else()
            set( repoAct EXCLUDE;APPEND )
        endif()
        # check action dependencies
        list( LENGTH ${idName} deps )
        if ( deps GREATER 2 )
            list( SUBLIST ${idName} 2 -1 deps )
            foreach( dep IN LISTS deps )
                list( GET ${dep} 0 did ) # dependency-id
                if ( "${repo}" STREQUAL "${MTN_REPO}" AND "${did}" IN_LIST MTN_REPO_FINISHED_ACTIONS ) # already executed...
                    continue()
                endif()
                filter_list( MTN_ACTIONS FIND_INDEX did BEGINS_WITH "${repo}${X_SEMI}${did}${X_SEMI}" )
                if ( did EQUAL -1 ) # dependency not in queue
                    addAction( ${repo} ${dep} ${reason} )
                endif()
            endforeach()
        endif()
        # separate ALL_REPOS from others, so sorting works right.
        list( POP_FRONT repoAct ra )
        filter_list( MTN_ACTIONS IN_PLACE ${ra} BEGINS_WITH "ALL_REPOS" MISSED_LIST ALL_REPOS )
        # produce expected subaction count
        list( GET ${idName} 1 cnt )
        string( TOUPPER "${repo}" r )   # To prepare the actual sub-action count,
        string( CONFIGURE "${cnt}" cnt )# configure current values into the formula string
        calc( cnt "${cnt}" )            # commit the calculation itself → result: subactions
        inc( MTN_ACTIONS_ADDED BY cnt )
        # sort list if necessary …
        list( APPEND MTN_ACTIONS "${repo}${X_SEMI}${id}${X_SEMI}${cnt}${X_SEMI}${reason}" )
        list( SORT MTN_ACTIONS COMPARE NATURAL )
        if ( ALL_REPOS )
            list( ${repoAct} MTN_ACTIONS ${ALL_REPOS} ) # no X_ as ALL_REPOS was not unlimited.
        endif()
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( addAction repo id reason ) # add. parameters possible!

# start acting …
 function( startActions modeTxt )
    if ( MTN_ACTIONS ) # initialize indentation stuff
        set( MTN_ACTION_MODE "${modeTxt}" )
        if ( CMAKE_SCRIPT ) # script mode already has indentation enabled.
            show_progress_info( LEAD "${MTN_PI_SCP}" FORCE "${modeTxt} starting" )
         else()
            begin_progress( TOTAL MTN_ACTIONS_ADDED CURRENT MTN_ACTIONS_POPPED
                EXPR "${MTN_PI_FILL}" LEAD_INCL "${MTN_PI_1}"
                LEFT "${MTN_PI_LEFT}" RIGHT "${MTN_PI_RIGHT}" INDENT_AFTER ${MTN_PIA}
                FORCE "${modeTxt} starting" )
        endif()
        nextAction()
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( startActions )
# pop next action with some output finalizing the previous… (unlisted arguments passed to Msg())
 function( nextAction )
    # first: finish previous action counting
    if ( DEFINED MTN_ACTION AND MTN_SUBACTION_CNT )
        inc( MTN_ACTIONS_POPPED BY MTN_SUBACTION_CNT )
        set( MTN_SUBACTION_CNT 0 )
    endif()
    Msg( ${ARGV} ) # previous action finished …
    # really pop next action.
    X_list( POP_FRONT MTN_ACTIONS MTN_REASON )
    if ( MTN_REASON )
        list( POP_FRONT MTN_REASON MTN_REPO )
        list( POP_FRONT MTN_REASON MTN_ACTION )
        list( POP_FRONT MTN_REASON MTN_SUBACTION_CNT )
     else() # no more actions …
        # now clean up indenting …
        end_progress( LEAD_CHAR "${MTN_PI_OUT}" DONE TEXT "${MTN_ACTION_MODE} finished." )
        unset( MTN_ACTION )
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( nextAction )
# give info about beginning a substep - output then count
 function( subInfo )
    Msg( ${ARGV} )
    Maintenance_subStep()
    return( PROPAGATE ${MTN_VARS} )
endfunction( subInfo )
# give info after finishing a substep - count, then output
 function( subReport )
    Maintenance_subStep()
    Msg( ${ARGV} )
    return( PROPAGATE ${MTN_VARS} )
endfunction( subReport )
# tell the system about some steps not taken / taken together | output optional!
 function( subOmit omittedCount )
    if ( omittedCount LESS_EQUAL MTN_SUBACTION_CNT )
        Maintenance_subStep( ${omittedCount} )
        if ( ${ARGN} )
            Msg( ${ARGN} )
        endif()
    else()
        subBad( ${ARGN} )   # consume all.
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( subOmit )
# tell the system about errors encountered, setting remaining sub-steps to "failed"
 function( subBad ) # output some failure, consume all other subs of this action!
    inc( MTN_ACTIONS_POPPED BY MTN_SUBACTION_CNT )
    set( MTN_SUBACTION_CNT "0" )
    Msg( ${ARGV} )
    return( PROPAGATE ${MTN_VARS} )
endfunction( subBad )
#[[
    ACTUAL ACTION IMPLEMENTATIONS FOLLOWING
]]
#[[ 0:checkNeededActions: checks the current repo state ]]
 function( checkNeededActions )
    # prüfe für das aktuelle MTN_REPO, ob:
    # → ein TAG bekannt ist oder es schon mal cloned wurde
    if ( NOT ${R}_TAG )
        # a check finding a new tag queues anything else about the files …
        addAction( ${MTN_REPO} ACTchkUP "no tag known" )
        set( _r " missing!" )
     else()
        set( _r " '${${R}_TAG}'" )
    endif()
    subInfo( "… checking last known Tag: ${_r}" )
    # → der GI - Block existiert
    if ( NOT MTN_GI_${R} )
        addAction( ${MTN_REPO} ACTupdGI ".gitignore-block missing" )
        set( _r "missing!" )
     else()
        set( _r "exists." )
    endif()
    subInfo( "… checking gitignore-block - ${_r}" )
    # → die benannten CACHE-Variablen gesetzt sind
    Maintenance_BeginImports()
    while( RIS_ACTIVE )
        # → prüfe, ob alle Importe existieren
        set( _r "" )
        while( RIS_SET_ACTIVE )
            if ( NOT EXISTS ${_ft} )
                set( needFile 1 )
                list( APPEND _r "file(s) missing" )
                break()
            endif()
            Maintenance_NextImportItem()
        endwhile( RIS_SET_ACTIVE )
        if ( RIS_NAME AND NOT DEFINED CACHE{${RIS_NAME}} )
            list( APPEND _r "cache entry missing" )
            addAction( ${MTN_REPO} ACTupdCA "cache entry '${RIS_NAME}' missing" )
        endif()
        list( JOIN _r ", " _r )
        if ( _r )
            string( PREPEND _r ": " )
        endif()
        if ( RIS_NAME )
            subInfo( "… checking cache-entries and imported files of set '${RIS_NAME}'${_r}" )
         else()
            subInfo( "… checking imported files of set #${RIS_SET}${_r}" )
        endif()
        Maintenance_NextImportSet()
    endwhile( RIS_ACTIVE )
    # needCheck → immer Beides. Sonst nur das Letztere …
    if ( needFile )
        addAction( ${MTN_REPO} ACTwrtBT "imported file(s) missing" )
    endif()
    # ☑ actions recorded.
    return( PROPAGATE ${MTN_VARS} )
endfunction( checkNeededActions )
#[[ 1:resetRepo: erases all imported files and left-over folders, adds further reset actions ]]
 function( resetRepo )
    # resetRepo contains multiple actions!
    addAction( ${MTN_REPO} ACTresGI "reset-repo" )
    addAction( ${MTN_REPO} ACTresRT "reset-repo" )

    # produce file and folder lists
    Msg( FORCE "collecting file and folder lists for removal" )
    Maintenance_BeginImports()
    while( RIS_ACTIVE )
        list( LENGTH repo_files _rfl )
        while( RIS_SET_ACTIVE)
            # Prüfe, ob es sich um ein Patched File handelt, oder
            # ein NEW_PATCH für dieses File eingefügt werden soll
            if ( NOT ( "${MAINTAIN_NEW_PATCHFILE}" PATH_EQUAL "${_ft}" OR ${_fn} IN_LIST ${R}_PATCHED ) )
                list( APPEND repo_files ${_ft} )
                if ( NOT _fp IN_LIST repo_folders )
                    list( APPEND repo_folders ${_fp} )
                endif()
            else()
                list( APPEND keep_folders ${_fp} )
            endif()
            Maintenance_NextImportItem()
        endwhile( RIS_SET_ACTIVE)
        list( LENGTH repo_files _cnt )
        calc( _cnt "${_cnt} - ${_rfl}" )
        subInfo( "collected import set #${RIS_SET}:'${RIS_NAME}': ${_cnt} files." )
        Maintenance_NextImportSet()
        # this should complete "repo_files" (prior to child-deletion)
    endwhile( RIS_ACTIVE )
    # exclude patch-folders and their children
    filter_list( repo_folders IN_PLACE EXCLUDE ANY_OF ${keep_folders} OR CHILD_OF_ANY ${keep_folders} )
    unset( _fp ) # expand repo_folders to contain all parents up until ROOTDIR
    foreach( _f IN LISTS repo_folders )
        while( NOT _f PATH_EQUAL MTN_ROOTDIR )
            file_path( GET _f PARENT_PATH _f )
            if ( NOT ( "${_f}" IN_LIST _fp OR "${_f}" IN_LIST repo_folders ) )
                list( APPEND _fp "${_f}" )
            endif()
        endwhile()
    endforeach()
    list( APPEND repo_folders "${_fp}" )
    # exclude all parents of build-tree files
    filter_list( repo_folders IN_PLACE EXCLUDE PARENT_OF_ANY ${MTN_BTFL} )
    # remove leafs from list
    filter_list( repo_folders IN_PLACE EXCLUDE CHILD_OF_ANY ${repo_folders} )
    list( LENGTH repo_files rf )
    subInfo( DONE "found ${rf} files to erase" )
    # clean repo-files from those that will get removed recursively
    filter_list( repo_files IN_PLACE EXCLUDE CHILD_OF_ANY ${repo_folders} )
    if ( NOT MTN_DRY ) # do the job …
        file( REMOVE_RECURSE ${repo_folders} ${repo_files} )
    endif()
    file_path( repo_folders RELATIVE_LIST ${MTN_ROOTDIR} repo_folders )
    subInfo( PRE "${MTN_DRY}removed left-over repo-imported folders:" MAXY 10 LIST ${repo_folders} )
    file_path( repo_files FILENAME_LIST repo_files )
    subInfo( PRE "${MTN_DRY}removed left-over repo-imported files:" MAXY 10 LIST ${repo_files} )
    return( PROPAGATE MTN_VARS ${MTN_VARS} )
endfunction( resetRepo )
#[[ 2:removeRepositoryGitignoreBlock: unsets the block, marks .gi dirty ]]
 function( removeGIblock )
    addAction( ALL_REPOS ACTwrtGI "repo block '${MTN_REPO}' removed" )
    if ( MTN_DRY )
        Msg( "would simply remove .gitignore-block from project's '.gitignore' …" )
    else()
        unset( MTN_GI_${R} ) # removes the current repo block
        dec( MTN_GI_CNT )
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( removeGIblock )
#[[ 3:resetRepositoryLastTag: sets the repositories know tag empty - queues repo-file-write
    → WHY? as part of a reset-process, it should not trigger upstream checks or syncs by itself,
      that »orchestration« needs to happen elsewhere!
 ]]
 function( resetKnownTag )
    if ( NOT "${${R}_TAG}" STREQUAL "" )
        addAction( ${MTN_REPO} ACTupdRF "tag was reset" )
        Msg( "${MTN_DRY}reset 'tag' (<${${R}_TAG}> ⇒ <unknown>)" )
        if ( NOT MTN_DRY )
            set( ${R}_NEW_TAG "" )
            list( APPEND MTN_VARS ${R}_NEW_TAG )
        endif()
    endif()
    return( PROPAGATE MTN_VARS ${MTN_VARS} )
endfunction( resetKnownTag )
#[[ 4:addNewPatchedFile: takes input from Cache to append a new patch to the config ]]
 function( addPatchFile )
    Maintenance_BeginImports()
    while( RIS_ACTIVE )
        if ( NOT RIS_NAME )
            calc( RIS_NAME "${i}" OUTPUT_FORMAT HEX )
        endif()
        while( RIS_SET_ACTIVE)
            if ( "${_ft}" PATH_EQUAL "${MAINTAIN_NEW_PATCHFILE}" )
                set( in_this_repo ${RIS_NAME} )
            endif()
            Maintenance_NextImportItem()
        endwhile( RIS_SET_ACTIVE)
        Maintenance_NextImportSet()
    endwhile( RIS_ACTIVE )
    file_path( GET MAINTAIN_NEW_PATCHFILE FILENAME _fn )
    if ( in_this_repo AND EXISTS "${MAINTAIN_NEW_PATCHFILE}" ) # Patch added?
        addAction( ${MTN_REPO} ACTupdGI "patch added" )
        addAction( ${MTN_REPO} ACTupdRF "patch added" )
        subReport( "${MTN_DRY}add patch '${_fn}' to patches - found inside import set '${in_this_repo}'" )
        if ( NOT MTN_DRY )
            list( APPEND ${R}_PATCHED ${_fn} )
            inc( ${R}_PATCH_CNT )
        endif()
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( addPatchFile )
#[[ 5:checkUpstreamRepoForNewTags: … what it says … ]]
 function( checkUpstream )
    init_Git()
    # Vereinfachung des Ganzen…
    set( _Uri "${${R}_LOGIN}:${${R}_USER}/${MTN_REPO}.git" )
    string( REGEX REPLACE "(.+)" ": (filter: '\\1')" _flt "${${R}_FILTER}" )
    subInfo( FORCE "querying '${MTN_REPO}' for latest tags${_flt}" )
    cmd( OUT_VAR _tags DO ${GIT_EXECUTABLE} ls-remote --tags --sort=-v:refname --refs ${_Uri} ${${R}_FILTER} )
    string( REPLACE "\n" ";" _tags "${_tags}" )
    list( POP_BACK _tags )
    list( TRANSFORM _tags REPLACE "\t" " " ) # git liefert "Hash\tTag"
    list( LENGTH _tags _tc )
    subInfo( PRE "received ${_tc} tags: " LIST ${_tags} )
    # erste Zeile = neuester Tag → parsen
    list( GET _tags 0 ${R}_NEW_TAG )
    string( REGEX REPLACE "^(.*refs/tags/)" "" ${R}_NEW_TAG "${${R}_NEW_TAG}" )
    # nun können wir mit Vergleichen beginnen …
    if ( ( NOT DEFINED ${R}_TAG ) OR ( NOT ( "${${R}_TAG}" STREQUAL "${${R}_NEW_TAG}" ) ) )
        set( _tx "${${R}_NEW_TAG}" )
        list( APPEND MTN_VARS ${R}_NEW_TAG )
        addAction( ${MTN_REPO} ACTwrtBT "new tag upstream" )
        addAction( ${MTN_REPO} ACTupdRF "tag changed" )
    else()
        set( _tx "${${R}_TAG} - no changes upstream!" )
    endif()
    subReport( DONE "tag-check yielded: ${_tx}" )
    return( PROPAGATE MTN_VARS ${MTN_VARS} )
endfunction( checkUpstream )
#[[ 6:syncRepositoryFromUpstream: … what it says … ]]
 function( syncUpstream )
    if ( ${R}_NEW_TAG OR ${R}_TAG )
        init_Git()
        set( _Uri "${${R}_LOGIN}:${${R}_USER}/${MTN_REPO}.git" )
        # as we will clone, the requirement is: folder shall not exist beforehand.
        if ( EXISTS ${MTN_REPODIR} ) # DRY RUN doesn't affect BINARY_DIR
            file( REMOVE_RECURSE ${MTN_REPODIR} )
            Msg( "… removed existing clone" )
        endif()
        # clone upstream repositiory - which Tag?
        set( _TAG "${${R}_NEW_TAG}" )
        if ( NOT _TAG )
            set( _TAG "${${R}_TAG}" )
        endif()
        cmd( WORKDIR ${CMAKE_CURRENT_BINARY_DIR} RES_VAR GIT_CLONED ERR_VAR GIT_ERROR OUT_VAR GIT_OUT
             DO ${GIT_EXECUTABLE} --no-lazy-fetch --no-advice clone --depth 1 --progress --branch "${_TAG}" ${_Uri} "${MTN_REPODIR}"
             ECHO_OUTPUT_VARIABLE )
        if ( GIT_CLONED GREATER_EQUAL 0 )
            subReport( "… git clone TAG '${_TAG}' succeeded" )
        else()
            subBad( WARN "git clone failed: ${GIT_ERROR} - cannot sync build tree" )
        endif()
    else()
        subBad( "cannot sync without knowing a tag" )
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( syncUpstream )
#[[ 7:writeBuildTree: … what it says … ]]
 function( writeBuildTree )
    # JSON-Listen abarbeiten - Dateien in den Builtree integrieren
    # → Voraussetzung: Repo is cloned.
    if ( NOT EXISTS ${MTN_REPODIR} )
        subBad( "repo seems not to be synced - cannot produce build tree" )
    else()
        unset( patches )
        Maintenance_BeginImports()
        while( RIS_ACTIVE )
            while( RIS_SET_ACTIVE)
                cmake_path( ABSOLUTE_PATH _f BASE_DIRECTORY "${MTN_REPODIR}" NORMALIZE OUTPUT_VARIABLE _fs )
                # Prüfe, ob es sich um ein Patched File handelt
                if ( ${_fn} IN_LIST ${R}_PATCHED )
                    list( APPEND patches ${_ft} ${_f} ) # mine, theirs
                elseif( NOT MTN_DRY )
                    cmake_path( GET _ft PARENT_PATH _fp )
                    if ( NOT EXISTS "${_fp}" )
                        file( MAKE_DIRECTORY ${_fp} )
                    endif()
                    file( COPY_FILE "${_fs}" "${_ft}" RESULT _r ONLY_IF_DIFFERENT INPUT_MAY_BE_RECENT )
                    if ( _r )
                        Msg( WARN "copying file '${_fn}' yielded error: ${_r}" )
                    endif()
                endif()
                Maintenance_NextImportItem()
            endwhile( RIS_SET_ACTIVE)
            if ( RIS_NAME )
                subInfo( "${MTN_DRY}copied import set '${RIS_NAME}' into '${RIS_TGT}' … ${RIS_ITEMS} items" )
             else()
                subInfo( "${MTN_DRY}copied import set #${RIS_SET} into '${RIS_TGT}' … ${RIS_ITEMS} items" )
            endif()
            Maintenance_NextImportSet()
        endwhile( RIS_ACTIVE )
        # 3-way-merge der gepatchten Dateien…
        if ( patches )
            unset( _pat )
            unset( _cft )
            unset( _err )
            unset( GIT_CLONED )
            if( DEFINED ${R}_TAG AND NOT "${${R}_TAG}" STREQUAL "" )
                Msg( FORCE "starting 3-way-merge – pulling previous TAG" )
                # Fetch old tag once per repo
                cmd( WORKDIR ${MTN_REPODIR} RES_VAR GIT_CLONED ERR_VAR GIT_ERROR OUT_VAR GIT_OUT
                    DO ${GIT_EXECUTABLE} fetch --progress --depth 1 origin refs/tags/${${R}_TAG}
                    ECHO_OUTPUT_VARIABLE )
                if ( GIT_CLONED LESS 0 )
                    subReport( DONE WARN "cloning base for a 3-way-merge failed: ${GIT_ERROR}" )
                else()
                    subReport( "successfully cloned base." )
                    while( patches )
                        list( POP_FRONT patches mf )
                        list( POP_FRONT patches tf )
                        file_path( GET tf FILENAME _fn )
                        if ( DEFINED GIT_CLONED )
                            file_path( ABSOLUTE_PATH _fn BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE bf )
                            # base beschaffen
                            cmd( WORKDIR ${MTN_REPODIR} RES_VAR _fetched ERR_VAR _fetch_err
                                 DO ${GIT_EXECUTABLE} show FETCH_HEAD:${tf} OUTPUT_FILE ${bf} ECHO_ERROR_VARIABLE )
                            if ( NOT _fetched EQUAL 0 )
                                list( APPEND _err ${tf} )
                                Msg( WARN "… base für '${tf}' nicht beschaffbar: ${_fetch_err}" )
                            else()
                                # secure merge → missing files "become empty" …
                                if ( EXISTS "${mf}" )
                                    file( READ ${mf} _mfc )
                                 else()
                                    unset( _mfc )
                                endif()
                                # merge durchführen → unterschiedliche Vorgehensweise basierend auf MTN_DRY!
                                cmd( WORKDIR ${MTN_REPODIR} RES_VAR _merged
                                     DO ${GIT_EXECUTABLE} merge-file ${mf} ${bf} ${tf} )
                                if ( _merged GREATER 0 )
                                    # Konflikt: NICHT in die echte Quelle, sondern separat ablegen
                                    file( MAKE_DIRECTORY "${MAINTENANCE_MERGE_CONFLICT_FOLDER}" )
                                    list( APPEND _cft ${tf} )
                                    if ( MTN_DRY ) # DRY MODE: move the conflicted file, restore build tree
                                        file( RENAME "${mf}" "${MAINTENANCE_MERGE_CONFLICT_FOLDER}/${_fn}" )
                                        file( WRITE "${mf}" "${_mfc}" )
                                    else() # else: save original from built tree to conflict folder
                                        file( WRITE "${MAINTENANCE_MERGE_CONFLICT_FOLDER}/${_fn}" "${_mfc}" )
                                    endif()
                                elseif ( _merged LESS 0 )
                                    file( WRITE "${mf}" "${_mfc}" ) # restore prev. content (…)
                                    list( APPEND _err ${tf} )
                                else()
                                    if ( MTN_DRY )
                                        file( WRITE "${mf}" "${_mfc}" ) # restore prev. content (…)
                                    endif()
                                    list( APPEND _pat ${tf} )
                                endif()
                                file( REMOVE ${bf} )
                            endif()
                        else() #[[ !no previous tag found - fallback mechanism!
                            ----------------------------------------------------
                            →   patched source will be tracked by our repo, so they simply SHOULD EXIST! ]]
                            if ( EXISTS ${mf} )
                                list( APPEND _pat ${tf} )
                            else()
                                list( APPEND _err ${tf} )
                            endif()
                        endif()
                    endwhile()
                    # Ergebnisanalyse / -ausgabe
                    if ( NOT _pat AND NOT _cft ) # no success at all
                        subReport( PRE "… merging failed: " LIST ${_err} DONE )
                    elseif( NOT _pat ) # no success, but conflicts
                        if( _err ) # and errors
                            Msg( PRE "… merge fails: " LIST ${_err} )
                        endif()
                        subReport( PRE "… merge conflicts: " LIST ${_cft} DONE )
                    else()
                        if ( _err )
                            Msg( PRE "… merge failures: " LIST ${_err} )
                        endif()
                        if ( _cft )
                            Msg( PRE "… merge conflicts: " LIST ${_cft} )
                        endif()
                        subReport( PRE "… merged successfully: " LIST ${_pat} DONE )
                    endif()
                endif()
            endif()
        endif()
        # after writing the build tree further entries need updating.
        addAction( ${MTN_REPO} ACTupdGI "buildtree rewritten" )
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( writeBuildTree )
#[[ 8:updateRepositoryCacheVariables: … what it says … ]]
 function( updateCache )
    Maintenance_BeginImports()
    while( RIS_ACTIVE )
        if ( RIS_NAME )
            unset( tlist )
            while( RIS_SET_ACTIVE)
                list( APPEND tlist "${_fb}" )
                Maintenance_NextImportItem()
            endwhile( RIS_SET_ACTIVE)
            set( CACHE{${RIS_NAME}} TYPE INTERNAL FORCE VALUE ${tlist} )
            subReport( "… entry '${RIS_NAME}' updated" )
        endif()
        Maintenance_NextImportSet()
    endwhile( RIS_ACTIVE )
    # update-script ausführen
    file_path( REPLACE_EXTENSION MTN_REPOFILE "cmake" OUTPUT_VARIABLE _SCRIPT )
    if ( ${${R}_SCRIPT} AND IS_READABLE ${_SCRIPT} )
        Msg( FORCE "running updateScript" ) # no sub-info, here …
        #[[
            AT THIS POINT, A GIVEN SCRIPT IS EXECUTED DIRECTLY VIA "include()"
            → this propagates the whole CMAKE scope stack to the called script!
            → repo, R, all ${R}_* variables, all MTN_* variables will be visible by the script
        ]]
        include( ${_SCRIPT} )
        subReport( DONE "updateScript finished." )
    endif()
endfunction( updateCache ) # no final propagation needed → changes CACHE entries!
#[[ 9:: ]]
 function( updateGIblock )
    addAction( ALL_REPOS ACTwrtGI "repo block '${MTN_REPO}' updated" )
    # recollect SUBDIRS, REPO_FILES, and OVERRIDES
    set( SUBDIRS "" )
    set( OVERRIDES "" )
    set( REPO_FILES "" )
    Maintenance_BeginImports()
    while( RIS_ACTIVE )
        while( RIS_SET_ACTIVE)
            if ( NOT ( "${_fn}" IN_LIST ${R}_PATCHED ) )
                list( APPEND REPO_FILES "${_ft}" )
            else()
                list( APPEND OVERRIDES ${_ft} )
            endif()
            if ( NOT "${_fp}" IN_LIST SUBDIRS )
                list( APPEND SUBDIRS "${_fp}" )
            endif()
            Maintenance_NextImportItem()
        endwhile( RIS_SET_ACTIVE)
        Maintenance_NextImportSet()
    endwhile( RIS_ACTIVE )
    list( LENGTH OVERRIDES _cnt )
    subReport( "… found ${_cnt} files in need of explicit inclusion" )
    # filter SUBDIRS using the filesystem state
    filter_list( SUBDIRS IN_PLACE EXCLUDE PARENT_OF_ANY ${MTN_BTFL} )
    # filter SUBDIRS to only include leaf-level items
    filter_list( SUBDIRS IN_PLACE EXCLUDE CHILD_OF_ANY ${SUBDIRS} )
    list( LENGTH SUBDIRS _cnt )
    subReport( "… found ${_cnt} leaf-level subdirs to exclude" )
    # last pre-step: produce EXTRA_FILES list … (non-patched, non-excluded)
    set( EXTRA_FILES "" )
    foreach( _f IN LISTS REPO_FILES )
        file_path( GET _f FILENAME _fn )
        if ( NOT ${_fn} IN_LIST ${R}_PATCHED )
            set( _fp ${_f} )
            # prüfe, ob eine Target-Folder der Datei erfasst ist
            set( match false )
            while ( NOT ${_fp} PATH_EQUAL ${MTN_ROOTDIR} )
                file_path( GET _fp PARENT_PATH _fp )
                if ( "${_fp}" IN_LIST SUBDIRS )
                    set( match true )
                    break()
                endif()
            endwhile()
            if ( NOT match )
                list( APPEND EXTRA_FILES ${_f} )
            endif()
        endif()
    endforeach()
    list( LENGTH EXTRA_FILES _cnt )
    subReport( "… found ${_cnt} files in need of explicit exclusion" )
    makeGIrepoBases() # set BS | BE variables to current MTN_REPO
    # final list transformations - remove MAINTENANCE_PROJECT_REPO_ROOT
    path_list( SUBDIRS MAKE_RELATIVE "${MAINTENANCE_PROJECT_REPO_ROOT}" APPEND_STRING "/**" )
    path_list( EXTRA_FILES MAKE_RELATIVE "${MAINTENANCE_PROJECT_REPO_ROOT}" )
    path_list( OVERRIDES MAKE_RELATIVE "${MAINTENANCE_PROJECT_REPO_ROOT}" PREPEND_STRING "!" )
    list( APPEND SUBDIRS ${EXTRA_FILES} ${OVERRIDES} )
    list( APPEND BS ${SUBDIRS} ${BE} )
    list( LENGTH SUBDIRS _cnt )
    subReport( "… produced ${_cnt} lines for ${MTN_REPO}'s .gitignore-block" )
    set( MTN_GI_${R} ${BS} )
    return( PROPAGATE ${MTN_VARS} )
endfunction( updateGIblock )
#[[ 10:UpdateRepoState: using current MTN and R - settings: update JSON file!

    Reads and modifies the Repository JSON file.
    !! Automatically detects and resets CACHE{MTN_NEW_PATCH} !!
    (( BUT it must have already been added to the repo from  ))
    (( outside, see Maintenance and patchRepo implementations ))
 ]]
 function( UpdateRepoState )
    file( READ "${MTN_REPOFILE}" _json )
    # the new Patch should already reside inside ${R}_PATCHED
    # → so there are multiple signs, that the new patch belongs to this repo
    #   - filename inside ${R}_PATCHED ←REQUIREMENT!
    file_path( GET MAINTAIN_NEW_PATCHFILE FILENAME _fn )
    if ( "${_fn}" IN_LIST ${R}_PATCHED )
        subInfo( "… updating new patch file '${_fn}'" )
        set( _i "[]" )
        foreach( _f IN LISTS ${R}_PATCHED )
            string( JSON _i SET "${_i}" 31337 "\"${_f}\"" )
        endforeach()
        string( JSON _json SET "${_json}" "patched" "${_i}" )
        set( CACHE{MAINTAIN_NEW_PATCHFILE} FORCE VALUE "" ) # consumed!
    else()
        Maintenance_subStep() # simply take the step anyhow
    endif()
    if ( DEFINED ${R}_NEW_TAG )
        subInfo( "… updating new TAG '${${R}_NEW_TAG}'" )
        string( JSON _json SET "${_json}" "tag" "\"${${R}_NEW_TAG}\"" )
        set( ${R}_TAG "${${R}_NEW_TAG}" )
        unset( ${R}_NEW_TAG )
    else()
        Maintenance_subStep() # simply take the step anyhow
    endif()
    subInfo( "${MTN_DRY}finally save repo-file" )
    if ( NOT MTN_DRY )
        file( WRITE "${MTN_REPOFILE}" "${_json}" )
    endif()
    return( PROPAGATE ${MTN_VARS} )
endfunction( UpdateRepoState )


# prepare progress-indentation - examples: large scale: up to 999 actions, small scale: percentage
if ( MAINTENANCE_PROGRESS_STYLE EQUAL 0 )
 set( MTN_PI_FILL "@DONE@% -@CURRENT@/@TOTAL@" ) # numbers will be made 3 chars wide!
 set( MTN_PI_1     "╔ MAINTENANCE ╗  " )
 set( MTN_PIA 2 )
 set( MTN_PI_LEFT  "║;╔;╚;╠;═" )
 set( MTN_PI_RIGHT "║;╗;╝;╣;┇" )
 set( MTN_PI_OUT    "═" )
 set( MTN_PI_SCP    "═ PROGRESS: ═" )
 elseif( MAINTENANCE_PROGRESS_STYLE EQUAL 1 ) # much simpler …
 set( MTN_PI_FILL "@DONE@%" ) # numbers will be made 3 chars wide!
 set( MTN_PI_1 "╭────╮ MAINTENANCE: " )
 set( MTN_PIA 2 )
 set( MTN_PI_LEFT  "│;╭;╰;├;─" )
 set( MTN_PI_RIGHT "│;╮;╯;┤;┆" )
 set( MTN_PI_OUT "─" )
 set( MTN_PI_SCP ":OK:" ) # UTF-mb-chars are counted wrongly - WTF is CMake not UTF-aware?
 endif()
if ( CMAKE_SCRIPT ) # start indentation right away in Maintenance-Script-Mode.
    cmake_path( GET CMAKE_SCRIPT_MODE_FILE FILENAME SMFN )
    if ( "${SMFN}" MATCHES "MaintainScript.cmake" )
        begin_progress( TOTAL MTN_ACTIONS_ADDED CURRENT MTN_ACTIONS_POPPED
            EXPR "${MTN_PI_FILL}" LEAD_INCL "${MTN_PI_1}"
            LEFT "${MTN_PI_LEFT}" RIGHT "${MTN_PI_RIGHT}" INDENT_AFTER ${MTN_PIA}
            " loading in script mode …" )
    endif()
endif()

#[[ check if merge-conflics "remain" ]]
 function( Maintenance_checkRemainingConflicts )
    set( MAINTENANCE_MERGE_CONFLICT_FOLDER "${MAINTENANCE_PROJECT_REPO_ROOT}/Maintenance-Merge-Conflicts" )
    # → prüfe, ob der User alle Merge-Konflikte "behoben" hat
    if ( EXISTS ${MAINTENANCE_MERGE_CONFLICT_FOLDER} )
        file( GLOB _mmc "${MAINTENANCE_MERGE_CONFLICT_FOLDER}/*" )
        list( LENGTH _mmc _mmcl )
        if ( _mmcl EQUAL 0 )
            file( REMOVE_RECURSE "${MAINTENANCE_MERGE_CONFLICT_FOLDER}" )
            set( MTN_HAS_CONFLICTS 0 )
        else()
            set( MTN_HAS_CONFLICTS 1 )
        endif()
    else()
        set( MTN_HAS_CONFLICTS 0 )
    endif()
    return( PROPAGATE MTN_HAS_CONFLICTS MAINTENANCE_MERGE_CONFLICT_FOLDER )
endfunction( Maintenance_checkRemainingConflicts )

#[[ Maintenance_LoadRepo – Read Repository JSON and set up Variables from file MTN_REPOFILE

    → MTN_REPO      Reponame (whatever the user chose for a name)
    → R             Reponame in UPPERCASE (for variable access)
    → ${R}_LOGIN    git provider login (e.g. "git@github.com")
    → ${R}_USER     git provider user name of the repository
    → ${R}_FILTER   tag-filter (e.g. "*docking" - see the asterisk!)
    → ${R}_TAG      last tag used to populate the current build tree
    → ${R}_SCRIPT   script to execute post-git-clone (to fulfill repo-special needs)
    → ${R}_LOADED   report message produced during loading the repository JSON file
                    → quiet loading, delayed reporting …
    → ${R}_IMPORT_CNT, ${R}_IMPORT_NSCNT    import set count, named import set count
    → ${R}_IMPORT_N                         import lists (unnamed, numbered)
    → ${R}_PATCH_CNT, ${R}_PATCHED          number of patches, file name list of patches

    Import Set processing:
    ----------------------
    Import sets are read from JSON, but not stored as specific variables unless really needed.
    Instead, the read JSON strings for each import and the number of import sets are stored
    within CMake Variables (see above) for latter interpretation.

    The iterator-approach using Maintenance_BeginImports() and Maintenance_NextImportSet()/Maintenance_NextImportItem()
    can be used to access sets and items in succession.  See below for details.
 ]]
 function( Maintenance_LoadRepo propagated )
    file( READ "${MTN_REPOFILE}" json )
    string( JSON MTN_REPO GET "${json}" "repo" )
    string( TOUPPER "${MTN_REPO}" R )
    string( JSON ${R}_USER GET "${json}" "user" )
    JSON( GET ${R}_FILTER "filter" "" "${json}" )
    JSON( GET ${R}_TAG "tag" "" "${json}" )
    JSON( GET ${R}_SCRIPT "updateScript" false "${json}" )
    bool( ${R}_SCRIPT )
    string( JSON ${R}_LOGIN ERROR_VARIABLE _err GET "${json}" "login" )
    if ( _err )
        list( APPEND MTN_ERRORS "repository '${MTN_REPO}': missing login data – don't know how to query!" )
    endif()
    set( ${propagated} "${R}_USER;${R}_FILTER;${R}_TAG;${R}_IMPORT_CNT;${R}_PATCH_CNT;${R}_PATCHED;${R}_IS_PATCHED;${R}_SCRIPT;${R}_LOADED;${R}_LOGIN;${R}_IMPORT_NSCNT" )
    # Imports lesen
    JSON( LENGTH ${R}_IMPORT_CNT "import" 0 "${json}" )
    set( i 0 )
    set( ${R}_IMPORT_NSCNT 0 )
    while( i LESS ${${R}_IMPORT_CNT} )
        string( JSON ${R}_IMPORT_${i} GET "${json}" "import" ${i} )
        list( APPEND ${propagated} "${R}_IMPORT_${i}" )
        JSON( GET n "name" "" "${${R}_IMPORT_${i}}" )
        if( n MATCHES ".+" ) # named set?
            inc( ${R}_IMPORT_NSCNT )
        endif()
        inc( i )
    endwhile()
    # PATCHES lesen
    JSON( LENGTH ${R}_PATCH_CNT "patched" 0 "${json}" )
    set( ${R}_IS_PATCHED 0 )
    set( i 0 )
    while( i LESS ${R}_PATCH_CNT )
        string( JSON _f GET "${json}" "patched" ${i} )
        list( APPEND ${R}_PATCHED ${_f} )
        inc( i )
        set( ${R}_IS_PATCHED 1 )
    endwhile()
    set( ${R}_LOADED "repo '${MTN_REPO}';user='${${R}_USER}';filter='${${R}_FILTER}';last tag='${${R}_TAG}';import-lists: ${${R}_IMPORT_CNT}" )
    if ( ${R}_SCRIPT )
        list( APPEND ${R}_LOADED "updateScript" )
    endif()
    if ( ${R}_PATCH_CNT GREATER 0 )
        list( TRANSFORM ${R}_PATCHED REPLACE "^(.+)$" "'\\1'" OUTPUT_VARIABLE _i )
        list( APPEND ${R}_LOADED ${_i} )
    else()
        list( APPEND ${R}_LOADED "no patched files" )
    endif()
    return( PROPAGATE MTN_ERRORS MTN_REPO R ${propagated} ${${propagated}} )
endfunction( Maintenance_LoadRepo )

#[[ Import Set Handling
    ===================
    Import sets are loaded as plain JSON without further parsing by Maintenance_LoadRepo().

    Operations on import sets mostly require traversing them one after the other.
    Conclusively, this looks very much like an iterator pattern …

    So - we'll present import set traversal functions:
    → Maintenance_BeginImports()        sets up the traversal in the caller's scope, using the
                                        MTN_REPO / R global variables.
    → Maintenance_NextImportSet()       switch to the next import set
    → Maintenance_NextImportItem()      switch to the next item in the file list

    And the respective variables - as already noted, those will be set in the caller's scope!
    This means: you cannot set up this iterator, then return from a function call to use the
    iterator from outside without propagating the following variables:
    → RIS_SET:          integer, kind of internal
    → RIS_ITEM:         integer, kind of internal
    → RIS_ACTIVE:       boolean (true, as long as there are more items to pull)
    → RIS_SET_ACTIVE:   boolean, indicates end of current set

    Usage: simplest loop
        Maintenance_BeginImports()
        while( RIS_ACTIVE )
            … do something with set
            while( RIS_SET_ACTIVE)
                … do something with files
                Maintenance_NextImportItem()
            endwhile( RIS_SET_ACTIVE)
            Maintenance_NextImportSet()
        endwhile( RIS_ACTIVE )

    Working variables are internal "_f*" vars - just to make sure you know they're internal!
    →   _f      repo-relative path as found in your JSON file
    →   _fn     plain file name without path (name)
    →   _fp     build-tree parent path of the file (parent)
    →   _ft     build-tree target file path (target)
    →   _fb     relative path to MTN_ROOTDIR (base)
 ]]
 list( APPEND RIS   RIS RIS_SET RIS_SET_ACTIVE RIS_ACTIVE RIS_ITEM RIS_TGT RIS_ABSTGT RIS_NAME
                    RIS_STRIP RIS_ITEMS _f _fn _fp _ft _fb )
 function( Maintenance_BeginImports )
    set( RIS_SET -1 )
    set( RIS_ITEM -1 )
    set( RIS_ACTIVE 0 )
    set( RIS_SET_ACTIVE 0 )
    Maintenance_NextImportSet()
    return( PROPAGATE ${RIS} )
 endfunction( Maintenance_BeginImports )
 function( Maintenance_NextImportSet )
    inc( RIS_SET )
    if ( RIS_SET LESS ${R}_IMPORT_CNT )
        set( RIS_ACTIVE 1 )
        JSON( GET RIS_TGT "tgt" "." "${${R}_IMPORT_${RIS_SET}}" )
        file_path( ABSOLUTE_PATH RIS_TGT BASE_DIRECTORY "${MTN_ROOTDIR}" NORMALIZE OUTPUT_VARIABLE RIS_ABSTGT )
        JSON( GET RIS_NAME "name" "" "${${R}_IMPORT_${RIS_SET}}" )
        JSON( GET RIS_STRIP "strip_folders" 0 "${${R}_IMPORT_${RIS_SET}}" )
        if ( NOT "${RIS_STRIP}" MATCHES "^[0-9]+$" ) # numerical values are what we want!
            string( TOLOWER "${RIS_STRIP}" RIS_STRIP )
            if ( RIS_STRIP AND NOT "${RIS_STRIP}" STREQUAL "none" )
                set( RIS_STRIP 4096 )
            else()
                set( RIS_STRIP 0 )
            endif()
        endif()
        set( RIS_ITEM -1 )
        JSON( LENGTH RIS_ITEMS "files" 0 "${${R}_IMPORT_${RIS_SET}}" )
        Maintenance_NextImportItem()
    else()
        set( RIS_ACTIVE 0 )
    endif()
    return( PROPAGATE ${RIS} )
 endfunction( Maintenance_NextImportSet )
 function( Maintenance_NextImportItem )
    inc( RIS_ITEM )
    if ( RIS_ITEM LESS RIS_ITEMS )
        set( RIS_SET_ACTIVE 1 )
        string( JSON _fj GET "${${R}_IMPORT_${RIS_SET}}" "files" )
        string( JSON _f GET "${_fj}" ${RIS_ITEM} )
        file_path( GET _f FILENAME _fn )
        if ( RIS_STRIP EQUAL 0 )
            file_path( ABSOLUTE_PATH _f BASE_DIRECTORY "${RIS_ABSTGT}" NORMALIZE OUTPUT_VARIABLE _ft )
        else()
            string( REPLACE "/" ";" _fp "${_f}" )   # Baue Pfad-Liste
            list( POP_BACK _fp )                    # Dateiname ist kein Teil davon!
            list( LENGTH _fp _sc )
            if ( _sc GREATER RIS_STRIP )
                set( _sc ${RIS_STRIP} )
            endif()
            foreach( _i RANGE 1 ${_sc} )
                list( POP_FRONT _fp )
            endforeach()
            list( PREPEND _fp "${RIS_ABSTGT}" )
            list( APPEND _fp "${_fn}" )
            list( JOIN _fp "/" _ft )
            file_path( ABSOLUTE_PATH _ft NORMALIZE )
        endif()
        file_path( GET _ft PARENT_PATH _fp )
        file_path( RELATIVE_PATH _ft BASE_DIRECTORY "${MTN_ROOTDIR}" OUTPUT_VARIABLE _fb )
    else()
        set( RIS_SET_ACTIVE 0 )
    endif()
    return( PROPAGATE ${RIS} )
 endfunction( Maintenance_NextImportItem )
 function( Maintenance_GetCurrentImportSetList outVar )
    string( JSON _fj GET "${${R}_IMPORT_${RIS_SET}}" "files" )
    dec( RIS_ITEMS TO n )
    foreach( i RANGE ${n} )
        string( JSON _f GET "${_fj}" ${i} )
        list( APPEND ${outVar} ${_f} )
    endforeach( i RANGE ${n} )
    return( PROPAGATE ${outVar} )
endfunction( Maintenance_GetCurrentImportSetList )

#[[ load the project's .gitignore file and determine possible repair / clean up actions ]]
 function( makeGIrepoBases )
    set( BS "#### REPOBLOCK - '${MTN_REPO}' - BEGIN ####" PARENT_SCOPE )
    set( BE "#### REPOBLOCK - '${MTN_REPO}' - END ####" PARENT_SCOPE )
 endfunction( makeGIrepoBases )
 function( Maintenance_LoadGI ) # read .gitignore, clean up current repo entry.
    Msg( START "reading git ignore file from '${MAINTENANCE_PROJECT_REPO_ROOT}'" )
    if ( EXISTS "${MAINTENANCE_PROJECT_REPO_ROOT}/.gitignore" )
        file( READ "${MAINTENANCE_PROJECT_REPO_ROOT}/.gitignore" MTN_GI )
     else() # empty .gitignore as a fallback
        unset( MTN_GI )
    endif()
    X_list( MTN_GI FROM_STRING "${MTN_GI}" )
    set( giList "MTN_GI;MTN_GI_CNT" )
    set( MTN_GI_CNT 0 )
    foreach( MTN_REPO IN LISTS MTN_REPOS )
        string( TOUPPER "${MTN_REPO}" R )
        list( APPEND giList "MTN_GI_${R}" )
        makeGIrepoBases()
        X_list( FIND MTN_GI "${BS}" _rs )
        X_list( FIND MTN_GI "${BE}" _re )
        if ( _rs GREATER -1 AND _re GREATER _rs )
            # splice away previous configs
            splice_list( MTN_GI EXCLUDE BEGIN ${_rs} END_INCL ${_re} SPLICE_VAR MTN_GI_${R} )
            inc( MTN_GI_CNT )
            Msg( "→ found repo block '${MTN_REPO}'" )
        elseif( _rs GREATER _re OR NOT ( _rs LESS 0 AND _re LESS 0 ) )
            Msg( "→ found problem: start_line=${_rs}, end_line=${_re} - something is wrong!" )
            list( APPEND MTN_ERRORS "Seems like the GIT-ignore file '${MTN_REPOROOT}/.gitignore' was corrupted.  Please have a closer look to repair!" )
        endif()
    endforeach()
    if ( NOT MTN_GI )
        Msg( DONE "project '.gitignore' EMPTY - very uncommon!" )
    else()
        list( LENGTH MTN_REPOS rc )
        if ( MTN_GI_CNT LESS rc )
            Msg( DONE "project '.gitignore' incomplete." )
        else()
            Msg( DONE "project '.gitignore' read, contains all repository-blocks" )
        endif()
    endif()
    # append gitignore-vars to global state:
    list( APPEND MTN_VARS ${giList} )
    return( PROPAGATE MTN_VARS ${MTN_VARS} )
endfunction( Maintenance_LoadGI )
#[[ rebuild and write project's .gitignore file from lists of strings ]]
 function( Maintenance_SaveGI ) # write Lists of Lines to .gitignore
    if ( MTN_GI OR EXISTS "${MAINTENANCE_PROJECT_REPO_ROOT}/.gitignore" )
        list( TRANSFORM MTN_REPOS TOUPPER OUTPUT_VARIABLE giList )
        list( TRANSFORM giList PREPEND "MTN_GI_" )
        foreach( _l IN LISTS giList )
            X_list( APPEND MTN_GI ${${_l}} )
        endforeach()
        X_list( LENGTH MTN_GI _l )
        X_list( JOIN MTN_GI "\n" MTN_GI )
        subReport( "produced ${_l} lines, writing '${MAINTENANCE_PROJECT_REPO_ROOT}/.gitignore'" )
        file( WRITE "${MAINTENANCE_PROJECT_REPO_ROOT}/.gitignore" "${MTN_GI}" )
     else()
        subReport( "writing .gitignore omittet - empty or non-existant." )
    endif()
endfunction( Maintenance_SaveGI )
#[[
    SUPPORT FUNCTIONALITY FOR ACTIONS
]]
function( Maintenance_setPI ) # set progress-indentation
    # calculate current value
    calc( MTN_PERCENT_DONE "( 100 * ${MTN_ACTIONS_POPPED} ) / ${MTN_ACTIONS_ADDED}" )
    num3( MTN_PERCENT_DONE DONE )
    num3( MTN_ACTIONS_POPPED POPPED )
    num3( MTN_ACTIONS_ADDED ADDED )
    string( CONFIGURE "${MTN_PI_FILL}" _t )
    list( POP_FRONT CMAKE_MESSAGE_INDENT )
    list( PREPEND CMAKE_MESSAGE_INDENT "${MTN_PI_PRE}${_t}${MTN_PI_POST}" )
    return( PROPAGATE CMAKE_MESSAGE_INDENT MTN_PERCENT_DONE )
 endfunction( Maintenance_setPI )
 function( num3 numVar tgtVar )
    string( LENGTH "${${numVar}}" _l )
    if ( _l LESS 3 )
        calc( _l "3 - ${_l}" )
        string( REPEAT " " ${_l} _l )
        set( ${tgtVar} "${_l}${${numVar}}" )
     else()
        set( ${tgtVar} "${${numVar}}" )
    endif()
    return( PROPAGATE ${tgtVar} )
 endfunction( num3 )

function( Maintenance_progress ) # simple output-wrapper (update Indentation, then fwd. `Msg()` )
    Maintenance_setPI()
    Msg( ${ARGV} )
    return( PROPAGATE ${MTN_VARS} )
 endfunction( Maintenance_progress )

function( Maintenance_subStep ) # make some progress (under the hood - output via Maintenance_setPI and Maintenance_progress)
    set( step 1 )
    if( ARGC EQUAL 1 )
        set( step "${ARGV0}" )
    endif()
    if ( MTN_SUBACTION_CNT GREATER_EQUAL step )
        dec( MTN_SUBACTION_CNT BY step )
        inc( MTN_ACTIONS_POPPED BY step )
     else()
        inc( MTN_ACTIONS_POPPED BY MTN_SUBACTION_CNT )
        set( MTN_SUBACTION_CNT 0 )
    endif()
    return( PROPAGATE MTN_SUBACTION_CNT MTN_ACTIONS_POPPED )
 endfunction( Maintenance_subStep )

#[[
    Better CMake target import support …
    ====================================
    Better CMake is a small extension package containing a few very helpful
    scripts enhancing your overall CMake build chain creation experience :)

    Maybe it would be more helpful to specify a list of targets we would like to link to?
    I mean, instead of long sections of testing to find complex requirements?

    - give a target list (required)
    - optionally give an optional target list
    - optionally add a `either`-list:
        - 1st item: how many items to choose from
        - nth item: items that satisfy the requirement
    → this function will return if all requirements could be met.
        - all targets in the primary list variable are found
        - from the optional list, all found targets are appended to the target list
        - from the either list, all found targets are appended to the target list
    → in any other case the behaviour is like with `find_package` - requirement not met: EXIT!

    Target naming: as target naming schemes vary, these assumptions are made:
    - <Package::Target> denotes a package to try via find_package
    - <Target> denotes a target name from a package config import

    Many times, linux implementations need core functionality available via package config, only,
    while e.g. corresponding windows implementations already have that functionality included thru
    the all-mighty Windows-SDK.

    So the idea actually is:
    - evaluate your project CACHE settings
    - build lists of required targets
        - if needed, build an "EITHER" list, if some packages are delivered in different ways on
        different platforms, this helps
    - call find_targets and be done.

    PRO?
    → no more multiple find_package / if()else()endif() sequences
    → instead 3 lists, which can be created and extended independently
    → more "semantic overview" over "what you actually need" - CMakeLists.txt better maintainable

 set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS 1 )
 set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS 1 ) ]]
 macro( ft_add_opts )
	foreach( _iit IN LISTS ${ARGV0} ) # optional targets list
		if ( "${_iit}" MATCHES "::" ) # contains find_package-signature
			string( REGEX MATCH "^[^:]+" _pkg "${_iit}" )
			if ( NOT ${_pkg} IN_LIST _pkgs )
				list( APPEND _pkgs ${_pkg} )
			endif()
			string( REGEX REPLACE "^[^:]+::" "" _iit "${_iit}" )
			if ( NOT "${_iit}" IN_LIST ${_pkg}_opts )
				list( APPEND ${_pkg}_opts ${_iit} )
			endif()
		else()
			list( APPEND _cfg_tgts ${_iit} )
			if ( NOT "${_iit}" IN_LIST _pkgs )
				list( APPEND _pkgs "${_iit}" )
			endif()
			if ( NOT "${_iit}" IN_LIST ${_iit}_opts )
				list( APPEND ${_iit}_opts "${_iit}" )
			endif()
		endif()
	endforeach()
 endmacro( ft_add_opts )
 function( ft_check_pkg_is_weird )
	# weird package handling
	if ( ${_pkg} MATCHES Vulkan AND WIN32 )
		if ( NOT TARGET Vulkan::glslang-default-resource-limits AND NOT TARGET glslang::glslang-default-resource-limits )
            cmake_path( GET Vulkan_INCLUDE_DIRS PARENT_PATH _IMPORT_PREFIX )
			list( APPEND CMAKE_PREFIX_PATH ${_IMPORT_PREFIX} )
			find_file( _d "glslang-default-resource-limitsd.lib" REQUIRED NO_CACHE PATH_SUFFIXES Lib )
			find_file( _r "glslang-default-resource-limits.lib" REQUIRED NO_CACHE PATH_SUFFIXES Lib )
			add_library( Vulkan::glslang-default-resource-limits STATIC IMPORTED )
			target_link_libraries( Vulkan::glslang-default-resource-limits PRIVATE ${_ff} )
			set_target_properties( Vulkan::glslang-default-resource-limits PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/Include"	)
			set_property( TARGET Vulkan::glslang-default-resource-limits APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG )
			set_target_properties( Vulkan::glslang-default-resource-limits PROPERTIES
			IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
			IMPORTED_LOCATION_DEBUG "${_d}" )
			set_property( TARGET Vulkan::glslang-default-resource-limits APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
			set_target_properties( Vulkan::glslang-default-resource-limits PROPERTIES
			IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
			IMPORTED_LOCATION_RELWITHDEBINFO "${_r}" )
			target_link_libraries( Vulkan::glslang INTERFACE [[$<TARGET_NAME_IF_EXISTS:Vulkan::glslang-default-resource-limits>]] )
			Msg( "→ added 'Vulkan::glslang-default-resource-limits', linked to 'Vulkan::glslang'" )
		endif()
	endif()
 endfunction( ft_check_pkg_is_weird )
 function( find_targets tgtListVar )
	cmake_parse_arguments( PARSE_ARGV 1 "ft" "" "" "OPTIONAL;EITHER" )
	# first go thru the targets themselves, separate package names, cumulate components, query.
	list( SORT ${tgtListVar} COMPARE NATURAL CASE INSENSITIVE ORDER ASCENDING )
	list( LENGTH ${tgtListVar} _l )
	list( LENGTH ft_EITHER _el )
	list( LENGTH ft_OPTIONAL _ol )
	set( _i 0 )
	unset( _pkg )
	unset( _pkgs )
	unset( _cfg_tgts )
	# gather a _pkgs list of packages, and ${_pkg}_tgts / ${_pkg}_opts
	while( _i LESS _l )
		list( GET ${tgtListVar} ${_i} _it )
		if ( NOT "${_it}" MATCHES "::" ) # pure target → PkgConfig + Target::Target
			list( APPEND _pkgs "${_it}" )
			list( APPEND ${_it}_tgts "${_it}" )
			list( APPEND _cfg_tgts "${_it}" )
		else()
			# next package?
			if ( NOT "${_it}" MATCHES "^${_pkg}::.*$" )
				string( REGEX MATCH "^[^:]+" _pkg "${_it}" )
				list( APPEND _pkgs ${_pkg} )
			endif()
			string( REGEX REPLACE "^${_pkg}::" "" _it "${_it}" )
			list( APPEND ${_pkg}_tgts ${_it} )
		endif()
		inc( _i )
	endwhile()
	# run thru EITHER-list
	set( _ii 0 )
	while( _ii LESS _el )
		list( GET ft_EITHER ${_ii} _iil )
		inc( _ii )
		list( SUBLIST ft_EITHER ${_ii} ${_iil} _eil )
		ft_add_opts( _eil )
		inc( _ii BY _iil )
	endwhile()
	# run thru optional-list
	ft_add_opts( ft_OPTIONAL )
	# now, we have packages and their requirements
	unset( _found )
	foreach( _pkg IN LISTS _pkgs )
		set( _tgt ${${_pkg}_tgts} )	# find_package normally defines Name::Name as default target,
		if ( "${_pkg}" IN_LIST _tgt ) # but won't find the component 'Name'
			list( REMOVE_ITEM _tgt "${_pkg}" )
		endif()
		find_package( ${_pkg} QUIET COMPONENTS ${_tgt} OPTIONAL_COMPONENTS ${${_pkg}_opts} )
		if ( ${_pkg}_DIR OR ${_pkg}_FOUND )
			ft_check_pkg_is_weird()
			foreach( _tgt IN LISTS ${_pkg}_tgts ${_pkg}_opts )
				if ( TARGET ${_pkg}::${_tgt} )
					list( APPEND _found ${_pkg}::${_tgt} )
					list( REMOVE_ITEM ${_pkg}_tgts ${_tgt} )
					list( REMOVE_ITEM ${_pkg}_opts ${_tgt} )
				endif()
			endforeach()
		endif()
	endforeach()
	list( REMOVE_DUPLICATES _found )
	Msg( PRE "find_package() found:" LIST ${_found} HIDE_EMPTY )
	# secondary search path: using package config → convert unfound targets to pkg-config queries
	foreach( _pkg IN LISTS _pkgs )
		foreach( _tgt IN LISTS ${_pkg}_tgts ${_pkg}_opts ) # not found by find_package
			string( REGEX REPLACE "^.*::" "" _tgt "${_tgt}" )
			list( APPEND _cfg_tgts ${_tgt} )
		endforeach()
	endforeach()
	list( REMOVE_DUPLICATES _cfg_tgts )
	foreach( _pkg IN LISTS _cfg_tgts )
		cmake_pkg_config( IMPORT ${_pkg} QUIET )
		if ( ${PKGCONFIG_${_pkg}_FOUND} ) # package found, check for targets
			# too bad, we'll have to check what was found
			foreach( _pkg ${_pkg} ${_pkgs} )
				foreach( _tgt IN LISTS ${_pkg}_tgts ${_pkg}_opts )
					if ( TARGET PkgConfig::${_tgt} )
						list( APPEND _pfound PkgConfig::${_tgt} )
						list( REMOVE_ITEM ${_pkg}_tgts ${_tgt} )
						list( REMOVE_ITEM ${_pkg}_opts ${_tgt} )
					endif()
				endforeach()
			endforeach()
		endif()
	endforeach()
	Msg( PRE "pakage_config() found:" LIST ${_pfound} HIDE_EMPTY )
	list( APPEND _found ${_pfound} )
	list( REMOVE_DUPLICATES _found )
	# check if we have found all required packages
	unset( _miss )
	foreach( _pkg IN LISTS _pkgs )
		if ( ${_pkg}_tgts ) # non-empty list? Requirements not met!
			list( APPEND _miss ${${_pkg}_tgts} )
			list( APPEND _miss ${${_pkg}_opts} )
		endif()
	endforeach()
	# check for missing EITHERS
	set( _ii 0 )
	while( _ii LESS _el )
		list( GET ft_EITHER ${_ii} _iil )
		inc( _ii )
		list( SUBLIST ft_EITHER ${_ii} ${_iil} _eil )
		set( _i false )
		foreach( _tgt IN LISTS _eil )
			if ( NOT "${_tgt}" MATCHES ".*::.*" )
				set( _tgt "PkgConfig::${_tgt}" )
			endif()
			if ( "${_tgt}" IN_LIST _found )
				if ( _i ) # remove same other targets from '_found', so preference
					list( REMOVE_ITEM _found "${_tgt}") # by order is honoured
				endif()
				set( _i true )
			endif()
		endforeach()
		if ( NOT _i )
			list( JOIN _eil "|" _eil )
			list( APPEND _miss "${_eil}" )
		endif()
		inc( _ii BY _iil )
	endwhile()
	# now, if anything is missing, we'll have to bail out.
	if ( _miss )
		Msg( FATAL PRE "find_targets was not sucessful - missing:" LIST ${_miss} )
	else()
		set( ${tgtListVar} "${_found}" PARENT_SCOPE )
	endif()
endfunction( find_targets )
