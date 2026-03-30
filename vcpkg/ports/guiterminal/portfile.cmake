set(SOURCE_PATH "${CURRENT_PORT_DIR}/../../..")
get_filename_component(SOURCE_PATH "${SOURCE_PATH}" ABSOLUTE)

if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    set(GUITERMINAL_BUILD_SHARED ON)
else()
    set(GUITERMINAL_BUILD_SHARED OFF)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_SHARED_LIBS=${GUITERMINAL_BUILD_SHARED}
        -DGUITERMINAL_BUILD_DEMO=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/GuiTerminal)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
