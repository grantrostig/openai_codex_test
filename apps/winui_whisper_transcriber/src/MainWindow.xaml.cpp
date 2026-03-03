#include "pch.h"

#include "MainWindow.xaml.h"

#include <sstream>

#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;

namespace winrt::WinUiWhisperTranscriber::implementation
{
    MainWindow::MainWindow()
        : transcription_service_(L"C:\\tools\\whisper.cpp\\main.exe", L"C:\\tools\\whisper.cpp\\models\\ggml-base.en.bin")
    {
        InitializeComponent();
    }

    void MainWindow::AppendStatusMessage(const std::wstring& new_status_message) const
    {
        status_text_block().Text(hstring(new_status_message));
    }

    fire_and_forget MainWindow::OnSelectSourceAudioFilesClicked(IInspectable const&, RoutedEventArgs const&)
    {
        FileOpenPicker file_open_picker;
        file_open_picker.SuggestedStartLocation(PickerLocationId::MusicLibrary);
        file_open_picker.FileTypeFilter().Append(L".wav");
        file_open_picker.FileTypeFilter().Append(L".mp3");
        file_open_picker.FileTypeFilter().Append(L".m4a");
        file_open_picker.FileTypeFilter().Append(L".flac");

        const auto selected_files = co_await file_open_picker.PickMultipleFilesAsync();
        for (auto const& selected_file : selected_files)
        {
            queued_source_audio_paths_.push_back(selected_file.Path().c_str());
            queued_source_audio_files_list_view().Items().Append(box_value(selected_file.Path()));
        }

        std::wstringstream status_stream;
        status_stream << L"Queued " << queued_source_audio_paths_.size() << L" file(s). Add more files or convert now.";
        AppendStatusMessage(status_stream.str());
    }

    fire_and_forget MainWindow::OnSelectDestinationFolderClicked(IInspectable const&, RoutedEventArgs const&)
    {
        FolderPicker folder_picker;
        folder_picker.SuggestedStartLocation(PickerLocationId::DocumentsLibrary);
        folder_picker.FileTypeFilter().Append(L"*");

        const auto selected_folder = co_await folder_picker.PickSingleFolderAsync();
        if (selected_folder)
        {
            destination_folder_path_ = selected_folder.Path().c_str();
            selected_destination_folder_text_block().Text(L"Destination folder: " + selected_folder.Path());
            AppendStatusMessage(L"Destination folder selected.");
        }
    }

    fire_and_forget MainWindow::OnConvertAllFilesClicked(IInspectable const&, RoutedEventArgs const&)
    {
        if (queued_source_audio_paths_.empty())
        {
            AppendStatusMessage(L"No source files selected.");
            co_return;
        }
        if (destination_folder_path_.empty())
        {
            AppendStatusMessage(L"No destination folder selected.");
            co_return;
        }

        AppendStatusMessage(L"Converting files asynchronously...");

        try
        {
            const auto conversion_report = co_await transcription_service_.ConvertManyAudioFilesAsync(
                queued_source_audio_paths_,
                destination_folder_path_);

            converted_files_report_list_view().Items().Clear();
            for (const auto& report_entry : conversion_report)
            {
                const std::wstring report_line =
                    report_entry.source_audio_path + L" => " + report_entry.output_text_path +
                    L" [" + report_entry.semantic_title + L"]";
                converted_files_report_list_view().Items().Append(box_value(report_line));
            }

            queued_source_audio_paths_.clear();
            queued_source_audio_files_list_view().Items().Clear();

            AppendStatusMessage(L"Conversion complete. You can add more files and convert again.");
        }
        catch (const std::exception& exception)
        {
            const std::wstring error_message = L"Conversion failed: " + to_hstring(exception.what()).c_str();
            AppendStatusMessage(error_message);
        }
    }
}
