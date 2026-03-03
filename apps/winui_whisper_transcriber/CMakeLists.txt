cmake_minimum_required(VERSION 3.28)
project(WinUiWhisperTranscriber LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This project targets Windows + Visual Studio generator.
if(NOT WIN32)
  message(WARNING "WinUiWhisperTranscriber requires Windows to build and run.")
  return()
endif()

add_executable(winui_whisper_transcriber WIN32
  src/App.xaml
  src/App.xaml.cpp
  src/App.xaml.h
  src/MainWindow.xaml
  src/MainWindow.xaml.cpp
  src/MainWindow.xaml.h
  src/SemanticTitleGenerator.cpp
  src/SemanticTitleGenerator.h
  src/TranscriptionService.cpp
  src/TranscriptionService.h
)

target_compile_definitions(winui_whisper_transcriber PRIVATE UNICODE _UNICODE)

target_compile_options(winui_whisper_transcriber PRIVATE /W4 /permissive-)

target_link_libraries(winui_whisper_transcriber PRIVATE windowsapp)

message(STATUS "Use Visual Studio with Windows App SDK and C++/WinRT tooling installed.")
