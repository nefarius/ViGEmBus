include(vcpkg_common_functions)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ViGEm/ViGEmClient
    REF v1.16.28.0
    SHA512 39a321d90655895135f56b9e611ba6230e98afe296ad59f1afe99671c4a4b9ac02feb57814207e7586f0eae04ce8fd875f715a1f3bb63750dd2f742fedb6af81
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
