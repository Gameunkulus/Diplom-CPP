#include <iostream>
#include <sstream>
#include <iomanip>

#include "../ini_file/ini_file.h"
#include "../data_base/data_base.h"
#include "indexer.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

int main()
{
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    setlocale(LC_ALL, "ru_RU.utf-8");

    std::string db_name;
    try
    {
        ini_file ini("config.ini");

        std::string documents_path = ini.get_value("Client.documents_path");
        std::string extensions_str = ini.get_value("Client.file_extensions");
        std::string index_mode = ini.get_value("Client.index_mode");
        bool smart_mode = (index_mode == "smart");

        std::vector<std::string> extensions;
        std::stringstream ss(extensions_str);
        std::string ext;
        while (std::getline(ss, ext, ','))
        {
            ext.erase(std::remove_if(ext.begin(), ext.end(), ::isspace), ext.end());
            if (!ext.empty()) extensions.push_back(ext);
        }

        db_name = ini.get_value("DataBase.bd_name");
        auto db = std::make_shared<data_base>(
            ini.get_value("DataBase.bd_host"),
            ini.get_value("DataBase.bd_port"),
            db_name,
            ini.get_value("DataBase.bd_user"),
            ini.get_value("DataBase.bd_pass"));

        std::cout << "========================================" << std::endl;
        std::cout << "  File System Indexer v2.0" << std::endl;
        std::cout << "  Directory:  " << documents_path << std::endl;
        std::cout << "  Extensions:";
        for (const auto& e : extensions) std::cout << " " << e;
        std::cout << std::endl;
        std::cout << "  Mode:       " << (smart_mode ? "smart" : "full") << std::endl;
        std::cout << "  Database:   " << db_name << std::endl;
        std::cout << "========================================\n" << std::endl;

        indexer ind(db);
        ind.index_directory(documents_path, extensions, smart_mode);
    }
    catch (const pqxx::broken_connection& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << "Possible reasons:" << std::endl
            << "  - connection unavailable" << std::endl
            << "  - database '" << db_name << "' does not exist" << std::endl
            << "  - permission denied" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
