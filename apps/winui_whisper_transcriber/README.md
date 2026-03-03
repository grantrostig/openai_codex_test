# WinUiWhisperTranscriber

A Windows App SDK (WinUI 3 + C++/WinRT) desktop app for local speech-to-text batch conversion.

## What it does

- Lets users add multiple audio files repeatedly (queue-style loop).
- Lets users select an output destination folder.
- Converts queued audio files asynchronously through `whisper.cpp` CLI.
- Produces `.txt` files named with the original filename plus a semantic title.
- Shows a conversion report listing source, destination, and generated title.

## Build notes

- Target platform: Windows 11/10.
- Recommended IDE: Visual Studio 2022 with **Windows App SDK** and **C++/WinRT** workloads.
- This repository includes `CMakeLists.txt` for project orchestration, but WinUI 3 requires Windows-specific toolchain integration.

## Runtime dependency

Configure paths in `MainWindow.xaml.cpp` constructor:

- `whisper_executable_path`: path to `main.exe` from `whisper.cpp`.
- `whisper_model_path`: model file such as `ggml-base.en.bin`.

## High-level flow

```mermaid
flowchart TD
    A[User clicks Add audio files] --> B[Queue files in ListView]
    B --> C[User selects destination folder]
    C --> D[User clicks Convert]
    D --> E[Run async conversion on background thread]
    E --> F[Invoke whisper.cpp CLI per file]
    F --> G[Generate semantic title from transcript text]
    G --> H[Write destination text file]
    H --> I[Display conversion report]
    I --> A
```

## Class diagram

```mermaid
classDiagram
    class MainWindow {
      -vector~wstring~ queued_source_audio_paths_
      -wstring destination_folder_path_
      -TranscriptionService transcription_service_
      +OnSelectSourceAudioFilesClicked()
      +OnSelectDestinationFolderClicked()
      +OnConvertAllFilesClicked()
    }

    class TranscriptionService {
      -wstring whisper_executable_path_
      -wstring whisper_model_path_
      +ConvertManyAudioFilesAsync(paths, destination)
      -InvokeWhisperExecutableAndReadOutput(path)
      -BuildTranscriptForSingleFile(path, destination)
    }

    class SemanticTitleGenerator {
      +BuildSemanticTitleFromTranscript(text)
      -NormalizeForTokenization(text)
    }

    MainWindow --> TranscriptionService
    TranscriptionService --> SemanticTitleGenerator
```
