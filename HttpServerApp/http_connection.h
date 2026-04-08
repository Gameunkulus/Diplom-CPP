#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/locale.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#include "../data_base/data_base.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
    HttpConnection(tcp::socket socket, std::shared_ptr<data_base> db, int results_per_page);
    void start();

private:
    tcp::socket                          socket_;
    beast::flat_buffer                   buffer_{ 8192 };
    http::request<http::dynamic_body>    request_;
    http::response<http::dynamic_body>   response_;
    net::steady_timer                    deadline_;
    std::shared_ptr<data_base>           db_;
    int                                  results_per_page_;

    void read_request();
    void process_request();
    void create_response_get();
    void create_response_post();
    void write_response();
    void check_deadline();

    std::string page_search_form();
    std::string page_not_found();
    std::string page_bad_request();
    std::string page_stats();
    std::string page_history();
    std::string page_search_results(const std::string& query, bool exact, int page = 1);
    std::string page_error(const std::string& title, const std::string& message);

    // Фильтрация результатов по точному вхождению фразы
    // (файловый I/O вынесен из data_base сюда — слой представления)
    static std::vector<data_base::tfidf_result> filter_exact_phrase(
        const std::vector<data_base::tfidf_result>& candidates,
        const std::string& lower_phrase);

    static std::string make_snippet(const std::string& filepath,
        const std::vector<std::string>& kw,
        std::size_t ctx = 120);
    static std::string highlight_snippet(const std::string& text,
        const std::vector<std::string>& kw);
};
