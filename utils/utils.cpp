#include "utils.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <boost/locale.hpp>

namespace search_utils
{

    std::string html_escape(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
            }
        }
        return out;
    }

    std::string url_decode(const std::string& s)
    {
        std::string r;
        r.reserve(s.size());
        for (std::size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '+') r += ' ';
            else if (s[i] == '%' && i + 2 < s.size())
            {
                int val = 0;
                std::istringstream iss(s.substr(i + 1, 2));
                if (iss >> std::hex >> val) { r += static_cast<char>(val); i += 2; }
                else r += s[i];
            }
            else r += s[i];
        }
        return r;
    }

    std::vector<std::string> get_keywords(const std::string& raw)
    {
        std::vector<std::string> kws;
        std::istringstream iss(raw);
        std::string token;
        while (iss >> token)
        {
            std::string lower = boost::locale::to_lower(token);
            if (utf8_char_count(lower) > 2)
                kws.push_back(lower);
        }
        std::sort(kws.begin(), kws.end());
        kws.erase(std::unique(kws.begin(), kws.end()), kws.end());
        return kws;
    }

    size_t utf8_char_count(const std::string& s)
    {
        size_t count = 0;
        for (size_t i = 0; i < s.size(); )
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c < 0x80)       i += 1;
            else if (c < 0xE0)  i += 2;
            else if (c < 0xF0)  i += 3;
            else                 i += 4;
            ++count;
        }
        return count;
    }

    bool is_stop_word(const std::string& word, const std::set<std::string>& stop_words)
    {
        return stop_words.count(word) > 0;
    }

    std::map<std::string, unsigned int> count_words(const std::string& text,
        const std::set<std::string>& stop_words)
    {
        constexpr size_t min_word_chars = 2;  // > 2 символов, т.е. 3+
        std::map<std::string, unsigned int> result;

        std::string lower_text = boost::locale::to_lower(text);

        std::string word;
        for (size_t i = 0; i < lower_text.size(); )
        {
            unsigned char c = static_cast<unsigned char>(lower_text[i]);

            int char_len = 1;
            bool is_letter = false;

            if (c < 0x80)
            {
                char_len = 1;
                is_letter = std::isalpha(c);
            }
            else if (c < 0xC0)
            {
                char_len = 1;
                is_letter = false;
            }
            else if (c < 0xE0)
            {
                char_len = 2;
                is_letter = true;
            }
            else if (c < 0xF0)
            {
                char_len = 3;
                is_letter = true;
            }
            else
            {
                char_len = 4;
                is_letter = true;
            }

            if (i + char_len > lower_text.size())
            {
                ++i;
                continue;
            }

            if (is_letter)
            {
                word.append(lower_text, i, char_len);
            }
            else
            {
                if (!word.empty())
                {
                    size_t chars = utf8_char_count(word);
                    if (chars > min_word_chars && !is_stop_word(word, stop_words))
                        result[word]++;
                    word.clear();
                }
            }

            i += char_len;
        }

        if (!word.empty())
        {
            size_t chars = utf8_char_count(word);
            if (chars > min_word_chars && !is_stop_word(word, stop_words))
                result[word]++;
        }

        return result;
    }

    // ============================================================
    //  Определение кодировки: UTF-8 или Windows-1251
    // ============================================================
    bool is_valid_utf8(const std::string& text)
    {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text.data());
        size_t len = text.size();
        size_t i = 0;
        while (i < len)
        {
            unsigned char c = bytes[i];
            int expected = 0;
            if (c < 0x80) { ++i; continue; }
            else if ((c & 0xE0) == 0xC0) expected = 1;
            else if ((c & 0xF0) == 0xE0) expected = 2;
            else if ((c & 0xF8) == 0xF0) expected = 3;
            else return false;
            if (i + expected >= len) return false;
            for (int j = 1; j <= expected; ++j)
                if ((bytes[i + j] & 0xC0) != 0x80) return false;
            i += 1 + expected;
        }
        return true;
    }

    std::string convert_to_utf8(const std::string& text)
    {
        if (is_valid_utf8(text)) return text;
        try { return boost::locale::conv::to_utf<char>(text, "Windows-1251"); }
        catch (...) {
            try { return boost::locale::conv::to_utf<char>(text, "Latin1"); }
            catch (...) { return text; }
        }
    }

    std::string read_file_as_utf8(const std::string& filepath)
    {
        std::ifstream f(filepath, std::ios::in | std::ios::binary);
        if (!f.is_open()) return {};

        std::string raw((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());

        if (raw.empty()) return {};
        return convert_to_utf8(raw);
    }

} // namespace search_utils
