cmake_minimum_required(VERSION 3.30)
project(audio_to_text_xtd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(xtd REQUIRED COMPONENTS forms drawing)

add_executable(audio_to_text_xtd
  src/main.cpp
)
target_link_libraries(audio_to_text_xtd PRIVATE xtd.forms xtd.drawing)

add_executable(audio_to_text_xtd_loop
  src/main_loop.cpp
)
target_link_libraries(audio_to_text_xtd_loop PRIVATE xtd.forms xtd.drawing)

include(GNUInstallDirs)
install(TARGETS audio_to_text_xtd
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS audio_to_text_xtd_loop
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
