include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ViGEm/ViGEmClient
    REF v1.16.18.0
    SHA512 7628b91197a9db42c7e6641ef4ac6aeb1854d199312b0a6ac3fbfe281354495e8f597865000b90c12ad36575aaf7a3da34da04a6efa8bb64bb9d47878640ac2c
    HEAD_REF master
)

if (VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
    vcpkg_install_msbuild(
        SOURCE_PATH ${SOURCE_PATH}
        PROJECT_SUBPATH "src/ViGEmClient.vcxproj"
        INCLUDES_SUBPATH "include"
        PLATFORM ${VCPKG_TARGET_ARCHITECTURE}
        RELEASE_CONFIGURATION Release_DLL
        DEBUG_CONFIGURATION Debug_DLL
        USE_VCPKG_INTEGRATION
        REMOVE_ROOT_INCLUDES)
elseif (VCPKG_LIBRARY_LINKAGE STREQUAL static)
    vcpkg_install_msbuild(
        SOURCE_PATH ${SOURCE_PATH}
        PROJECT_SUBPATH "src/ViGEmClient.vcxproj"
        INCLUDES_SUBPATH "include"
        PLATFORM ${VCPKG_TARGET_ARCHITECTURE}
        RELEASE_CONFIGURATION Release_LIB
        DEBUG_CONFIGURATION Debug_LIB
        USE_VCPKG_INTEGRATION
        REMOVE_ROOT_INCLUDES)
endif()

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/vigemclient RENAME copyright)
