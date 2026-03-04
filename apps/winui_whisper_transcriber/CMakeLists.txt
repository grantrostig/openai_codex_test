cmake_minimum_required(VERSION 3.28)
project(WinUiWhisperTranscriber LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Singleton_memory_mangager)
set (SRCS README.md
   src/App.xaml.cpp
   src/App.xaml.h
   src/App.xaml
   src/MainWindow.xaml
   src/MainWindow.xaml.cpp
   src/MainWindow.xaml.h
   src/pch.h
   src/SemanticTitleGenerator.cpp
   src/SemanticTitleGenerator.h
   src/TranscriptionService.cpp
   src/TranscriptionService.h
   docs/requirements.md
)
# Linux, I suppose?  TODO:
if(NOT WIN32)
  add_executable(winui_whisper_transcriber2
    src/App.xaml.cpp
    ${SRCS}
  )
  message(STATUS "WinUiWhisperTranscriber requires Windows to build and run.")
  # message(WARNING "WinUiWhisperTranscriber requires Windows to build and run.")
  # return()
endif()
# This project targets Windows + Visual Studio generator.
if(WIN32)
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
endif()
