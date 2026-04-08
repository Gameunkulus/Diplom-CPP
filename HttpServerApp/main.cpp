#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <vector>

#include <boost/locale.hpp>

#include "http_connection.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include "../ini_file/ini_file.h"
#include "../data_base/data_base.h"

// Глобальный указатель на io_context для graceful shutdown
static net::io_context* g_ioc = nullptr;

void signal_handler(int signum)
{
    std::cout << "\nShutting down server (signal " << signum << ")..." << std::endl;
    if (g_ioc) g_ioc->stop();
}

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket,
    std::shared_ptr<data_base> db, int results_per_page)
{
    acceptor.async_accept(socket,
        [&acceptor, &socket, db, results_per_page](beast::error_code ec)
        {
            if (!ec)
                std::make_shared<HttpConnection>(std::move(socket), db, results_per_page)->start();
            httpServer(acceptor, socket, db, results_per_page);
        });
}

int main()
{
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    boost::locale::generator gen;
    std::locale::global(gen(""));

    // Регистрируем обработчик сигналов для корректного завершения
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try
    {
        ini_file ini("config.ini");

        // Количество потоков = количество ядер (минимум 2)
        unsigned int num_threads = std::max(2u, std::thread::hardware_concurrency());

        auto db = std::make_shared<data_base>(
            ini.get_value("DataBase.bd_host"),
            ini.get_value("DataBase.bd_port"),
            ini.get_value("DataBase.bd_name"),
            ini.get_value("DataBase.bd_user"),
            ini.get_value("DataBase.bd_pass"),
            num_threads);  // размер пула соединений = количеству потоков

        unsigned short port = static_cast<unsigned short>(
            std::stoi(ini.get_value("Server.server_port")));

        int results_per_page = 10;
        std::string rpp = ini.get_value("Server.results_per_page");
        if (!rpp.empty()) results_per_page = std::stoi(rpp);

        auto const address = net::ip::make_address("0.0.0.0");
        net::io_context ioc{ static_cast<int>(num_threads) };
        g_ioc = &ioc;

        tcp::acceptor acceptor{ ioc, {address, port} };
        tcp::socket socket{ ioc };

        httpServer(acceptor, socket, db, results_per_page);

        std::cout << "========================================" << std::endl;
        std::cout << "  Search Engine Server v2.0" << std::endl;
        std::cout << "  http://localhost:" << port << std::endl;
        std::cout << "  Threads:  " << num_threads << std::endl;
        std::cout << "  DB pool:  " << num_threads << " connections" << std::endl;
        std::cout << "  /stats    - statistics" << std::endl;
        std::cout << "  /history  - search history" << std::endl;
        std::cout << "  Press Ctrl+C to stop" << std::endl;
        std::cout << "========================================" << std::endl;

        // Запускаем worker-потоки для io_context
        std::vector<std::thread> threads;
        threads.reserve(num_threads - 1);
        for (unsigned int i = 1; i < num_threads; ++i)
        {
            threads.emplace_back([&ioc]() { ioc.run(); });
        }

        // Главный поток тоже обрабатывает события
        ioc.run();

        // Ждём завершения всех worker-потоков
        for (auto& t : threads)
        {
            if (t.joinable()) t.join();
        }

        std::cout << "Server stopped." << std::endl;
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    g_ioc = nullptr;
    return 0;
}
