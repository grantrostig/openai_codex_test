#pragma once
#include "App.g.h"

namespace winrt::WinUiWhisperTranscriber::implementation {
    struct App : AppT<App> {
        App();
        void OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const& event_args);
    private:
        winrt::Microsoft::UI::Xaml::Window app_window_{ nullptr };
    };
}

namespace winrt::WinUiWhisperTranscriber::factory_implementation {
    struct App : AppT<App, implementation::App> { };
}
