#pragma once

#include "MainWindow.g.h"

#include "TranscriptionService.h"

#include <vector>

namespace winrt::WinUiWhisperTranscriber::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        winrt::fire_and_forget OnSelectSourceAudioFilesClicked(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& event_args);

        winrt::fire_and_forget OnSelectDestinationFolderClicked(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& event_args);

        winrt::fire_and_forget OnConvertAllFilesClicked(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& event_args);

    private:
        void AppendStatusMessage(const std::wstring& new_status_message) const;

        std::vector<std::wstring> queued_source_audio_paths_;
        std::wstring destination_folder_path_;
        WinUiWhisperTranscriber::TranscriptionService transcription_service_;
    };
}

namespace winrt::WinUiWhisperTranscriber::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
