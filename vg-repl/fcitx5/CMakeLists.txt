cmake_minimum_required(VERSION 3.16)
# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
project(zhuyinflow-fcitx5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Fcitx5Core 5.1.7 REQUIRED)

set(VG_INSTALL_ADDON_DIR "${CMAKE_INSTALL_PREFIX}/lib/fcitx5" CACHE PATH
  "Fcitx5 addon installation directory")
set(VG_INSTALL_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/fcitx5" CACHE PATH
  "Fcitx5 data installation directory")
set(VG_INSTALL_ICON_DIR "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps" CACHE PATH
  "Fcitx5 input method icon installation directory")
set(VG_INSTALL_RUNTIME_DIR "${VG_INSTALL_ADDON_DIR}" CACHE PATH
  "Private bridge and Vanguard installation directory")
set(VG_INSTALL_RPATH "$ORIGIN" CACHE STRING "Addon installed runtime search path")
set(VG_SYSTEM_TEXTMAP "" CACHE FILEPATH "Optional packaged factory dictionary fallback")

set(VG_BRIDGE_LIBRARY "" CACHE FILEPATH "Path to the built libvgbridge.so")
if(NOT EXISTS "${VG_BRIDGE_LIBRARY}")
  message(FATAL_ERROR "Set VG_BRIDGE_LIBRARY to the built libvgbridge.so")
endif()
set(VG_VANGUARD_LIBRARY "" CACHE FILEPATH "Path to the built libVanguard.so")
if(NOT EXISTS "${VG_VANGUARD_LIBRARY}")
  message(FATAL_ERROR "Set VG_VANGUARD_LIBRARY to the built libVanguard.so")
endif()

add_library(zhuyinflow SHARED src/VGEngine.cpp)
target_include_directories(zhuyinflow PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../Sources/VGBridge/include"
)
target_link_libraries(zhuyinflow PRIVATE Fcitx5::Core "${VG_BRIDGE_LIBRARY}")
target_compile_options(zhuyinflow PRIVATE -Wall -Wextra -Wpedantic)
if(VG_SYSTEM_TEXTMAP)
  target_compile_definitions(zhuyinflow PRIVATE VG_SYSTEM_TEXTMAP="${VG_SYSTEM_TEXTMAP}")
endif()
set_target_properties(zhuyinflow PROPERTIES
  OUTPUT_NAME zhuyinflow
  PREFIX ""
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "${VG_INSTALL_RPATH}"
)

include(CTest)
if(BUILD_TESTING)
  get_filename_component(VG_DEFAULT_TEST_TEXTMAP
    "${CMAKE_CURRENT_SOURCE_DIR}/../../upstream/Packages/vChewing_OSNeutral_LibVanguard/Sources/LXAssemblyMaterials4Tests/Resources/vanguardTextMap_test.txtMap" ABSOLUTE)
  set(VG_TEST_TEXTMAP "${VG_DEFAULT_TEST_TEXTMAP}" CACHE FILEPATH "Factory TextMap fixture for headless tests")
  if(NOT EXISTS "${VG_TEST_TEXTMAP}")
    message(FATAL_ERROR "Set VG_TEST_TEXTMAP to a valid factory TextMap for headless tests")
  endif()
  add_executable(key_event_adapter_test tests/key_event_adapter_test.cpp)
  target_link_libraries(key_event_adapter_test PRIVATE Fcitx5::Core)
  add_test(NAME key_event_adapter COMMAND key_event_adapter_test)

  find_package(PkgConfig REQUIRED)
  pkg_get_variable(FCITX_LIBDIR Fcitx5Core libdir)
  find_path(FCITX_TESTFRONTEND_INCLUDE_DIR testfrontend_public.h
    PATH_SUFFIXES Fcitx5/Module/fcitx-module/testfrontend REQUIRED
  )
  find_path(FCITX_TESTFRONTEND_DATADIR testing/addon/testfrontend.conf
    PATHS /usr/share/fcitx5 REQUIRED
  )

  add_executable(fcitx5_smoke_test tests/fcitx5_smoke_test.cpp)
  target_include_directories(fcitx5_smoke_test PRIVATE "${FCITX_TESTFRONTEND_INCLUDE_DIR}")
  target_link_libraries(fcitx5_smoke_test PRIVATE Fcitx5::Core)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-data/addon")
  configure_file(data/addon/zhuyinflow.conf "${CMAKE_CURRENT_BINARY_DIR}/test-data/addon/zhuyinflow.conf" COPYONLY)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-data/inputmethod")
  configure_file(data/inputmethod/zhuyinflow.conf "${CMAKE_CURRENT_BINARY_DIR}/test-data/inputmethod/zhuyinflow.conf" COPYONLY)
  function(zhuyinflow_isolate_test name)
    set(test_root "${CMAKE_CURRENT_BINARY_DIR}/test-state/${name}")
    file(MAKE_DIRECTORY "${test_root}/home" "${test_root}/config" "${test_root}/data" "${test_root}/cache" "${test_root}/runtime")
    configure_file(tests/profile.in "${test_root}/config/profile" COPYONLY)
    set_tests_properties(${name} PROPERTIES
      ENVIRONMENT "FCITX_ADDON_DIRS=${CMAKE_CURRENT_BINARY_DIR}:${FCITX_LIBDIR}/fcitx5;FCITX_DATA_DIRS=${CMAKE_CURRENT_BINARY_DIR}/test-data:/usr/share/fcitx5:${FCITX_TESTFRONTEND_DATADIR}/testing;FCITX_CONFIG_HOME=${test_root}/config;HOME=${test_root}/home;XDG_CONFIG_HOME=${test_root}/config;XDG_DATA_HOME=${test_root}/data;XDG_CACHE_HOME=${test_root}/cache;XDG_RUNTIME_DIR=${test_root}/runtime;DBUS_SESSION_BUS_ADDRESS=unix:path=${test_root}/no-session-bus;DISPLAY=;WAYLAND_DISPLAY=;ZHUYINFLOW_TEXTMAP=${VG_TEST_TEXTMAP};ZHUYINFLOW_MIXED_ALPHANUMERICAL=1;ZHUYINFLOW_FURIOUS_TYPING_ZHUYIN=1"
      TIMEOUT 30
    )
  endfunction()
  add_test(NAME fcitx5_smoke COMMAND fcitx5_smoke_test)
  zhuyinflow_isolate_test(fcitx5_smoke)
  add_executable(fcitx5_regression_test tests/fcitx5_regression_test.cpp)
  target_link_libraries(fcitx5_regression_test PRIVATE Fcitx5::Core)
  foreach(scenario candidates focus focus-no-preedit focus-formatted focus-client-commit focus-formatted-client-commit missing-dictionary invalid-dictionary unsupported-key continuous-input)
    add_test(NAME fcitx5_${scenario} COMMAND fcitx5_regression_test ${scenario})
    zhuyinflow_isolate_test(fcitx5_${scenario})
  endforeach()
endif()

include(GNUInstallDirs)
install(TARGETS zhuyinflow LIBRARY DESTINATION "${VG_INSTALL_ADDON_DIR}")
install(FILES "${VG_BRIDGE_LIBRARY}" "${VG_VANGUARD_LIBRARY}"
  DESTINATION "${VG_INSTALL_RUNTIME_DIR}"
)
install(FILES data/addon/zhuyinflow.conf
  DESTINATION "${VG_INSTALL_DATA_DIR}/addon"
)
install(FILES data/inputmethod/zhuyinflow.conf
  DESTINATION "${VG_INSTALL_DATA_DIR}/inputmethod"
)
install(FILES data/icons/zhuyinflow.svg
  DESTINATION "${VG_INSTALL_ICON_DIR}"
)
