vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

set(CRP_ROOT "${CURRENT_PORT_DIR}/..")
set(CRP_LIVE555_DIR "${CRP_ROOT}/thirdparty/live555")

foreach(_required IN ITEMS
    "${CRP_ROOT}/src"
    "${CRP_ROOT}/src/coralreefplayer.h.in"
    "${CRP_ROOT}/src/version.h.in"
    "${CRP_LIVE555_DIR}/CMakeLists.txt"
)
    if(NOT EXISTS "${_required}")
        message(FATAL_ERROR "Required CoralReefPlayer file/dir is missing: ${_required}")
    endif()
endforeach()

vcpkg_cmake_configure(
    SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}"
    OPTIONS
        "-DCRP_ROOT=${CRP_ROOT}"
        "-DCRP_LIVE555_DIR=${CRP_LIVE555_DIR}"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME coralreefplayer CONFIG_PATH share/coralreefplayer)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${CRP_ROOT}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
