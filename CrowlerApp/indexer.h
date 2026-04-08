#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <memory>

#include "../data_base/data_base.h"

class indexer
{
public:
    indexer(std::shared_ptr<data_base> db);

    void index_directory(const std::string& path,
        const std::vector<std::string>& extensions,
        bool smart_mode = true);
    void index_file(const std::filesystem::path& file_path);

private:
    std::shared_ptr<data_base> db_;
    std::set<std::string> stop_words_;
};
