cmake_minimum_required(VERSION 3.21)

project(GuiTerminal VERSION 1.0.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(GUITERMINAL_BUILD_DEMO "Build the Win32 demo application" ${PROJECT_IS_TOP_LEVEL})

if(BUILD_SHARED_LIBS AND GUITERMINAL_BUILD_DEMO)
    message(FATAL_ERROR "GUITERMINAL_BUILD_DEMO requires a static GuiTerminal build. Set GUITERMINAL_BUILD_DEMO=OFF when BUILD_SHARED_LIBS=ON.")
endif()

set(GUITERMINAL_PUBLIC_HEADERS
    include/GuiTerminalBuffer.h
    include/GuiTerminalControl.h
    include/GuiTerminalC.h
    include/GuiTerminalParser.h
    include/GuiTerminalRenderer.h
)

add_library(GuiTerminal
    src/GuiTerminalBuffer.cpp
    src/GuiTerminalControl.cpp
    src/GuiTerminalC.cpp
    src/GuiTerminalParser.cpp
    src/GuiTerminalRenderer.cpp
)

add_library(GuiTerminal::GuiTerminal ALIAS GuiTerminal)

target_compile_features(GuiTerminal PUBLIC cxx_std_17)
target_include_directories(GuiTerminal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BUILD_SHARED_LIBS)
    target_sources(GuiTerminal PRIVATE
        src/dllmain.cpp
    )
    target_compile_definitions(GuiTerminal
        PUBLIC
            GUITERMINAL_SHARED
        PRIVATE
            GUITERMINAL_BUILD_DLL
            UNICODE
            _UNICODE
            _WINDOWS
    )
    if(MSVC)
        target_link_options(GuiTerminal PRIVATE
            "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/GuiTerminal.def"
        )
    endif()
else()
    target_compile_definitions(GuiTerminal PRIVATE
        UNICODE
        _UNICODE
        _LIB
    )
endif()

if(MSVC)
    target_compile_options(GuiTerminal PRIVATE /W3 /permissive- /sdl)
endif()

target_link_libraries(GuiTerminal
    PUBLIC
        d2d1
        dwrite
)

set_target_properties(GuiTerminal PROPERTIES
    DEBUG_POSTFIX d
    EXPORT_NAME GuiTerminal
)

if(GUITERMINAL_BUILD_DEMO)
    add_executable(GuiTerminalDemo WIN32
        demo/demo.cpp
        demo/demo.rc
        demo/resource.h
        demo/demo.ico
        demo/demo_small.ico
    )

    target_compile_features(GuiTerminalDemo PRIVATE cxx_std_17)
    target_compile_definitions(GuiTerminalDemo PRIVATE UNICODE _UNICODE)
    target_include_directories(GuiTerminalDemo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/demo
    )

    if(MSVC)
        target_compile_options(GuiTerminalDemo PRIVATE /W3 /permissive- /sdl)
    endif()

    target_link_libraries(GuiTerminalDemo PRIVATE
        GuiTerminal::GuiTerminal
        shcore
    )

    set_target_properties(GuiTerminalDemo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/GuiTerminalDemo/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/$<CONFIG>
    )
endif()

install(TARGETS GuiTerminal
    EXPORT GuiTerminalTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_SHARED_LIBS)
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/GuiTerminalC.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
else()
    install(
        FILES ${GUITERMINAL_PUBLIC_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

install(EXPORT GuiTerminalTargets
    FILE GuiTerminalTargets.cmake
    NAMESPACE GuiTerminal::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GuiTerminal
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GuiTerminalConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/GuiTerminalConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GuiTerminal
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/GuiTerminalConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/GuiTerminalConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/GuiTerminalConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GuiTerminal
)
