#pragma once

#include <string>

namespace WinUiWhisperTranscriber
{
    class SemanticTitleGenerator
    {
    public:
        [[nodiscard]] static std::wstring BuildSemanticTitleFromTranscript(const std::wstring& transcript_text);

    private:
        [[nodiscard]] static std::wstring NormalizeForTokenization(const std::wstring& input_text);
    };
}
