# Audio to Text GUI (C++ / xtd)

A Fedora-friendly desktop app written in C++ with `xtd::forms`.

It lets you:
- pick one or more audio files,
- run transcription through `whisper.cpp` (`whisper-cli`),
- save each transcript as a `.txt` next to the source file.

This repo now includes two GUI apps:
- `audio_to_text_xtd`: original app (saves next to source files).
- `audio_to_text_xtd_loop`: loop-focused app (explicit source selection + destination folder + convert loop).

Output file naming format:
- `<original-file-name> - <content-title>.txt`
- Example: `meeting-01 - Project Timeline Action Items.txt`

The title is generated from the transcript content (first meaningful words, stopwords removed).

## Prerequisites (Fedora)

Install build dependencies:

```bash
sudo dnf install -y cmake gcc-c++ make git gtk3-devel
```

Install `xtd` (if not already installed) and make sure CMake can find it.

Install transcription backend (`whisper.cpp`):

```bash
git clone https://github.com/ggml-org/whisper.cpp
cd whisper.cpp
cmake -B build -DWHISPER_BUILD_EXAMPLES=ON
cmake --build build -j
```

The CLI executable will usually be at:
- `whisper.cpp/build/bin/whisper-cli`

Download a model file (for example `ggml-base.en.bin`) and note its path.

## Build

From this project root:

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/audio_to_text_xtd
```

Loop app:

```bash
./build/audio_to_text_xtd_loop
```

In the UI:
1. Set `whisper-cli path` (or leave as `whisper-cli` if it is on your PATH).
2. Select your whisper model `.bin` file.
3. Optionally set language (`en`, `es`, etc.).
4. Click `Select Audio Files`.
5. Click `Transcribe and Save`.

Loop app flow:
1. Set `whisper-cli path`, model, and optional language.
2. Choose source audio files.
3. Choose destination folder.
4. Click `Convert`.
5. Review converted files list and conversion log.
6. After completion, choose `Yes` to pick more files and run another batch.

## Notes

- This app shells out to `whisper-cli`; it does not bundle speech models.
- For long files, transcription may take time; progress updates per file.
