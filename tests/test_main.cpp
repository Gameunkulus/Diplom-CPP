#include <gtest/gtest.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <boost/locale.hpp>

// Линкуем реальный модуль utils — тесты проверяют настоящий код
#include "../utils/utils.h"
#include "../ini_file/ini_file.h"

// ============================================================
//  TF-IDF формула (standalone — нет в utils, проверяем математику)
// ============================================================
double calc_tfidf(int term_count, int total_words_in_doc, int total_docs, int docs_with_term)
{
    if (docs_with_term == 0 || total_words_in_doc == 0) return 0.0;
    double tf = static_cast<double>(term_count) / static_cast<double>(total_words_in_doc);
    double idf = std::log(static_cast<double>(total_docs) / static_cast<double>(docs_with_term));
    return tf * idf;
}

// ============================================================
//  ТЕСТЫ — search_utils
// ============================================================

// Тест 1: html_escape корректно экранирует спецсимволы
TEST(HtmlEscape, EscapesSpecialCharacters)
{
    EXPECT_EQ(search_utils::html_escape("<script>alert('xss')</script>"),
        "&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;");
    EXPECT_EQ(search_utils::html_escape("Tom & Jerry"), "Tom &amp; Jerry");
    EXPECT_EQ(search_utils::html_escape("\"hello\""), "&quot;hello&quot;");
    EXPECT_EQ(search_utils::html_escape("plain text"), "plain text");
    EXPECT_EQ(search_utils::html_escape(""), "");
}

// Тест 2: url_decode декодирует URL-encoded строки
TEST(UrlDecode, DecodesUrlEncoding)
{
    EXPECT_EQ(search_utils::url_decode("hello+world"), "hello world");
    EXPECT_EQ(search_utils::url_decode("C%3A%5Cpath"), "C:\\path");
    EXPECT_EQ(search_utils::url_decode("%D0%BF%D1%80%D0%B8%D0%B2%D0%B5%D1%82"),
        "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82");
    EXPECT_EQ(search_utils::url_decode("no+encoding+needed"), "no encoding needed");
    EXPECT_EQ(search_utils::url_decode(""), "");
}

// Тест 3: get_keywords разбивает строку и фильтрует короткие слова (<=2 Unicode символов)
TEST(GetKeywords, SplitsAndFilters)
{
    boost::locale::generator gen;
    std::locale::global(gen(""));

    auto kw = search_utils::get_keywords("Programming Language");
    EXPECT_EQ(kw.size(), 2);
    EXPECT_EQ(kw[0], "language");
    EXPECT_EQ(kw[1], "programming");

    // "I"(1), "am"(2), "a"(1) отфильтрованы; "big"(3) и "cat"(3) остаются
    auto kw2 = search_utils::get_keywords("I am a big cat");
    EXPECT_EQ(kw2.size(), 2);
    EXPECT_EQ(kw2[0], "big");
    EXPECT_EQ(kw2[1], "cat");

    // Все слова <= 2 символов — пусто
    auto kw_short = search_utils::get_keywords("I go to my ox");
    EXPECT_EQ(kw_short.size(), 0);

    // Дубликаты убираются
    auto kw3 = search_utils::get_keywords("test Test TEST");
    EXPECT_EQ(kw3.size(), 1);
    EXPECT_EQ(kw3[0], "test");
}

// Тест 4: is_stop_word фильтрует стоп-слова
TEST(StopWords, FiltersCommonWords)
{
    std::set<std::string> stops = {
        "the", "and", "for", "are", "but", "not", "you",
        "all", "can", "had", "her", "was", "one", "our",
        "и", "в", "на", "не", "что", "он", "она"
    };

    EXPECT_TRUE(search_utils::is_stop_word("the", stops));
    EXPECT_TRUE(search_utils::is_stop_word("and", stops));
    EXPECT_TRUE(search_utils::is_stop_word("и", stops));
    EXPECT_TRUE(search_utils::is_stop_word("на", stops));
    EXPECT_FALSE(search_utils::is_stop_word("programming", stops));
    EXPECT_FALSE(search_utils::is_stop_word("algorithm", stops));
    EXPECT_FALSE(search_utils::is_stop_word("программирование", stops));
}

// Тест 5: count_words подсчитывает частоту слов (реальная функция из utils)
TEST(CountWords, CountsWordFrequency)
{
    boost::locale::generator gen;
    std::locale::global(gen(""));

    std::set<std::string> stops = {
        "the", "and", "for", "are", "but", "not", "you",
        "all", "can", "had", "her", "was", "one", "our"
    };

    auto words = search_utils::count_words(
        "Python is a popular programming language Python", stops);

    EXPECT_EQ(words["python"], 2u);
    EXPECT_EQ(words["popular"], 1u);
    EXPECT_EQ(words["programming"], 1u);
    EXPECT_EQ(words["language"], 1u);
    // "is" (2 символа) отфильтрован по длине
    EXPECT_EQ(words.count("is"), 0u);
    // "a" (1 символ) отфильтрован по длине
    EXPECT_EQ(words.count("a"), 0u);
}

// Тест 6: TF-IDF математика
TEST(TfIdf, CalculatesCorrectly)
{
    double score = calc_tfidf(2, 10, 100, 10);
    EXPECT_NEAR(score, 0.4605, 0.001);

    double score_common = calc_tfidf(5, 10, 100, 100);
    EXPECT_NEAR(score_common, 0.0, 0.001);

    double score_rare = calc_tfidf(1, 10, 1000, 1);
    EXPECT_NEAR(score_rare, 0.6908, 0.001);

    EXPECT_EQ(calc_tfidf(0, 0, 0, 0), 0.0);
}

// Тест 7: INI-файл формат ключей
TEST(IniFormat, ValidatesKeyValueFormat)
{
    std::string full_key = "DataBase.bd_host";
    auto dot_pos = full_key.find('.');
    ASSERT_NE(dot_pos, std::string::npos);

    std::string section = full_key.substr(0, dot_pos);
    std::string key = full_key.substr(dot_pos + 1);

    EXPECT_EQ(section, "DataBase");
    EXPECT_EQ(key, "bd_host");

    std::string no_section = "hostname";
    EXPECT_EQ(no_section.find('.'), std::string::npos);
}

// Тест 8: utf8_char_count считает символы, не байты
TEST(Utf8CharCount, CountsUnicodeCharacters)
{
    // ASCII
    EXPECT_EQ(search_utils::utf8_char_count("hello"), 5);
    EXPECT_EQ(search_utils::utf8_char_count(""), 0);
    // Кириллица: "привет" = 6 символов, 12 байт
    EXPECT_EQ(search_utils::utf8_char_count("\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"), 6);
    // Смешанный: "hi мир" = 6 символов (h, i, пробел, м, и, р)
    EXPECT_EQ(search_utils::utf8_char_count("hi \xD0\xBC\xD0\xB8\xD1\x80"), 6);
}

// ============================================================
//  НОВЫЕ ТЕСТЫ — UTF-8 валидация и конвертация
// ============================================================

// Тест 9: is_valid_utf8 — валидные последовательности
TEST(Utf8Validation, ValidSequences)
{
    // Чистый ASCII
    EXPECT_TRUE(search_utils::is_valid_utf8("Hello, world!"));
    // Пустая строка
    EXPECT_TRUE(search_utils::is_valid_utf8(""));
    // Кириллица (правильный UTF-8)
    EXPECT_TRUE(search_utils::is_valid_utf8("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"));
    // Emoji (4-байтовый UTF-8)
    EXPECT_TRUE(search_utils::is_valid_utf8("\xF0\x9F\x98\x80"));
    // Смешанный ASCII + кириллица
    EXPECT_TRUE(search_utils::is_valid_utf8("test \xD1\x82\xD0\xB5\xD1\x81\xD1\x82"));
}

// Тест 10: is_valid_utf8 — невалидные последовательности
TEST(Utf8Validation, InvalidSequences)
{
    // Одиночный continuation byte
    EXPECT_FALSE(search_utils::is_valid_utf8("\x80"));
    // Обрезанная 2-байтовая последовательность
    EXPECT_FALSE(search_utils::is_valid_utf8("\xC0"));
    // Обрезанная 3-байтовая последовательность
    EXPECT_FALSE(search_utils::is_valid_utf8("\xE0\x80"));
    // Невалидный байт 0xFF
    EXPECT_FALSE(search_utils::is_valid_utf8("\xFF\xFE"));
    // Windows-1251 "Привет" — невалидный UTF-8
    EXPECT_FALSE(search_utils::is_valid_utf8("\xCF\xF0\xE8\xE2\xE5\xF2"));
}

// Тест 11: convert_to_utf8 — уже UTF-8 текст возвращается как есть
TEST(Utf8Convert, PassthroughValidUtf8)
{
    std::string utf8_text = "Hello, world!";
    EXPECT_EQ(search_utils::convert_to_utf8(utf8_text), utf8_text);

    std::string utf8_cyrillic = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
    EXPECT_EQ(search_utils::convert_to_utf8(utf8_cyrillic), utf8_cyrillic);
}

// Тест 12: convert_to_utf8 — Windows-1251 конвертируется в UTF-8
TEST(Utf8Convert, ConvertsWindows1251)
{
    // "Тест" в Windows-1251: 0xD2 0xE5 0xF1 0xF2
    std::string win1251 = "\xD2\xE5\xF1\xF2";
    std::string result = search_utils::convert_to_utf8(win1251);
    // Результат должен быть валидным UTF-8
    EXPECT_TRUE(search_utils::is_valid_utf8(result));
    // И не совпадать с входом (т.к. вход невалидный UTF-8)
    EXPECT_NE(result, win1251);
}

// ============================================================
//  НОВЫЕ ТЕСТЫ — read_file_as_utf8
// ============================================================

// Вспомогательный класс для создания временных файлов
class TempFileTest : public ::testing::Test
{
protected:
    std::string temp_dir_;

    void SetUp() override
    {
        temp_dir_ = std::filesystem::temp_directory_path().string() + "/search_engine_tests";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(temp_dir_);
    }

    std::string create_file(const std::string& name, const std::string& content)
    {
        std::string path = temp_dir_ + "/" + name;
        std::ofstream f(path, std::ios::binary);
        f.write(content.data(), content.size());
        f.close();
        return path;
    }
};

// Тест 13: read_file_as_utf8 — чтение ASCII файла
TEST_F(TempFileTest, ReadAsciiFile)
{
    auto path = create_file("ascii.txt", "Hello world");
    auto content = search_utils::read_file_as_utf8(path);
    EXPECT_EQ(content, "Hello world");
}

// Тест 14: read_file_as_utf8 — чтение UTF-8 файла
TEST_F(TempFileTest, ReadUtf8File)
{
    std::string utf8_text = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xD0\xBC\xD0\xB8\xD1\x80";
    auto path = create_file("utf8.txt", utf8_text);
    auto content = search_utils::read_file_as_utf8(path);
    EXPECT_EQ(content, utf8_text);
    EXPECT_TRUE(search_utils::is_valid_utf8(content));
}

// Тест 15: read_file_as_utf8 — чтение Windows-1251 файла
TEST_F(TempFileTest, ReadWin1251File)
{
    // "Тест" в Windows-1251
    std::string win1251_text = "\xD2\xE5\xF1\xF2";
    auto path = create_file("win1251.txt", win1251_text);
    auto content = search_utils::read_file_as_utf8(path);
    EXPECT_TRUE(search_utils::is_valid_utf8(content));
    EXPECT_FALSE(content.empty());
}

// Тест 16: read_file_as_utf8 — пустой файл
TEST_F(TempFileTest, ReadEmptyFile)
{
    auto path = create_file("empty.txt", "");
    auto content = search_utils::read_file_as_utf8(path);
    EXPECT_TRUE(content.empty());
}

// Тест 17: read_file_as_utf8 — несуществующий файл
TEST_F(TempFileTest, ReadNonexistentFile)
{
    auto content = search_utils::read_file_as_utf8("/nonexistent/path/file.txt");
    EXPECT_TRUE(content.empty());
}

// ============================================================
//  НОВЫЕ ТЕСТЫ — INI-файл парсинг с реальным файлом
// ============================================================

// Тест 18: ini_file — парсинг реального конфига
TEST_F(TempFileTest, IniFileParsing)
{
    std::string ini_content =
        "[DataBase]\n"
        "bd_host = localhost\n"
        "bd_port = 5432\n"
        "\n"
        "[Server]\n"
        "server_port = 8080\n";

    auto path = create_file("test.ini", ini_content);
    ini_file ini(path);

    EXPECT_EQ(ini.get_value("DataBase.bd_host"), "localhost");
    EXPECT_EQ(ini.get_value("DataBase.bd_port"), "5432");
    EXPECT_EQ(ini.get_value("Server.server_port"), "8080");
    EXPECT_EQ(ini.get_value("Unknown.key"), "");
}

// Тест 19: ini_file — значения с пробелами сохраняются (FIX #6)
TEST_F(TempFileTest, IniFilePreservesInternalSpaces)
{
    std::string ini_content =
        "[Client]\n"
        "documents_path = D:\\Test Documents\\files\n"
        "name = John Doe\n";

    auto path = create_file("spaces.ini", ini_content);
    ini_file ini(path);

    // Внутренние пробелы должны сохраняться
    EXPECT_EQ(ini.get_value("Client.documents_path"), "D:\\Test Documents\\files");
    EXPECT_EQ(ini.get_value("Client.name"), "John Doe");
}

// Тест 20: ini_file — комментарии игнорируются
TEST_F(TempFileTest, IniFileIgnoresComments)
{
    std::string ini_content =
        "; This is a comment\n"
        "# This is also a comment\n"
        "[Section]\n"
        "key = value\n";

    auto path = create_file("comments.ini", ini_content);
    ini_file ini(path);

    EXPECT_EQ(ini.get_value("Section.key"), "value");
}

// ============================================================
//  НОВЫЕ ТЕСТЫ — count_words с кириллицей
// ============================================================

// Тест 21: count_words — кириллица корректно обрабатывается
TEST(CountWords, HandlesCyrillicText)
{
    boost::locale::generator gen;
    std::locale::global(gen(""));

    std::set<std::string> stops = { "и", "в", "на", "не" };

    // "Программирование и разработка программ" →
    // "программирование"(1), "разработка"(1), "программ"(1)
    // "и" — стоп-слово, фильтруется
    auto words = search_utils::count_words(
        "\xD0\x9F\xD1\x80\xD0\xBE\xD0\xB3\xD1\x80\xD0\xB0\xD0\xBC\xD0\xBC\xD0\xB8\xD1\x80\xD0\xBE\xD0\xB2\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5"  // Программирование
        " \xD0\xB8"  // и
        " \xD1\x80\xD0\xB0\xD0\xB7\xD1\x80\xD0\xB0\xD0\xB1\xD0\xBE\xD1\x82\xD0\xBA\xD0\xB0"  // разработка
        " \xD0\xBF\xD1\x80\xD0\xBE\xD0\xB3\xD1\x80\xD0\xB0\xD0\xBC\xD0\xBC",  // программ
        stops);

    // "и" — стоп-слово, отфильтровано
    EXPECT_EQ(words.count("\xD0\xB8"), 0u);
    // Все остальные слова должны быть найдены (>2 символов Unicode)
    EXPECT_GE(words.size(), 2u);
}

// Тест 22: count_words — пустая строка
TEST(CountWords, EmptyText)
{
    std::set<std::string> stops;
    auto words = search_utils::count_words("", stops);
    EXPECT_TRUE(words.empty());
}

// Тест 23: count_words — только стоп-слова и короткие слова
TEST(CountWords, OnlyStopWordsAndShort)
{
    boost::locale::generator gen;
    std::locale::global(gen(""));

    std::set<std::string> stops = { "the", "and", "for" };
    auto words = search_utils::count_words("the and for I am a", stops);
    EXPECT_TRUE(words.empty());
}

// ============================================================
//  НОВЫЕ ТЕСТЫ — html_escape граничные случаи
// ============================================================

// Тест 24: html_escape — все спецсимволы вместе
TEST(HtmlEscape, AllSpecialCharsTogether)
{
    EXPECT_EQ(search_utils::html_escape("<>&\"'"),
        "&lt;&gt;&amp;&quot;&#39;");
}

// Тест 25: html_escape — кириллический текст не изменяется
TEST(HtmlEscape, CyrillicPassthrough)
{
    std::string cyrillic = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
    EXPECT_EQ(search_utils::html_escape(cyrillic), cyrillic);
}

// ============================================================
//  НОВЫЕ ТЕСТЫ — url_decode граничные случаи
// ============================================================

// Тест 26: url_decode — процент в конце строки
TEST(UrlDecode, TrailingPercent)
{
    // "%ZZ" — невалидный hex, должен остаться как есть
    auto result = search_utils::url_decode("test%");
    EXPECT_FALSE(result.empty());
}

// Тест 27: url_decode — пустой ввод
TEST(UrlDecode, EmptyInput)
{
    EXPECT_EQ(search_utils::url_decode(""), "");
}
