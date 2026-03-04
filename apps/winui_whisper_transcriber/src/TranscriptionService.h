#pragma once

#include <string>
#include <vector>

#include <winrt/Windows.Foundation.h>

namespace WinUiWhisperTranscriber {

struct ConvertedFileReport {
    std::wstring source_audio_path;
    std::wstring output_text_path;
    std::wstring semantic_title;
};

class TranscriptionService {
    [[nodiscard]] ConvertedFileReport ConvertSingleFile( const std::wstring& source_audio_path, const std::wstring& destination_folder_path) const;
    [[nodiscard]] std::wstring InvokeWhisperExecutableAndReadOutput(const std::wstring& source_audio_path) const;
    std::wstring whisper_executable_path_;
    std::wstring whisper_model_path_;
public:
    TranscriptionService(std::wstring whisper_executable_path, std::wstring whisper_model_path);

    winrt::Windows::Foundation::IAsyncOperation<std::vector<ConvertedFileReport>>
    ConvertManyAudioFilesAsync( const std::vector<std::wstring>& source_audio_paths, const std::wstring& destination_folder_path);

};
}
