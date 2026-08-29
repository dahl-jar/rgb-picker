cmake_minimum_required(VERSION 3.22)

set(RGBPICKER_VERSION "1.0.0" CACHE STRING "RGB Picker release version")
if(NOT RGBPICKER_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "RGBPICKER_VERSION must use MAJOR.MINOR.PATCH")
endif()

project(rgb-picker VERSION ${RGBPICKER_VERSION} LANGUAGES C CXX)

include(CTest)

option(RGBPICKER_BUILD_CLI "Build the rgb-ctl CLI" ON)
option(RGBPICKER_BUILD_GUI "Build the rgb-picker desktop app (Windows)" ${WIN32})
option(RGBPICKER_ENABLE_CLANG_TIDY "Run clang-tidy while compiling first-party C++" OFF)
option(
    RGBPICKER_REQUIRE_MINGW_RUNTIME_DLLS
    "Fail configuration when MinGW runtime DLLs cannot be packaged"
    OFF
)
option(RGBPICKER_WARNINGS_AS_ERRORS "Treat first-party compiler warnings as errors" OFF)

if(RGBPICKER_ENABLE_CLANG_TIDY)
    find_program(RGBPICKER_CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
endif()

function(rgbpicker_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(RGBPICKER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -Wshadow
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Wcast-align
                -Wunused
                -Woverloaded-virtual
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wnull-dereference
                -Wdouble-promotion
                -Wformat=2
                -Wimplicit-fallthrough
        )
        if(RGBPICKER_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(rgbpicker_enable_clang_tidy target)
    if(RGBPICKER_ENABLE_CLANG_TIDY)
        set_property(
            TARGET ${target}
            PROPERTY CXX_CLANG_TIDY
                "${RGBPICKER_CLANG_TIDY_EXECUTABLE};--warnings-as-errors=*"
        )
    endif()
endfunction()

function(rgbpicker_target_defaults target)
    target_compile_features(${target} PUBLIC cxx_std_23)
    target_include_directories(
        ${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    rgbpicker_enable_warnings(${target})
    rgbpicker_enable_clang_tidy(${target})
endfunction()

add_library(rgbpicker_core src/core/color.cpp)
rgbpicker_target_defaults(rgbpicker_core)

add_library(
    rgbpicker_storage
    src/storage/config_file.cpp
    src/storage/profiles.cpp
    src/storage/settings.cpp
)
rgbpicker_target_defaults(rgbpicker_storage)
target_link_libraries(rgbpicker_storage PUBLIC rgbpicker_core)

if(WIN32)
    set(openrgb_shim ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openrgb/shim)
    set(hidapi_vendor ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hidapi)

    add_library(hidapi STATIC ${hidapi_vendor}/windows/hid.c)
    target_include_directories(hidapi SYSTEM PUBLIC ${hidapi_vendor}/include)
    target_link_libraries(hidapi PUBLIC setupapi hid)

    set(openrgb_fork ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openrgb-src)
    set(lianli_fork ${openrgb_fork}/Controllers/LianLiController)
    set(libusb_vendor ${openrgb_fork}/dependencies/libusb-1.0.27)

    add_library(
        openrgb_drivers STATIC
        ${lianli_fork}/LianLiControllerDetect.cpp
        ${lianli_fork}/LianLiGAIITrinityController/LianLiGAIITrinityController.cpp
        ${lianli_fork}/LianLiGAIITrinityController/RGBController_LianLiGAIITrinity.cpp
        ${lianli_fork}/LianLiStrimerLConnectController/LianLiStrimerLConnectController.cpp
        ${lianli_fork}/LianLiStrimerLConnectController/RGBController_LianLiStrimerLConnect.cpp
        ${lianli_fork}/LianLiUniHubALController/LianLiUniHubALController.cpp
        ${lianli_fork}/LianLiUniHubALController/RGBController_LianLiUniHubAL.cpp
        ${lianli_fork}/LianLiUniHubController/LianLiUniHubController.cpp
        ${lianli_fork}/LianLiUniHubController/RGBController_LianLiUniHub.cpp
        ${lianli_fork}/LianLiUniHubSLController/LianLiUniHubSLController.cpp
        ${lianli_fork}/LianLiUniHubSLController/RGBController_LianLiUniHubSL.cpp
        ${lianli_fork}/LianLiUniHubSLInfinityController/LianLiUniHubSLInfinityController.cpp
        ${lianli_fork}/LianLiUniHubSLInfinityController/RGBController_LianLiUniHubSLInfinity.cpp
        ${lianli_fork}/LianLiUniHubSLV2Controller/LianLiUniHubSLV2Controller.cpp
        ${lianli_fork}/LianLiUniHubSLV2Controller/RGBController_LianLiUniHubSLV2.cpp
        ${lianli_fork}/LianLiUniHub_AL10Controller/LianLiUniHub_AL10Controller.cpp
        ${lianli_fork}/LianLiUniHub_AL10Controller/RGBController_LianLiUniHub_AL10.cpp
        ${lianli_fork}/LianLiUniversalScreenController/LianLiUniversalScreenController.cpp
        ${lianli_fork}/LianLiUniversalScreenController/RGBController_LianLiUniversalScreen.cpp
        ${openrgb_fork}/RGBController/RGBController.cpp
        ${openrgb_fork}/RGBController/RGBControllerKeyNames.cpp
        ${openrgb_fork}/StringUtils.cpp
        ${openrgb_fork}/Controllers/GigabyteRGBFusion2USBController/GigabyteRGBFusion2USBController.cpp
        ${openrgb_fork}/Controllers/GigabyteRGBFusion2USBController/GigabyteFusion2USB_Devices.cpp
        ${openrgb_fork}/Controllers/GigabyteRGBFusion2USBController/RGBController_GigabyteRGBFusion2USB.cpp
        ${openrgb_fork}/Controllers/GigabyteRGBFusion2USBController/GigabyteRGBFusion2USBControllerDetect.cpp
        ${openrgb_fork}/dmiinfo/dmiinfo.cpp
        ${openrgb_fork}/wmi/wmi.cpp
        ${openrgb_shim}/comsupp_mingw.cpp
        ${openrgb_fork}/i2c_smbus/i2c_smbus.cpp
        ${openrgb_fork}/i2c_smbus/Windows/i2c_smbus_nvapi.cpp
        ${openrgb_fork}/dependencies/NVFC/nvapi.cpp
        ${openrgb_fork}/Controllers/MSIGPUController/MSIGPUv2Controller/MSIGPUv2Controller.cpp
        ${openrgb_fork}/Controllers/MSIGPUController/MSIGPUv2Controller/RGBController_MSIGPUv2.cpp
        ${openrgb_fork}/Controllers/MSIGPUController/MSIGPUv2Controller/MSIGPUv2ControllerDetect.cpp
    )
    target_compile_features(openrgb_drivers PUBLIC cxx_std_17)
    target_include_directories(
        openrgb_drivers SYSTEM
        PUBLIC
            ${openrgb_fork}/RGBController
            ${openrgb_shim}
            ${openrgb_fork}/dependencies/json
            ${lianli_fork}/LianLiGAIITrinityController
            ${lianli_fork}/LianLiStrimerLConnectController
            ${lianli_fork}/LianLiUniHubALController
            ${lianli_fork}/LianLiUniHubController
            ${lianli_fork}/LianLiUniHubSLController
            ${lianli_fork}/LianLiUniHubSLInfinityController
            ${lianli_fork}/LianLiUniHubSLV2Controller
            ${lianli_fork}/LianLiUniHub_AL10Controller
            ${lianli_fork}/LianLiUniversalScreenController
            ${libusb_vendor}/include
            ${openrgb_fork}/Controllers/GigabyteRGBFusion2USBController
            ${openrgb_fork}/Controllers/MSIGPUController/MSIGPUv2Controller
            ${openrgb_fork}/i2c_smbus
            ${openrgb_fork}/i2c_smbus/Windows
            ${openrgb_fork}/dependencies/NVFC
            ${openrgb_fork}/pci_ids
            ${openrgb_fork}/dmiinfo
            ${openrgb_fork}/wmi
            ${openrgb_fork}
    )

    set(libusb_import ${CMAKE_CURRENT_BINARY_DIR}/libusb-1.0.dll.a)
    add_custom_command(
        OUTPUT ${libusb_import}
        COMMAND
            ${CMAKE_DLLTOOL} -d ${libusb_vendor}/libusb-1.0.def -D libusb-1.0.dll -l
            ${libusb_import}
        DEPENDS ${libusb_vendor}/libusb-1.0.def
        COMMENT "Building vendored libusb import library"
        VERBATIM
    )
    add_custom_target(libusb_import_lib DEPENDS ${libusb_import})
    add_dependencies(openrgb_drivers libusb_import_lib)
    target_link_libraries(openrgb_drivers PUBLIC hidapi wbemuuid ole32 oleaut32 ${libusb_import})

    function(rgbpicker_ship_libusb target)
        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                ${libusb_vendor}/MinGW64/dll/libusb-1.0.dll $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying vendored libusb next to ${target}"
        )
    endfunction()
endif()

if(RGBPICKER_BUILD_CLI OR RGBPICKER_BUILD_GUI OR BUILD_TESTING)
    add_library(
        rgbpicker_backend
        src/backend/device/device_rules.cpp
        src/backend/factory/runtime_backend_factory.cpp
        src/backend/hardware/driver_backend.cpp
        src/backend/hardware/hardware_backend.cpp
        src/backend/session/backend_session.cpp
    )
    rgbpicker_target_defaults(rgbpicker_backend)
    target_link_libraries(rgbpicker_backend PUBLIC rgbpicker_core PRIVATE rgbpicker_storage)
    if(WIN32)
        target_link_libraries(
            rgbpicker_backend PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,openrgb_drivers>"
        )
        target_include_directories(
            rgbpicker_backend SYSTEM
            PRIVATE
                ${openrgb_fork}/Controllers/MSIGPUController/MSIGPUv2Controller
                ${openrgb_fork}/i2c_smbus
                ${openrgb_shim}
        )
    endif()
endif()

if(RGBPICKER_BUILD_CLI OR BUILD_TESTING)
    add_library(rgbpicker_cli src/cli/cli.cpp src/cli/cli_options.cpp)
    rgbpicker_target_defaults(rgbpicker_cli)
    target_link_libraries(rgbpicker_cli PUBLIC rgbpicker_backend)
endif()

if(RGBPICKER_BUILD_CLI)
    add_executable(rgb-ctl src/cli/main.cpp)
    target_compile_features(rgb-ctl PRIVATE cxx_std_23)
    target_link_libraries(rgb-ctl PRIVATE rgbpicker_cli)
    rgbpicker_enable_warnings(rgb-ctl)
    rgbpicker_enable_clang_tidy(rgb-ctl)
    if(WIN32)
        rgbpicker_ship_libusb(rgb-ctl)
    endif()
endif()

if(RGBPICKER_BUILD_GUI)
    if(NOT WIN32)
        message(FATAL_ERROR "rgb-picker currently builds on Windows only")
    endif()

    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        dbb5eeaadffb6a3ba6a60de1290312e5802dba5a
    )
    FetchContent_MakeAvailable(imgui)

    add_library(
        imgui_win32_dx11 STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    )
    target_compile_features(imgui_win32_dx11 PUBLIC cxx_std_17)
    target_include_directories(
        imgui_win32_dx11 SYSTEM
        PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends
    )
    target_compile_definitions(imgui_win32_dx11 PUBLIC IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
    target_link_libraries(imgui_win32_dx11 PUBLIC d3d11 d3dcompiler dwmapi)

    enable_language(RC)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gui.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/gui.rc
        @ONLY
    )
    add_executable(
        rgb-picker WIN32
        src/gui_main.cpp
        src/gui/devices/properties.cpp
        src/gui/profiles/rail.cpp
        src/gui/app/session.cpp
        src/gui/settings/settings_dialog.cpp
        src/gui/app/shell.cpp
        src/gui/app/worker.cpp
        src/gui/devices/workspace.cpp
        src/gui/ui/theme.cpp
        src/gui/ui/widgets.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/gui.rc
    )
    target_include_directories(rgb-picker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_features(rgb-picker PRIVATE cxx_std_23)
    target_link_libraries(
        rgb-picker
        PRIVATE
            rgbpicker_backend
            rgbpicker_storage
            imgui_win32_dx11
            shell32
    )
    rgbpicker_enable_warnings(rgb-picker)
    rgbpicker_enable_clang_tidy(rgb-picker)

    add_custom_command(
        TARGET rgb-picker
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts
            $<TARGET_FILE_DIR:rgb-picker>/fonts
        COMMENT "Copying bundled fonts next to rgb-picker"
    )
    rgbpicker_ship_libusb(rgb-picker)
endif()

if(BUILD_TESTING)
    add_executable(
        rgb-picker-tests
        src/backend/device/device_rules_test.cpp
        src/backend/hardware/hardware_backend_test.cpp
        src/backend/session/backend_session_test.cpp
        src/cli/cli_contract_test.cpp
        src/core/color_test.cpp
        src/storage/profiles_test.cpp
        src/storage/settings_test.cpp
        tests/test_main.cpp
    )
    target_compile_features(rgb-picker-tests PRIVATE cxx_std_23)
    target_include_directories(
        rgb-picker-tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_link_libraries(
        rgb-picker-tests
        PRIVATE
            rgbpicker_cli
    )
    rgbpicker_enable_warnings(rgb-picker-tests)
    rgbpicker_enable_clang_tidy(rgb-picker-tests)
    if(WIN32)
        rgbpicker_ship_libusb(rgb-picker-tests)
    endif()
    add_test(NAME rgb-picker-tests COMMAND rgb-picker-tests)
endif()

if(WIN32 AND TARGET rgb-picker)
    install(TARGETS rgb-picker RUNTIME DESTINATION .)
    if(TARGET rgb-ctl)
        install(TARGETS rgb-ctl RUNTIME DESTINATION .)
    endif()

    install(FILES ${libusb_vendor}/MinGW64/dll/libusb-1.0.dll DESTINATION .)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/ DESTINATION fonts)
    install(
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
            ${CMAKE_CURRENT_SOURCE_DIR}/NOTICE.md
        DESTINATION .
    )
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hidapi/LICENSE-bsd.txt
        DESTINATION licenses
        RENAME hidapi-BSD-3-Clause.txt
    )
    install(
        FILES ${imgui_SOURCE_DIR}/LICENSE.txt
        DESTINATION licenses
        RENAME Dear-ImGui-MIT.txt
    )
    install(
        FILES ${libusb_vendor}/COPYING
        DESTINATION licenses
        RENAME libusb-LGPL-2.1.txt
    )

    if(MINGW)
        get_filename_component(mingw_binary_directory ${CMAKE_CXX_COMPILER} DIRECTORY)
        get_filename_component(mingw_prefix ${mingw_binary_directory} DIRECTORY)
        foreach(runtime_library IN ITEMS libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
            execute_process(
                COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${runtime_library}
                OUTPUT_VARIABLE runtime_library_path
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE runtime_lookup_result
            )
            if(
                runtime_lookup_result EQUAL 0
                AND NOT runtime_library_path STREQUAL runtime_library
                AND EXISTS ${runtime_library_path}
            )
                install(FILES ${runtime_library_path} DESTINATION .)
            elseif(RGBPICKER_REQUIRE_MINGW_RUNTIME_DLLS)
                message(FATAL_ERROR "Unable to locate MinGW runtime library: ${runtime_library}")
            endif()
        endforeach()
        install(
            DIRECTORY ${mingw_prefix}/share/licenses/gcc-libs/
            DESTINATION licenses/mingw-gcc
        )
        install(
            DIRECTORY ${mingw_prefix}/share/licenses/libwinpthread/
            DESTINATION licenses/mingw-winpthreads
        )
    endif()

    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_PACKAGE_NAME "RGB Picker")
    set(CPACK_PACKAGE_VENDOR "dahl-jar")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Windows RGB hardware controller")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_FILE_NAME "rgb-picker-${PROJECT_VERSION}-windows-x64")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "RGB Picker")
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
    set(CPACK_MONOLITHIC_INSTALL ON)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico rgbpicker_nsis_icon)
    set(CPACK_NSIS_DISPLAY_NAME "RGB Picker")
    set(CPACK_NSIS_PACKAGE_NAME "RGB Picker")
    set(CPACK_NSIS_MUI_ICON ${rgbpicker_nsis_icon})
    set(CPACK_NSIS_MUI_UNIICON ${rgbpicker_nsis_icon})
    set(CPACK_NSIS_INSTALLED_ICON_NAME "rgb-picker.exe")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "rgb-picker.exe")
    set(CPACK_NSIS_MENU_LINKS "rgb-picker.exe" "RGB Picker")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    include(CPack)
endif()
