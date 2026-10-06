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
set_target_properties(zhuyinflow PROPERTIES
  OUTPUT_NAME zhuyinflow
  PREFIX ""
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
)

include(CTest)
if(BUILD_TESTING)
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
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-config")
  configure_file(tests/profile.in "${CMAKE_CURRENT_BINARY_DIR}/test-config/profile" COPYONLY)
  add_test(NAME fcitx5_smoke COMMAND fcitx5_smoke_test)
  set_tests_properties(fcitx5_smoke PROPERTIES
    ENVIRONMENT
      "FCITX_ADDON_DIRS=${VG_INSTALL_ADDON_DIR}:${FCITX_LIBDIR}/fcitx5;FCITX_DATA_DIRS=${VG_INSTALL_DATA_DIR}:/usr/share/fcitx5:${FCITX_TESTFRONTEND_DATADIR}/testing;FCITX_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-config;ZHUYINFLOW_MIXED_ALPHANUMERICAL=1"
  )
endif()

include(GNUInstallDirs)
install(TARGETS zhuyinflow LIBRARY DESTINATION "${VG_INSTALL_ADDON_DIR}")
install(FILES "${VG_BRIDGE_LIBRARY}" "${VG_VANGUARD_LIBRARY}"
  DESTINATION "${VG_INSTALL_ADDON_DIR}"
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
