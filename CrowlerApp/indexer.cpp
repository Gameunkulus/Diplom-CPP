#include "indexer.h"
#include "../utils/utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <boost/locale.hpp>

// ============================================================

indexer::indexer(std::shared_ptr<data_base> db) : db_(db)
{
    boost::locale::generator gen;
    std::locale loc = gen("");
    std::locale::global(loc);
    std::cout.imbue(loc);

    stop_words_ = {
        "able", "about", "above", "after", "again", "against", "also",
        "been", "before", "being", "below", "between", "both",
        "could", "does", "doing", "down", "during", "each",
        "even", "every", "find", "first", "from", "further",
        "have", "having", "here", "hers", "herself", "himself",
        "into", "itself", "just", "more", "most", "myself",
        "once", "only", "other", "over", "same",
        "should", "some", "such", "than", "that",
        "their", "theirs", "them", "themselves", "then", "there",
        "these", "they", "this", "those", "through", "under",
        "until", "very", "were", "what", "when", "where",
        "which", "while", "whom", "will", "with", "your",
        "yours", "yourself", "yourselves",
        "the", "and", "for", "are", "but", "not", "you",
        "all", "can", "had", "her", "was", "one", "our",
        "out", "has", "his", "how", "its", "may", "new",
        "now", "old", "see", "way", "who", "did", "get",
        "got", "him", "let", "say", "she", "too", "use"
    };

    std::set<std::string> russian_stop = {
        "и", "в", "во", "не", "что", "он", "на", "я", "с", "со",
        "как", "а", "то", "все", "она", "так", "его", "но", "да", "ты",
        "к", "у", "же", "вы", "за", "бы", "по", "только", "её", "мне",
        "было", "вот", "от", "меня", "ещё", "нет", "о", "из", "ему",
        "теперь", "когда", "даже", "ну", "вдруг", "ли", "если", "уже",
        "или", "ни", "быть", "был", "него", "до", "вас", "нибудь",
        "опять", "уж", "вам", "ведь", "там", "потом", "себя", "ничего",
        "ей", "может", "они", "тут", "где", "есть", "надо", "ней",
        "для", "мы", "тебя", "их", "чем", "была", "сам", "чтоб",
        "без", "будто", "чего", "раз", "тоже", "себе", "под", "будет",
        "ж", "тогда", "кто", "этот", "того", "потому", "этого", "какой",
        "совсем", "ним", "здесь", "этом", "один", "почти", "мой",
        "тем", "чтобы", "нее", "сейчас", "были", "куда", "зачем",
        "всех", "никогда", "можно", "при", "наконец", "два", "об",
        "другой", "хоть", "после", "над", "больше", "тот", "через",
        "эти", "нас", "про", "всего", "них", "какая", "много",
        "разве", "три", "эту", "моя", "впрочем", "хорошо", "свою",
        "этой", "перед", "иногда", "лучше", "чуть", "том", "нельзя",
        "такой", "ним", "какие", "давно", "пока"
    };
    stop_words_.insert(russian_stop.begin(), russian_stop.end());
}

void indexer::index_directory(const std::string& path,
    const std::vector<std::string>& extensions, bool smart_mode)
{
    namespace fs = std::filesystem;

    if (!fs::exists(path) || !fs::is_directory(path))
    {
        std::cerr << "Error: directory does not exist: " << path << std::endl;
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    std::vector<fs::directory_entry> files;
    for (const auto& entry : fs::recursive_directory_iterator(path))
    {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        for (const auto& allowed : extensions)
        {
            if (ext == allowed) { files.push_back(entry); break; }
        }
    }

    int total_files = static_cast<int>(files.size());
    std::cout << "Found " << total_files << " files to process" << std::endl;
    std::cout << "Mode: " << (smart_mode ? "smart (only changed files)" : "full reindex")
        << "\n" << std::endl;

    int processed = 0, indexed = 0, skipped = 0, updated = 0;

    for (const auto& entry : files)
    {
        processed++;
        std::string file_path_str = entry.path().string();
        int percent = (total_files > 0) ? (processed * 100 / total_files) : 100;

        if (smart_mode)
        {
            auto ftime = fs::last_write_time(entry);
            auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::clock_cast<std::chrono::system_clock>(ftime));
            long long file_mod_time = sctp.time_since_epoch().count();
            long long db_mod_time = db_->get_url_modified_time(file_path_str);

            if (db_mod_time != 0 && db_mod_time >= file_mod_time)
            {
                std::cout << "[" << percent << "%] SKIP (unchanged) "
                    << entry.path().filename() << std::endl;
                skipped++;
                continue;
            }

            if (db_mod_time != 0)
            {
                std::cout << "[" << percent << "%] UPDATE "
                    << entry.path().filename() << std::endl;
                updated++;
            }
            else
            {
                std::cout << "[" << percent << "%] ";
            }
        }
        else
        {
            std::cout << "[" << percent << "%] ";
        }

        index_file(entry.path());
        indexed++;
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Indexing complete!" << std::endl;
    std::cout << "  Total files:    " << total_files << std::endl;
    std::cout << "  New indexed:    " << (indexed - updated) << std::endl;
    std::cout << "  Updated:        " << updated << std::endl;
    std::cout << "  Skipped:        " << skipped << std::endl;
    std::cout << "  Time:           " << std::fixed << std::setprecision(2)
        << elapsed << " sec" << std::endl;
    std::cout << "  Stop words:     " << stop_words_.size() << " (EN + RU)" << std::endl;
    std::cout << "  Encoding:       auto-detect (UTF-8 / Windows-1251)" << std::endl;
    std::cout << "========================================" << std::endl;
}

void indexer::index_file(const std::filesystem::path& file_path)
{
    namespace fs = std::filesystem;

    std::string content = search_utils::read_file_as_utf8(file_path.string());

    if (content.empty())
    {
        std::cout << "SKIP empty: " << file_path.filename() << std::endl;
        return;
    }

    // Используем общую функцию из utils
    auto words = search_utils::count_words(content, stop_words_);

    if (words.empty())
    {
        std::cout << "SKIP no words: " << file_path.filename() << std::endl;
        return;
    }

    auto ftime = fs::last_write_time(file_path);
    auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::clock_cast<std::chrono::system_clock>(ftime));
    long long mod_time = sctp.time_since_epoch().count();

    std::string path_str = file_path.string();

    try
    {
        db_->add_keyword_link(words, path_str, mod_time);
        std::cout << "OK " << file_path.filename()
            << " (" << words.size() << " unique words)" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "ERROR " << file_path.filename() << ": " << e.what() << std::endl;
    }
}
