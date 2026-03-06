include(CMakeFindDependencyMacro)

find_dependency(Threads)

get_filename_component(NEXUSFORCE_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include("${NEXUSFORCE_CMAKE_DIR}/NexusForceCompilerOptions.cmake")

if(EXISTS "${NEXUSFORCE_CMAKE_DIR}/NexusForceTargets.cmake")
    include("${NEXUSFORCE_CMAKE_DIR}/NexusForceTargets.cmake")
else()
    set(NEXUSFORCE_FOUND TRUE)
    set(NEXUSFORCE_INCLUDE_DIRS "@PACKAGE_CMAKE_INSTALL_INCLUDEDIR@")

    add_library(NexusForce::NexusForce INTERFACE IMPORTED)
    target_include_directories(NexusForce::NexusForce INTERFACE
            ${NEXUSFORCE_INCLUDE_DIRS}
    )
endif()

function(nexusforce_apply_compiler_options target)
    nexusforce_compiler_options(${target})
endfunction()
