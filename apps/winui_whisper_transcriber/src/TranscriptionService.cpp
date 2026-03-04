#include "TranscriptionService.h"
#include "SemanticTitleGenerator.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace WinUiWhisperTranscriber {

TranscriptionService::TranscriptionService(std::wstring whisper_executable_path, std::wstring whisper_model_path)
    : whisper_executable_path_(std::move(whisper_executable_path)), whisper_model_path_(std::move(whisper_model_path)) {}

std::wstring
TranscriptionService::InvokeWhisperExecutableAndReadOutput(const std::wstring &source_audio_path) const {
    const std::filesystem::path source_path(source_audio_path);
    const std::filesystem::path whisper_output_path = source_path.parent_path() / (source_path.stem().wstring() + L".txt");
    const std::wstring          whisper_command_line = L'"' + whisper_executable_path_ + L'"' + L" -m \"" + whisper_model_path_ + L"\" -f \"" +
                                              source_audio_path + L"\" -otxt -of \"" +
                                              (source_path.parent_path() / source_path.stem()).wstring() + L"\"";
    const int                   whisper_process_result = _wsystem(whisper_command_line.c_str());
    if(whisper_process_result != 0) { throw std::runtime_error("whisper.cpp process failed."); }
    std::wifstream              generated_transcript_file_stream(whisper_output_path);
    if(!generated_transcript_file_stream.good()) { throw std::runtime_error("Unable to read generated transcript from whisper.cpp output."); }
    std::wstring
    transcript_contents((std::istreambuf_iterator<wchar_t>(generated_transcript_file_stream)), std::istreambuf_iterator<wchar_t>());
    return transcript_contents;
}

ConvertedFileReport
TranscriptionService::ConvertSingleFile(const std::wstring &source_audio_path, const std::wstring &destination_folder_path) const {
    const std::wstring transcript_text = InvokeWhisperExecutableAndReadOutput(source_audio_path);
    const std::filesystem::path source_path(source_audio_path);
    const std::filesystem::path destination_path(destination_folder_path);

    const std::wstring semantic_title = SemanticTitleGenerator::BuildSemanticTitleFromTranscript(transcript_text);

    const std::wstring          output_file_name = source_path.stem().wstring() + L" - " + semantic_title + L".txt";
    const std::filesystem::path output_path      = destination_path / output_file_name;

    std::wofstream output_transcript_file_stream(output_path);
    output_transcript_file_stream << L"Title: " << semantic_title << L"\n\n";
    output_transcript_file_stream << transcript_text;

    return {
        .source_audio_path = source_audio_path,
        .output_text_path  = output_path.wstring(),
        .semantic_title    = semantic_title,
    };
}

winrt::Windows::Foundation::IAsyncOperation<std::vector<ConvertedFileReport>>
TranscriptionService::
    ConvertManyAudioFilesAsync(const std::vector<std::wstring> &source_audio_paths, const std::wstring &destination_folder_path) {
    co_await winrt::resume_background();

    std::vector<ConvertedFileReport> report_entries;
    report_entries.reserve(source_audio_paths.size());

    for(const auto &source_audio_path : source_audio_paths) {
        report_entries.push_back(ConvertSingleFile(source_audio_path, destination_folder_path));
    }

    co_return report_entries;
}

}
