 #################################################################################
 #
 # Inviwo - Interactive Visualization Workshop
 #
 # Copyright (c) 2013-2017 Inviwo Foundation
 # All rights reserved.
 # 
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met: 
 # 
 # 1. Redistributions of source code must retain the above copyright notice, this
 # list of conditions and the following disclaimer. 
 # 2. Redistributions in binary form must reproduce the above copyright notice,
 # this list of conditions and the following disclaimer in the documentation
 # and/or other materials provided with the distribution. 
 # 
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 # ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 # WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 # DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 # ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 # (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 # LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 # ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 # SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 # 
 #################################################################################

#--------------------------------------------------------------------
# Creates project with initial variables
# Creates a CMake projects
# Defines:  (example project_name = OpenGL)
# _projectName -> OpenGL
# _allLibsDir -> ""
# _allDefinitions -> ""
# _allLinkFlags -> ""
# _allPchDirs -> ""
# _cpackName -> modules or qt_modules if QT
# _pchDisabledForThisModule -> FALSE 
macro(ivw_project project_name)
    project(${project_name} ${ARGN})
    set(_projectName ${project_name})
    set(_allLibsDir "")
    set(_allDefinitions "")
    set(_allLinkFlags "")
    set(_allPchDirs "")
    set(_cpackName modules)
    set(_pchDisabledForThisModule FALSE)
    string(TOUPPER ${project_name} u_project_name)
    if(u_project_name MATCHES "QT+")
        set(_cpackName qt_modules)
    endif()
endmacro()

#--------------------------------------------------------------------
# Creates a inviwo module
# Defines:  (example project_name = OpenGL)
#    _packageName           -> InviwoOpenGLModule
#    _preModuleDependencies -> ""
#   PARENT_SCOPE:
#   IVW_MODULE_CLASS        -> OpenGL
#   IVW_MODULE_PACKAGE_NAME -> InviwoOpenGLModule
macro(ivw_module project_name)
    string(TOLOWER ${project_name} l_project_name)
    ivw_project(${l_project_name})
    set(_packageName Inviwo${project_name}Module)
    set(_preModuleDependencies "")
    set(IVW_MODULE_CLASS ${project_name} PARENT_SCOPE)
    set(IVW_MODULE_PACKAGE_NAME ${_packageName} PARENT_SCOPE)
endmacro()

#--------------------------------------------------------------------
# Creates project with initial variables
macro(ivw_set_cpack_name cpack_name)
    set(_cpackName ${cpack_name})
endmacro()

#--------------------------------------------------------------------
# Add cmake module path
macro(ivw_add_cmake_find_package_path)
    foreach(item ${ARGN})
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${item})
    endforeach()
endmacro()

#--------------------------------------------------------------------
# Register the use of modules
macro(ivw_register_use_of_modules)
    foreach(module ${ARGN})
        string(TOUPPER ${module} u_module)
        ivw_add_definition(REG_${u_module})
    endforeach()
endmacro()

#--------------------------------------------------------------------
# Retrieve all modules as a list
function(ivw_retrieve_all_modules module_list)
    set(${module_list} ${ivw_all_registered_modules} PARENT_SCOPE)
endfunction()

#--------------------------------------------------------------------
# Generate header for external modules
function(ivw_generate_module_paths_header)
    set(dirs "")
    foreach(dir ${IVW_ROOT_DIR}/modules ${IVW_EXTERNAL_MODULES})
        if(IS_DIRECTORY ${dir})
            list(APPEND dirs ${dir})
        else()
            message("Path to external module is not a directory (${dir})")
        endif()
    endforeach()

    list_to_longstringvector(vec ${dirs})
    list(LENGTH IVW_EXTERNAL_MODULES count)
    math(EXPR count "${count}+1")
    set(paths "const std::array<const std::string, ${count}> inviwoModulePaths_ = {${vec}}")

    set(IVW_MODULES_PATHS_ARRAY ${paths})

    configure_file(${IVW_CMAKE_TEMPLATES}/inviwomodulespaths_template.h 
                   ${CMAKE_BINARY_DIR}/modules/_generated/inviwomodulespaths.h @ONLY)
endfunction()

#--------------------------------------------------------------------
# Generate a module registration header file (with configure file etc)
function(ivw_private_generate_module_registration_file modules_var)
    set(headers "")
    set(functions "")
    foreach(mod ${${modules_var}})      
        set(header
            "#ifdef REG_${mod}\n"
            "#include <${${mod}_dir}/${${mod}_dir}module.h>\n"
            "#endif\n"
        )
        join(";" "" header ${header})

        ivw_mod_name_to_dir(module_dependencies ${${mod}_dependencies})
        list_to_stringvector(module_depends_vector ${module_dependencies})
        set(factory_object
            "    #ifdef REG_${mod}\n" 
            "    modules.emplace_back(new InviwoModuleFactoryObjectTemplate<${${mod}_class}Module>(\n"
            "        \"${${mod}_class}\",\n"
            "        \"${${mod}_description}\",\n" 
            "        ${module_depends_vector}\n" 
            "        )\n"
            "    )__SEMICOLON__\n"
            "    #endif\n"
            "\n"
        )
        join(";" "" factory_object ${factory_object})

        list(APPEND headers ${header})
        list(APPEND functions ${factory_object})
    endforeach()

    join(";" "" headers ${headers})
    join(";" "" functions ${functions})

    # undo encoding of linebreaks and semicolon in the module description read from file
    # linebreaks are replaced with '\n"'
    string(REPLACE "__LINEBREAK__" "\\n\"\n        \"" functions "${functions}")
    string(REPLACE "__SEMICOLON__" ";" functions "${functions}")

    string(REPLACE ":" ";" MODULE_HEADERS "${headers}")   
    string(REPLACE ":" ";" MODULE_CLASS_FUNCTIONS "${functions}")

    configure_file(${IVW_CMAKE_TEMPLATES}/mod_registration_template.h 
                   ${CMAKE_BINARY_DIR}/modules/_generated/moduleregistration.h @ONLY)
endfunction()

function(ivw_private_create_pyconfig modulepaths activemodules)
    # template vars:
    set(MODULEPATHS ${modulepaths})
    set(ACTIVEMODULES ${activemodules})

    find_package(Git QUIET)
    if(GIT_FOUND)
        ivw_debug_message(STATUS "git found: ${GIT_EXECUTABLE}")
    else()
        set(GIT_EXECUTABLE "")
    endif()

    configure_file(${IVW_CMAKE_TEMPLATES}/pyconfig_template.ini 
                   ${CMAKE_BINARY_DIR}/pyconfig.ini @ONLY)
endfunction()

function(ivw_private_is_valid_module_dir path dir retval)
    if(IS_DIRECTORY ${module_path}/${dir})
        string(TOLOWER ${dir} test)
        string(REPLACE " " "" ${test} test)
        if(${dir} STREQUAL ${test})
            if(EXISTS ${module_path}/${dir}/CMakeLists.txt)
                ivw_private_get_ivw_module_name(${module_path}/${dir}/CMakeLists.txt name)
                string(TOLOWER ${name} l_name)
                if(${dir} STREQUAL ${l_name})
                    set(${retval} TRUE PARENT_SCOPE)
                    return()
                else()
                    message("Found invalid module \"${dir}\" at \"${module_path}\". "
                        "ivw_module called with \"${name}\" which is different from the directory \"${dir}\""
                        "They should be the same except for casing.")
                endif()
            else()
                message("Found invalid module \"${dir}\" at \"${module_path}\". "
                    "CMakeLists.txt is missing")
            endif()
        else()
            message("Found invalid module dir \"${dir}\" at \"${module_path}\". "
                "Dir names should be all lowercase and without spaces")
        endif()
    endif()
    set(${retval} FALSE PARENT_SCOPE)
endfunction()

#--------------------------------------------------------------------
# Register modules
# Generate module options (which was not specified before) and,
# Sort directories based on dependencies inside directories
# defines:  (example project_name = OpenGL)
# INVIWOOPENGLMODULE_description  -> </readme.md>
# INVIWOOPENGLMODULE_dependencies -> </depends.cmake::dependencies>
# 
function(ivw_register_modules retval)
    # Collect all modules and information
    set(modules "")
    foreach(module_path ${IVW_MODULE_DIR} ${IVW_EXTERNAL_MODULES})
        string(STRIP ${module_path} module_path)
        file(GLOB dirs RELATIVE ${module_path} ${module_path}/[^.]*)
        foreach(dir ${dirs})
            ivw_dir_to_mod_dep(mod ${dir})
            list(FIND modules ${mod} found)
            if(NOT ${found} EQUAL -1)
                message("Module with name ${dir} already added at ${${mod}_path}")
                continue()
            endif()
            ivw_private_is_valid_module_dir(${module_path} ${dir} valid)
            if(${valid})
                ivw_debug_message(STATUS "register module: ${dir}")
                ivw_dir_to_mod_prefix(opt ${dir})           # OpenGL -> IVW_MODULE_OPENGL
                ivw_dir_to_module_taget_name(target ${dir}) # OpenGL -> inviwo-module-opengl
                # Get the classname with the right casing
                ivw_private_get_ivw_module_name(${module_path}/${dir}/CMakeLists.txt name)
                list(APPEND modules ${mod})
                set("${mod}_dir"    "${dir}"                CACHE INTERNAL "Module dir")
                set("${mod}_base"   "${module_path}"        CACHE INTERNAL "Module base")
                set("${mod}_path"   "${module_path}/${dir}" CACHE INTERNAL "Module path")
                set("${mod}_opt"    "${opt}"                CACHE INTERNAL "Module cmake option")
                set("${mod}_target" "${target}"             CACHE INTERNAL "Module target")
                set("${mod}_class"  "${name}"               CACHE INTERNAL "Module class")
                set("${mod}_name"   "Inviwo${name}Module"   CACHE INTERNAL "Module name")

                # Check of there is a depends.cmake
                # Defines dependencies, aliases
                # Save dependencies to INVIWO<NAME>MODULE_dependencies
                # Save aliases to INVIWO<NAME>MODULE_aliases
                if(EXISTS "${${mod}_path}/depends.cmake")
                    set(dependencies "")
                    set(aliases "")
                    include(${${mod}_path}/depends.cmake) 
                    set("${mod}_dependencies" ${dependencies} CACHE INTERNAL "Module dependencies")
                    set("${mod}_aliases" ${aliases} CACHE INTERNAL "Module aliases")
                    unset(dependencies)
                    unset(aliases)
                endif()

                # Check if there is a readme.md of the module. 
                # In that case set to INVIWO<NAME>MODULE_description
                if(EXISTS "${${mod}_path}/readme.md")
                    file(READ "${${mod}_path}/readme.md" description)
                    # encode linebreaks, i.e. '\n', and semicolon in description for
                    # proper handling in CMAKE
                    encodeLineBreaks(cdescription ${description})
                    set("${mod}_description" ${cdescription} CACHE INTERNAL "Module description")
                endif()
            endif()
        endforeach()
    endforeach()

    # Add modules to cmake cache
    foreach(mod ${modules})
        lowercase(default_dirs ${ivw_default_modules})          
        list(FIND default_dirs ${${mod}_dir} index)
        if(NOT index EQUAL -1)
            ivw_add_module_option_to_cache(${${mod}_dir} ON FALSE)
        else()
            ivw_add_module_option_to_cache(${${mod}_dir} OFF FALSE)
        endif()
    endforeach()

    # Find aliases
    set(aliases "")
    foreach(mod ${modules})
        foreach(alias ${${mod}_aliases})
            list(APPEND aliases ${alias})
            if(DEFINED alias_${alias}_mods)
                list(APPEND alias_${alias}_mods ${mod})
            else()
                set(alias_${alias}_mods ${mod})
            endif()
        endforeach()
    endforeach()

    # Substitute aliases
    foreach(mod ${modules})
        set(new_dependencies "")
        foreach(dependency ${${mod}_dependencies})
            list(FIND aliases ${dependency} found)
            if(NOT ${found} EQUAL -1)
                if(DEFINED ${${mod}_opt}_${dependency})
                    list(APPEND new_dependencies ${${${mod}_opt}_${dependency}})
                else()
                    # Find the best substitute
                    list(GET ${alias_${dependency}_mods} 0 new_mod)
                    set(new_dep ${${new_mod}_name})
                    foreach(alias_mod ${alias_${dependency}_mods})
                        set(new_dep ${${alias_mod}_name})
                        if(${${alias_mod}_opt}}) # if substitution is enabled we stick with that one.
                            break()
                        endif()
                    endforeach()
                    list(APPEND new_dependencies ${new_dep})
                    set(${${mod}_opt}_${dependency} "${new_dep}" CACHE STRING "Dependency")
                endif()
                set(alias_names "")
                foreach(alias_mod ${alias_${dependency}_mods})
                    list(APPEND alias_names ${${alias_mod}_name})
                endforeach()
                set_property(CACHE ${${mod}_opt}_${dependency} PROPERTY STRINGS ${alias_names})
            else()
                list(APPEND new_dependencies ${dependency})
            endif()
        endforeach()
        set("${mod}_dependencies" ${new_dependencies} CACHE INTERNAL "Module dependencies")
    endforeach()

    # Filter out inviwo dependencies
    foreach(mod ${modules})
        set(ivw_dependencies "")
        foreach(dependency ${${mod}_dependencies})
            ivw_mod_name_to_mod_dep(dep ${dependency})
            list(FIND modules ${dep} found)
            if(NOT ${found} EQUAL -1) # This is a dependency to a inviwo module
                list(APPEND ivw_dependencies ${dep})
            endif()
        endforeach()
        set("${mod}_ivw_dependencies" ${ivw_dependencies} CACHE INTERNAL "Module inviwo module dependencies")
    endforeach()

    # Sort modules by dependencies
    ivw_topological_sort(modules _ivw_dependencies sorted_modules)
    #ivw_print_list(sorted_modules)

    # enable depencenies
    ivw_reverse_list_copy(sorted_modules rev_sorted_modules)
    foreach(mod ${rev_sorted_modules})
        if(${${mod}_opt})
            foreach(dep ${${mod}_ivw_dependencies})
                if(NOT ${${dep}_opt})
                    ivw_add_module_option_to_cache(${${dep}_dir} ON TRUE)
                    message(STATUS "${${dep}_opt} was set to build, "
                        "due to dependency towards ${${mod}_opt}")
                endif()
            endforeach()
        endif()
    endforeach()

    # Add enabled modules in sorted order
    set(ivw_module_classes "")
    set(ivw_module_names "")

    set(IVW_MODULE_PACKAGE_NAMES "")
    set(IVW_MODULE_CLASS "")
    set(IVW_MODULE_PACKAGE_NAME "")

    foreach(mod ${sorted_modules})
        if(${${mod}_opt})
            add_subdirectory(${${mod}_path} ${IVW_BINARY_DIR}/modules/${${mod}_dir})
            if(NOT "${${mod}_class}" STREQUAL "${IVW_MODULE_CLASS}")
                message("Missmatched module class names \"${${mod}_class}\" vs \"${IVW_MODULE_CLASS}\"")
            endif()
            list(APPEND ivw_module_names ${${mod}_name})
            list(APPEND ivw_module_classes ${${mod}_class})
            list(APPEND IVW_MODULE_PACKAGE_NAMES ${IVW_MODULE_PACKAGE_NAME})
        endif()
    endforeach()

    list(REMOVE_DUPLICATES ivw_module_classes)

    # Save list of modules
    set(ivw_all_registered_modules ${ivw_module_names} CACHE INTERNAL "All registered inviwo modules")

    # Generate module registration file
    ivw_private_generate_module_registration_file(sorted_modules)

    # Save information for python tools.
    ivw_private_create_pyconfig("${IVW_MODULE_DIR};${IVW_EXTERNAL_MODULES}" "${ivw_module_classes}")

    set(${retval} ${sorted_modules} PARENT_SCOPE)
endfunction()

#--------------------------------------------------------------------
# Set module build option to true if the owner is built
function(ivw_add_build_module_dependency the_module the_owner)
    ivw_dir_to_mod_prefix(mod_name ${the_module})
    first_case_upper(dir_name_cap ${the_module})
    if(${the_owner} AND NOT ${mod_name})
        ivw_add_module_option_to_cache(${the_module} ON TRUE)
        message(STATUS "${mod_name} was set to build, due to dependency towards ${the_owner}")
    endif()
endfunction()

#--------------------------------------------------------------------
# Add all external modules specified in cmake string IVW_EXTERNAL_MODULES
macro(ivw_add_external_projects)
    foreach(project_root_path ${IVW_EXTERNAL_PROJECTS})
        string(STRIP ${project_root_path} project_root_path)
        get_filename_component(FOLDER_NAME ${project_root_path} NAME)
        add_subdirectory(${project_root_path} ${CMAKE_CURRENT_BINARY_DIR}/ext_${FOLDER_NAME})
    endforeach()
endmacro()

#--------------------------------------------------------------------
# Set module build option to true
function(ivw_enable_module the_module)
    ivw_add_module_option_to_cache(${the_module} ON FALSE)
endfunction()

#--------------------------------------------------------------------
# Add a library dependency to module. call before ivw_create_module
macro(add_dependency_libs_to_module)
    list(APPEND _preModuleDependencies "${ARGN}")
endmacro()

#--------------------------------------------------------------------
# Creates source group structure recursively
function(ivw_group group_name)
    foreach(currentSourceFile ${ARGN})
        if(NOT IS_ABSOLUTE ${currentSourceFile})
            set(currentSourceFile ${CMAKE_CURRENT_SOURCE_DIR}/${currentSourceFile})
        endif()
        string(REPLACE "include/inviwo/" "src/" currentSourceFileModified ${currentSourceFile})
        file(RELATIVE_PATH folder ${CMAKE_CURRENT_SOURCE_DIR} ${currentSourceFileModified})
        get_filename_component(folder ${folder} PATH)

        if(group_name STREQUAL "Test Files")
            string(REPLACE "tests/unittests" "" folder ${folder})
        endif()

        if(NOT folder STREQUAL "")
            string(REGEX REPLACE "/+$" "" folderlast ${folder})
            string(REPLACE "/" "\\" folderlast ${folderlast})
            source_group("${group_name}\\${folderlast}" FILES ${currentSourceFile})
        else()
            source_group("${group_name}" FILES ${currentSourceFile})
        endif(NOT folder STREQUAL "")
    endforeach(currentSourceFile ${ARGN})
endfunction()

#--------------------------------------------------------------------
# Creates VS folder structure
function(ivw_folder target folder_name)
    set_target_properties(${target} PROPERTIES FOLDER ${folder_name})
endfunction()

#--------------------------------------------------------------------
# Specify standard compile options
# ivw_define_standard_properties(target1 [target2 ...])
function(ivw_define_standard_properties)
    foreach(target ${ARGN})
        # Specify warnings
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR 
            "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
            "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
            get_property(comp_opts TARGET ${target} PROPERTY COMPILE_OPTIONS)
            list(APPEND comp_opts "-Wall")
            list(APPEND comp_opts "-Wextra")
            list(APPEND comp_opts "-pedantic")
            list(APPEND comp_opts "-Wno-unused-parameter") # not sure we want to remove them.
            list(APPEND comp_opts "-Wno-missing-braces")   # http://stackoverflow.com/questions/13905200/is-it-wise-to-ignore-gcc-clangs-wmissing-braces-warning
            set_property(TARGET ${target} PROPERTY COMPILE_OPTIONS ${comp_opts})
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            get_property(comp_opts TARGET ${target} PROPERTY COMPILE_OPTIONS)
            string(REGEX REPLACE "(^|;)([/-])W[0-9](;|$)" ";" comp_opts "${comp_opts}") # remove any other waning level
            #list(APPEND comp_opts "/nologo") # Suppress Startup Banner
            list(APPEND comp_opts "/W4")     # Set default warning level to 4
            list(APPEND comp_opts "/wd4005") # macro redefinition    https://msdn.microsoft.com/en-us/library/8d10sc3w.aspx
            list(APPEND comp_opts "/wd4201") # nameless struct/union https://msdn.microsoft.com/en-us/library/c89bw853.aspx
            list(APPEND comp_opts "/wd4251") # needs dll-interface   https://msdn.microsoft.com/en-us/library/esew7y1w.aspx
            list(APPEND comp_opts "/wd4505") # unreferenced funtion  https://msdn.microsoft.com/en-us/library/mt694070.aspx
            list(APPEND comp_opts "/wd4996") # ignore deprication    https://msdn.microsoft.com/en-us/library/ttcz0bys.aspx
            list(REMOVE_DUPLICATES comp_opts)
            set_property(TARGET ${target} PROPERTY COMPILE_OPTIONS ${comp_opts})
    
            get_property(comp_defs TARGET ${target} PROPERTY COMPILE_DEFINITIONS)
            list(APPEND comp_defs "_CRT_SECURE_NO_WARNINGS") # https://msdn.microsoft.com/en-us/library/ms175759.aspx
            list(REMOVE_DUPLICATES comp_defs)
            set_property(TARGET ${target} PROPERTY COMPILE_DEFINITIONS ${comp_defs})
        endif()

        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
            #https://developer.apple.com/library/mac/documentation/DeveloperTools/Reference/XcodeBuildSettingRef/1-Build_Setting_Reference/build_setting_ref.html
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_NON_VIRTUAL_DESTRUCTOR YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_UNUSED_FUNCTION YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_UNUSED_VARIABLE YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_HIDDEN_VIRTUAL_FUNCTIONS YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_ABOUT_RETURN_TYPE YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_EFFECTIVE_CPLUSPLUS_VIOLATIONS YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_PEDANTIC YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_GCC_WARN_SIGN_COMPARE YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_CLANG_WARN_ENUM_CONVERSION YES)
            set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_WARNING_CFLAGS "-Wunreachable-code")
         endif()
    endforeach()
endfunction()

#--------------------------------------------------------------------
# Define standard defintions
macro(ivw_define_standard_definitions project_name target_name)
    # Set the compiler flags
    ivw_to_macro_name(u_project_name ${project_name})
    target_compile_definitions(${target_name} PRIVATE -D${u_project_name}_EXPORTS)
    target_compile_definitions(${target_name} PRIVATE -DGLM_EXPORTS)

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # Large memory support
        if(CMAKE_SIZEOF_VOID_P MATCHES 4)
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " /LARGEADDRESSAWARE") 
        endif()
        target_compile_definitions(${target_name} PRIVATE -DUNICODE)
        target_compile_definitions(${target_name} PRIVATE -D_CRT_SECURE_NO_WARNINGS 
                                                          -D_CRT_SECURE_NO_DEPRECATE)
    else()
        target_compile_definitions(${target_name} PRIVATE -DHAVE_CONFIG_H)
    endif()

    source_group("CMake Files" FILES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
endmacro()

#--------------------------------------------------------------------
# Add folder to module pack
macro(ivw_add_to_module_pack folder)
    if(IVW_PACKAGE_PROJECT)
        get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        if(APPLE)
            install(DIRECTORY ${folder}
                     DESTINATION Inviwo.app/Contents/Resources/modules/${FOLDER_NAME}
                     COMPONENT ${_cpackName})
        else()
            install(DIRECTORY ${folder}
                     DESTINATION modules/${FOLDER_NAME}
                     COMPONENT ${_cpackName})
        endif()
    endif()
endmacro()

#--------------------------------------------------------------------
# Creates project module from name
# This it called from the inviwo module CMakeLists.txt 
# that is included from ivw_register_modules. 
macro(ivw_create_module)
    ivw_debug_message(STATUS "create module: ${_projectName}")
    ivw_dir_to_mod_dep(mod ${_projectName})  # opengl -> INVIWOOPENGLMODULE

    set(CMAKE_FILES "")
    if(EXISTS "${${mod}_path}/depends.cmake")
        list(APPEND CMAKE_FILES "${${mod}_path}/depends.cmake")
    endif()
    if(EXISTS "${${mod}_path}/readme.md")
        list(APPEND CMAKE_FILES "${${mod}_path}/readme.md")
    endif()
    source_group("CMake Files" FILES ${CMAKE_FILES})

    # Add module class files
    set(MOD_CLASS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_projectName}module.h)
    list(APPEND MOD_CLASS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_projectName}module.cpp)
    list(APPEND MOD_CLASS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_projectName}moduledefine.h)
    remove_duplicates(IVW_UNIQUE_MOD_FILES ${ARGN} ${MOD_CLASS_FILES} ${CMAKE_FILES})
    # Create library
    add_library(${${mod}_target} ${IVW_UNIQUE_MOD_FILES})
    
    # Define standard properties
    ivw_define_standard_definitions(${${mod}_opt} ${${mod}_target})
    ivw_define_standard_properties(${${mod}_target})
    
    # Add dependencies
    target_link_libraries(${${mod}_target} PUBLIC ${_preModuleDependencies})

    # Add dependencies from depends.cmake and InviwoCore
    ivw_add_dependencies_on_target(${${mod}_target} InviwoCore ${${mod}_dependencies})

    # Optimize compilation with pre-compilied headers based on inviwo-core
    ivw_compile_optimize_inviwo_core_on_target(${${mod}_target})

    # Make package (for other modules to find)
    ivw_make_package(${_packageName} ${${mod}_target})

    # Add stuff to the installer
    ivw_private_install_package(${${mod}_target})
    ivw_private_install_module_dirs()

    ivw_make_unittest_target("${${mod}_dir}" "${${mod}_target}")
endmacro()

#--------------------------------------------------------------------
# Make package (with configure file etc)
macro(ivw_make_package package_name target)
        # retrieve output name of target, use project name if not set
    get_target_property(ivw_output_name ${target} OUTPUT_NAME)
    if(NOT ivw_output_name)
        set(ivw_output_name ${target})
    endif()

    # retrieve target definitions
    get_target_property(ivw_target_defs ${target} INTERFACE_COMPILE_DEFINITIONS)
    if(ivw_target_defs)
        ivw_prepend(ivw_target_defs "-D" ${ivw_target_defs})
        list(APPEND _allDefinitions ${ivw_target_defs})
    endif()

    # Set up libraries
    if(WIN32 AND BUILD_SHARED_LIBS)
        set(PROJECT_LIBRARIES ${IVW_LIBRARY_DIR}/$<CONFIG>/${ivw_output_name}$<$<CONFIG:DEBUG>:${CMAKE_DEBUG_POSTFIX}>.lib)
    else()
       set(PROJECT_LIBRARIES ${ivw_output_name}$<$<CONFIG:DEBUG>:${CMAKE_DEBUG_POSTFIX}>)
    endif()
    
    get_target_property(ivw_allLibs ${target} INTERFACE_LINK_LIBRARIES)
    if(NOT ivw_allLibs)
        set(ivw_allLibs "")
    endif()
    list(APPEND ivw_allLibs ${PROJECT_LIBRARIES})
    remove_duplicates(ivw_unique_allLibs ${ivw_allLibs})
    set(ivw_allLibs ${ivw_unique_allLibs})

    # Set up inlude directories 
    # Should only INTERFACE_INCLUDE_DIRECTORIES but we can't since we use include_directories...
    get_target_property(ivw_allIncDirs ${target} INCLUDE_DIRECTORIES)
    if(NOT ivw_allIncDirs)
        set(ivw_allIncDirs "")
    endif()
    get_target_property(ivw_allInterfaceIncDirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(ivw_allInterfaceIncDirs)
        list(APPEND ivw_allIncDirs ${ivw_allInterfaceIncDirs})
    endif()
    remove_duplicates(ivw_unique_allIncDirs ${ivw_allIncDirs})
    set(ivw_allIncDirs ${ivw_unique_allIncDirs})

    remove_duplicates(uniqueLibsDir ${_allLibsDir})
    remove_duplicates(uniqueDefs ${_allDefinitions})
    remove_duplicates(uniqueLinkFlags ${_allLinkFlags})
    
    string(TOUPPER ${package_name} u_package_name)
    set(package_name ${u_package_name})
    set(_allLibsDir ${uniqueLibsDir})
    set(_allDefinitions ${uniqueDefs})
    set(_allLinkFlags ${uniqueLinkFlags})
    set(_project_name ${target})
  
    configure_file(${IVW_CMAKE_TEMPLATES}/mod_package_template.cmake 
                   ${IVW_CMAKE_BINARY_MODULE_DIR}/Find${package_name}.cmake @ONLY)
endmacro()


#--------------------------------------------------------------------
# Install files
function(ivw_private_install_package project_name)
    ivw_default_install_comp_targets(${_cpackName} ${project_name})
endfunction()

function(ivw_private_install_module_dirs)
    if(IVW_PACKAGE_PROJECT) 
        get_filename_component(module_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
        foreach(folder data docs)
            set(dir ${CMAKE_CURRENT_SOURCE_DIR}/${folder})
            if(EXISTS ${dir})
                if(APPLE)
                    install(DIRECTORY ${dir}
                            DESTINATION Inviwo.app/Contents/Resources/modules/${module_name}
                            COMPONENT ${_cpackName})
                else()
                    install(DIRECTORY ${dir}
                            DESTINATION modules/${module_name}
                            COMPONENT ${_cpackName})
                endif()
            endif()
        endforeach()
    endif()
endfunction()

#--------------------------------------------------------------------
# Add includes
macro(ivw_include_directories)
    include_directories(${ARGN})
endmacro()

#--------------------------------------------------------------------
# Add includes, should be called inside a module 
# after ivw_module has been called
# and befor ive_create_module
macro(ivw_link_directories)
    # Set includes
    link_directories("${ARGN}")
    # Append includes to project list
    list(APPEND _allLibsDir ${ARGN})
endmacro()

#--------------------------------------------------------------------
# Add includes, should be called inside a module 
# after ivw_module has been called
# and befor ive_create_module
macro(ivw_add_link_flags)
    # Set link flags
    get_property(flags TARGET ${_projectName} PROPERTY LINK_FLAGS)
    list(APPEND flags "${ARGN}")
    set_property(TARGET ${_projectName} PROPERTY LINK_FLAGS ${flags})

    # Append includes to project list
    list(APPEND _allLinkFlags "\"${ARGN}\"")
endmacro()

#--------------------------------------------------------------------
# Add definition, should be called inside a module 
# after ivw_module has been called
# and befor ive_create_module
macro(ivw_add_definition def)
    add_definitions(-D${def})
    list(APPEND _allDefinitions -D${def})
endmacro()

#--------------------------------------------------------------------
# Add definition to list only , should be called inside a module 
# after ivw_module has been called
# and befor ive_create_module
macro(ivw_add_definition_to_list def)
    list(APPEND _allDefinitions -D${def})
endmacro()

#--------------------------------------------------------------------
# adds link_directories for the supplies dependencies
# should be called inside a module 
# after ivw_module has been called
# and befor ive_create_module
macro(ivw_add_dependency_directories)
    foreach (package ${ARGN})
        # Locate libraries
        find_package(${package} QUIET REQUIRED)
        ivw_find_package_name(${package} u_package)
      
        # Append library directories to project list
        set(uniqueNewLibDirs ${${u_package}_LIBRARY_DIR})
        remove_from_list(uniqueNewLibDirs "${${u_package}_LIBRARY_DIR}" ${_allLibsDir})
        set(${u_package}_LIBRARY_DIR ${uniqueNewLibDirs})
        list(APPEND _allLibsDir ${${u_package}_LIBRARY_DIR})

        # Set directory links
        link_directories(${${u_package}_LIBRARY_DIR})
    endforeach()
endmacro()

#--------------------------------------------------------------------
# Adds dependency and includes package variables to the project
# 
# Defines: for example the OpenGL module
# INVIWOOPENGLMODULE_DEFINITIONS=
# INVIWOOPENGLMODULE_FOUND=
# INVIWOOPENGLMODULE_INCLUDE_DIR=
# INVIWOOPENGLMODULE_LIBRARIES=
# INVIWOOPENGLMODULE_LIBRARY_DIR=
# INVIWOOPENGLMODULE_LINK_FLAGS=
# INVIWOOPENGLMODULE_USE_FILE=
# 
# Appends to globals:
# _allLibsDir
# _allDefinitions
# _allLinkFlags
# 
macro(ivw_add_dependencies)
    ivw_add_dependencies_on_target(${_projectName} ${ARGN})
endmacro()

macro(ivw_add_dependencies_on_target target)
    foreach (package ${ARGN})
        # Locate libraries
        find_package(${package} QUIET REQUIRED)

        ivw_find_package_name(${package} u_package)
        
        # Append library directories to project list
        set(uniqueNewLibDirs ${${u_package}_LIBRARY_DIR})
        remove_from_list(uniqueNewLibDirs "${${u_package}_LIBRARY_DIR}" ${_allLibsDir})
        list(APPEND _allLibsDir ${${u_package}_LIBRARY_DIR})
        
        foreach (libDir ${${u_package}_LIBRARY_DIRS})
            remove_from_list(uniqueNewLibDirs ${libDir} ${_allLibsDir})
            list(APPEND _allLibsDir ${libDir})
        endforeach()
        
        set(${u_package}_LIBRARY_DIRS ${uniqueNewLibDirs})
        
        # Append includes to project list
        if(NOT DEFINED ${u_package}_LIBRARIES  AND DEFINED ${u_package}_LIBRARY)
            if(DEFINED ${u_package}_LIBRARY_DEBUG)
                set(${u_package}_LIBRARIES optimized "${${u_package}_LIBRARY}" 
                                           debug "${${u_package}_LIBRARY_DEBUG}")
            else()
                set(${u_package}_LIBRARIES "${${u_package}_LIBRARY}")
            endif()
        endif()
      
        # Append definitions to project list
        set(uniqueNewDefs ${${u_package}_DEFINITIONS})
        remove_from_list(uniqueNewDefs "${${u_package}_DEFINITIONS}" ${_allDefinitions})
        set(${u_package}_DEFINITIONS ${uniqueNewDefs})
        list (APPEND _allDefinitions ${${u_package}_DEFINITIONS})

        # Append link flags to project list
        set(uniqueNewLinkFlags ${${u_package}_LINK_FLAGS})
        remove_from_list(uniqueNewLinkFlags "${${u_package}_LINK_FLAGS}" ${_allLinkFlags})
        set(${u_package}_LINK_FLAGS ${uniqueNewLinkFlags})
        if(NOT "${${u_package}_LINK_FLAGS}" STREQUAL "")
            list (APPEND _allLinkFlags "\"${${u_package}_LINK_FLAGS}\"")
        endif()
    
        # Set includes and append to list (Only add new include dirs)
        get_target_property(ivw_already_added_incdirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
        if(NOT ivw_already_added_incdirs)
            set(ivw_already_added_incdirs "")
        endif()
        remove_from_list(ivw_new_incdirs "${${u_package}_INCLUDE_DIR};${${u_package}_INCLUDE_DIRS}" ${ivw_already_added_incdirs})
        target_include_directories(${target} PUBLIC ${ivw_new_incdirs})

        # Set directory links
        link_directories(${${u_package}_LIBRARY_DIRS})

        # Set directory links
        add_definitions(${${u_package}_DEFINITIONS})
      
        # Add dependency projects
        if(BUILD_${u_package})
            if(NOT DEFINED ${u_package}_PROJECT)
                set(${u_package}_PROJECT ${package})
            endif()
            add_dependencies(${target} ${${u_package}_PROJECT})
        endif(BUILD_${u_package})
      
        # Link library (Only link new libs)
        get_target_property(ivw_already_added_libs ${target} INTERFACE_LINK_LIBRARIES)
        if(NOT ivw_already_added_libs)
            set(ivw_already_added_libs "")
        endif()
        remove_from_list(ivw_new_libs "${${u_package}_LIBRARIES}" ${ivw_already_added_libs})
        target_link_libraries(${target} PUBLIC ${ivw_new_libs})
      
        # Link flags
        if(NOT "${${u_package}_LINK_FLAGS}" STREQUAL "")
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${${u_package}_LINK_FLAGS}")
        endif()
    endforeach()
endmacro()

#-------------------------------------------------------------------#
#                            QT stuff                               #
#-------------------------------------------------------------------#
# Wrap Qt CPP to create MOC files
macro(ivw_qt_wrap_cpp retval)
    qt5_wrap_cpp(the_list ${ARGN})
    set(${retval} ${the_list})
endmacro()

#--------------------------------------------------------------------
# Set automoc on a target
macro(ivw_qt_automoc project_name)
    set_target_properties(${project_name} PROPERTIES AUTOMOC TRUE)
endmacro()

#--------------------------------------------------------------------
# Define QT definitions
macro(ivw_define_qt_definitions)
    if(WIN32) 
        # Disable some warnings for qt modules.
        add_definitions( "/wd4800" ) # 'type' : forcing value to bool 'true' or 'false'
    endif()

    add_definitions(-DQT_CORE_LIB
                    -DQT_GUI_LIB)
endmacro()

#--------------------------------------------------------------------
# Adds special qt dependency and includes package variables to the project
macro(ivw_qt_add_to_install ivw_comp)
    foreach(qtarget ${ARGN})
        find_package(${qtarget} QUIET REQUIRED)
        if(IVW_PACKAGE_PROJECT)
            if(${qtarget}_FOUND)
                if(WIN32)
                    set(QTARGET_DIR "${${qtarget}_DIR}/../../../bin")
                    install(FILES ${QTARGET_DIR}/${qtarget}${CMAKE_DEBUG_POSTFIX}.dll 
                            DESTINATION bin 
                            COMPONENT ${ivw_comp} 
                            CONFIGURATIONS Debug)
                    install(FILES ${QTARGET_DIR}/${qtarget}.dll 
                            DESTINATION bin 
                            COMPONENT ${ivw_comp} 
                            CONFIGURATIONS Release)
                    foreach(plugin ${${qtarget}_PLUGINS})
                        get_target_property(_loc ${plugin} LOCATION)
                        get_filename_component(_path ${_loc} PATH)
                        get_filename_component(_dirname ${_path} NAME)
                        install(FILES ${_loc} 
                                DESTINATION bin/${_dirname} 
                                COMPONENT ${ivw_comp})
                    endforeach()
                elseif(APPLE)
                    foreach(plugin ${${qtarget}_PLUGINS})
                        get_target_property(_loc ${plugin} LOCATION)
                        get_filename_component(_path ${_loc} PATH)
                        get_filename_component(_dirname ${_path} NAME)
                        install(FILES ${_loc} 
                                DESTINATION Inviwo.app/Contents/plugins/${_dirname} 
                                COMPONENT ${ivw_comp})
                    endforeach()
                endif()
            endif()
        endif()
    endforeach()
endmacro()

#-------------------------------------------------------------------#
#                        Precompile headers                         #
#-------------------------------------------------------------------#
# Add directory to precompilied headers
macro(ivw_add_pch_path)
    list(APPEND _allPchDirs ${ARGN})
endmacro()

#--------------------------------------------------------------------
# Creates project with initial variables
macro(ivw_set_pch_disabled_for_module)
    set(_pchDisabledForThisModule TRUE)
endmacro()

#--------------------------------------------------------------------
# Set header ignore paths for cotire
macro(ivw_cotire_ignore)
    ivw_cotire_ignore_on_target(${_projectName})
endmacro()
macro(ivw_cotire_ignore_on_target target)
    get_target_property(COTIRE_PREFIX_HEADER_IGNORE_PATH ${target} COTIRE_PREFIX_HEADER_IGNORE_PATH)
    if(NOT COTIRE_PREFIX_HEADER_IGNORE_PATH)
        set(COTIRE_PREFIX_HEADER_IGNORE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    
    list(APPEND COTIRE_PREFIX_HEADER_IGNORE_PATH ${IVW_COTIRE_EXCLUDES})
    list(REMOVE_DUPLICATES COTIRE_PREFIX_HEADER_IGNORE_PATH)
    
    if (WIN32)
        # prevent definition of min and max macros through inclusion of Windows.h
        add_definitions("-DNOMINMAX")
        add_definitions("-DWIN32_LEAN_AND_MEAN")
    endif()
    
    set_target_properties(${target} PROPERTIES COTIRE_PREFIX_HEADER_IGNORE_PATH "${COTIRE_PREFIX_HEADER_IGNORE_PATH}")  
endmacro()

#--------------------------------------------------------------------
# Optimize compilation with pre-compilied headers from inviwo core
macro(ivw_compile_optimize_inviwo_core)
    ivw_compile_optimize_inviwo_core_on_target(${_projectName})
endmacro()

macro(ivw_compile_optimize_inviwo_core_on_target target)
    if(PRECOMPILED_HEADERS)
        if(_pchDisabledForThisModule)
            set_target_properties(${target} PROPERTIES COTIRE_ENABLE_PRECOMPILED_HEADER FALSE)
        endif()

        ivw_cotire_ignore_on_target(${target})
        set_target_properties(${target} PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
        get_target_property(_prefixHeader inviwo-core COTIRE_CXX_PREFIX_HEADER)
        set_target_properties(${target} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "${_prefixHeader}")
        cotire(${target})
    endif()
endmacro()

#--------------------------------------------------------------------
# Optimize compilation with pre-compilied headers
macro(ivw_compile_optimize)
    ivw_compile_optimize_on_target(${_projectName})
endmacro()
macro(ivw_compile_optimize_on_target target)
    if(PRECOMPILED_HEADERS)
        ivw_cotire_ignore_on_target(${target})
        set_target_properties(${target} PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
        list(APPEND _allPchDirs ${IVW_EXTENSIONS_DIR})
        set_target_properties(${target} PROPERTIES COTIRE_PREFIX_HEADER_INCLUDE_PATH "${_allPchDirs}")
        cotire(${target})
    endif()
endmacro()

#--------------------------------------------------------------------
# Suppres all compiler warnings
# ivw_suppress_compiler_warnings(target1 [target2 ...])
function(ivw_suppress_compiler_warnings)
    foreach(target ${ARGN})
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR 
            "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR
            "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
            set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS -w)

        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            get_property(comp_opts TARGET ${target} PROPERTY COMPILE_OPTIONS)
            string(REGEX REPLACE "(^|;)([/-])W[0-9](;|$)" ";" comp_opts "${comp_opts}")
            list(APPEND comp_opts "/W0")
            list(REMOVE_DUPLICATES comp_opts)
            set_property(TARGET ${target} PROPERTY COMPILE_OPTIONS ${comp_opts})
    
            get_property(comp_defs TARGET ${target} PROPERTY COMPILE_DEFINITIONS)
            list(APPEND comp_defs "_CRT_SECURE_NO_WARNINGS")
            list(REMOVE_DUPLICATES comp_defs)
            set_property(TARGET ${target} PROPERTY COMPILE_DEFINITIONS ${comp_defs})
            
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " /IGNORE:4006")
        endif()

        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
            set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GCC_WARN_INHIBIT_ALL_WARNINGS YES)
        endif()
    endforeach()
endfunction()


