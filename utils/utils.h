#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

namespace search_utils
{
    // HTML экранирование спецсимволов
    std::string html_escape(const std::string& s);

    // Декодирование URL-encoded строки (%XX, +)
    std::string url_decode(const std::string& s);

    // Разбиение запроса на ключевые слова:
    // lowercase через Boost.Locale, фильтрация <= 2 символов, дедупликация
    std::vector<std::string> get_keywords(const std::string& raw);

    // Подсчёт UTF-8 символов (не байтов) в строке
    size_t utf8_char_count(const std::string& s);

    // Подсчёт частоты слов в тексте:
    // lowercase, фильтрация <= 2 символов (Unicode), фильтрация стоп-слов
    std::map<std::string, unsigned int> count_words(const std::string& text,
        const std::set<std::string>& stop_words);

    // Проверка является ли слово стоп-словом
    bool is_stop_word(const std::string& word, const std::set<std::string>& stop_words);

    // Проверка валидности UTF-8 последовательности
    bool is_valid_utf8(const std::string& text);

    // Конвертация текста в UTF-8 (автоопределение: UTF-8 / Windows-1251 / Latin1)
    std::string convert_to_utf8(const std::string& text);

    // Чтение файла в строку с автоконвертацией в UTF-8
    std::string read_file_as_utf8(const std::string& filepath);
}
