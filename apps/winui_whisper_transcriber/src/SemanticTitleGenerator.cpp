#include "SemanticTitleGenerator.h"

#include <algorithm>
#include <cwctype>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace WinUiWhisperTranscriber
{
    std::wstring SemanticTitleGenerator::NormalizeForTokenization(const std::wstring& input_text)
    {
        std::wstring normalized_text = input_text;
        std::ranges::transform(normalized_text, normalized_text.begin(), [](wchar_t current_character)
        {
            if (std::iswalnum(current_character) || std::iswspace(current_character))
            {
                return static_cast<wchar_t>(std::towlower(current_character));
            }
            return L' ';
        });
        return normalized_text;
    }

    std::wstring SemanticTitleGenerator::BuildSemanticTitleFromTranscript(const std::wstring& transcript_text)
    {
        static const std::set<std::wstring> stop_word_dictionary {
            L"a", L"an", L"and", L"are", L"as", L"at", L"be", L"by", L"for", L"from", L"has", L"he", L"in",
            L"is", L"it", L"its", L"of", L"on", L"or", L"that", L"the", L"to", L"was", L"were", L"will", L"with",
            L"this", L"we", L"you", L"i", L"they", L"them", L"our"
        };

        const std::wstring normalized_text = NormalizeForTokenization(transcript_text);

        std::wistringstream token_stream(normalized_text);
        std::map<std::wstring, int> token_frequencies;

        for (std::wstring current_token; token_stream >> current_token;)
        {
            if (current_token.size() < 4 || stop_word_dictionary.contains(current_token))
            {
                continue;
            }
            ++token_frequencies[current_token];
        }

        if (token_frequencies.empty())
        {
            return L"General Audio Notes";
        }

        std::vector<std::pair<std::wstring, int>> ranked_tokens(token_frequencies.begin(), token_frequencies.end());
        std::ranges::sort(ranked_tokens, [](const auto& left, const auto& right)
        {
            if (left.second != right.second)
            {
                return left.second > right.second;
            }
            return left.first < right.first;
        });

        std::wstring semantic_title = L"Transcript";
        const std::size_t number_of_keywords_to_use = std::min<std::size_t>(3, ranked_tokens.size());
        for (std::size_t keyword_index = 0; keyword_index < number_of_keywords_to_use; ++keyword_index)
        {
            semantic_title += (keyword_index == 0 ? L": " : L", ");
            semantic_title += ranked_tokens.at(keyword_index).first;
        }

        return semantic_title;
    }
}
