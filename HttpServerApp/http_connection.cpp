#include "http_connection.h"
#include "../utils/utils.h"
#include <iostream>
#include <cmath>

// ============================================================
//  CSS / NAV / FOOTER
// ============================================================
static const char* CSS = R"(
<style>
  body{font-family:Arial,sans-serif;margin:40px auto;max-width:820px;background:#f5f5f5}
  h1{color:#333} h2{color:#555}
  .card{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1);margin-bottom:16px}
  input[type=text]{width:60%;padding:10px;font-size:16px;border:1px solid #ccc;border-radius:4px}
  button{padding:10px 24px;font-size:16px;background:#4285f4;color:#fff;border:none;border-radius:4px;cursor:pointer}
  button:hover{background:#3367d6}
  a{color:#1a0dab;text-decoration:none} a:hover{text-decoration:underline}
  .snippet{color:#545454;font-size:14px;margin:4px 0 0 0}
  .score{color:#006621;font-size:13px}
  .meta{color:#888;font-size:13px}
  mark{background:#fff3a8;padding:1px 3px;border-radius:2px}
  nav a{margin-right:16px}
  .stat-num{font-size:32px;font-weight:bold;color:#4285f4}
  table{width:100%;border-collapse:collapse}
  th{text-align:left;padding:8px;border-bottom:2px solid #ccc}
  td{padding:8px;border-bottom:1px solid #eee}
  .pagination{text-align:center;margin:20px 0}
  .pagination a,.pagination span{display:inline-block;padding:8px 16px;margin:0 4px;
    border-radius:4px;border:1px solid #ccc;text-decoration:none;color:#333}
  .pagination span{background:#4285f4;color:#fff;border-color:#4285f4}
  .error-card{background:#fff;padding:20px;border-radius:8px;border-left:4px solid #dc3545;
    box-shadow:0 2px 4px rgba(0,0,0,.1);margin-bottom:16px}
  .checkbox-label{font-size:14px;color:#555;margin-left:8px;cursor:pointer}
  input[type=checkbox]{cursor:pointer}
</style>
)";

static const char* NAV = R"(
<nav class="card">
  <a href="/">Search</a>
  <a href="/stats">Statistics</a>
  <a href="/history">History</a>
</nav>
)";

static const char* FOOTER = R"(
<p class="meta" style="text-align:center;margin-top:30px">
  File System Search Engine &mdash; Diploma Project</p>
)";

// ============================================================
//  Constructor / start
// ============================================================
HttpConnection::HttpConnection(tcp::socket socket,
    std::shared_ptr<data_base> db, int results_per_page)
    : socket_(std::move(socket))
    , deadline_(socket_.get_executor(), std::chrono::seconds(60))
    , db_(std::move(db))
    , results_per_page_(results_per_page)
{
}

void HttpConnection::start()
{
    read_request();
    check_deadline();
}

// ============================================================
//  Networking
// ============================================================
void HttpConnection::read_request()
{
    auto self = shared_from_this();
    http::async_read(socket_, buffer_, request_,
        [self](beast::error_code ec, std::size_t) {
            if (!ec) self->process_request();
        });
}

void HttpConnection::process_request()
{
    response_.version(request_.version());
    response_.keep_alive(false);
    response_.set(http::field::content_type, "text/html; charset=utf-8");

    switch (request_.method())
    {
    case http::verb::get:  create_response_get();  break;
    case http::verb::post: create_response_post(); break;
    default:
        response_.result(http::status::bad_request);
        beast::ostream(response_.body()) << page_bad_request();
        break;
    }
    response_.prepare_payload();
    write_response();
}

void HttpConnection::write_response()
{
    auto self = shared_from_this();
    http::async_write(socket_, response_,
        [self](beast::error_code ec, std::size_t) {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            self->deadline_.cancel();
        });
}

void HttpConnection::check_deadline()
{
    auto self = shared_from_this();
    deadline_.async_wait([self](beast::error_code ec) {
        if (!ec) self->socket_.close(ec);
        });
}

// ============================================================
//  GET routing
// ============================================================
void HttpConnection::create_response_get()
{
    std::string target(request_.target());
    response_.result(http::status::ok);

    if (target == "/" || target.empty())
        beast::ostream(response_.body()) << page_search_form();
    else if (target == "/stats")
        beast::ostream(response_.body()) << page_stats();
    else if (target == "/history")
        beast::ostream(response_.body()) << page_history();
    else if (target.rfind("/search?", 0) == 0)
    {
        std::string q;
        auto qpos = target.find("q=");
        if (qpos != std::string::npos) {
            auto end = target.find('&', qpos);
            q = search_utils::url_decode(target.substr(qpos + 2,
                end == std::string::npos ? end : end - qpos - 2));
        }
        bool exact = (target.find("exact=on") != std::string::npos);
        int page = 1;
        auto ppos = target.find("page=");
        if (ppos != std::string::npos) {
            try { page = std::stoi(target.substr(ppos + 5)); }
            catch (...) {}
        }
        if (q.empty()) {
            response_.result(http::status::bad_request);
            beast::ostream(response_.body()) << page_bad_request();
        }
        else {
            beast::ostream(response_.body()) << page_search_results(q, exact, page);
        }
    }
    else
    {
        response_.result(http::status::not_found);
        beast::ostream(response_.body()) << page_not_found();
    }
}

// ============================================================
//  POST routing
// ============================================================
void HttpConnection::create_response_post()
{
    std::string target(request_.target());
    if (target != "/" && target != "/search")
    {
        response_.result(http::status::bad_request);
        beast::ostream(response_.body()) << page_bad_request();
        return;
    }

    std::string body_str = beast::buffers_to_string(request_.body().data());

    std::string query;
    auto pos = body_str.find("search=");
    if (pos != std::string::npos)
    {
        auto end = body_str.find('&', pos);
        query = search_utils::url_decode(body_str.substr(pos + 7,
            end == std::string::npos ? end : end - pos - 7));
    }

    bool exact = (body_str.find("exact=on") != std::string::npos);

    if (query.empty())
    {
        response_.result(http::status::bad_request);
        beast::ostream(response_.body()) << page_bad_request();
        return;
    }

    response_.result(http::status::ok);
    beast::ostream(response_.body()) << page_search_results(query, exact, 1);
}

// ============================================================
//  Pages
// ============================================================
std::string HttpConnection::page_search_form()
{
    std::ostringstream s;
    s << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Search</title>"
        << CSS << "</head><body>" << NAV
        << "<div class='card'><h1>Search Engine</h1>"
        "<form method='POST' action='/'>"
        "<input type='text' name='search' placeholder='Enter search query...' autofocus>"
        " <button type='submit'>Search</button><br><br>"
        "<label class='checkbox-label'>"
        "<input type='checkbox' name='exact'> Exact phrase match"
        "</label>"
        "</form>"
        "<p class='meta'>Ranking: TF-IDF. Check \"Exact phrase\" to find words in exact order.</p>"
        "</div>"
        << FOOTER << "</body></html>";
    return s.str();
}

std::string HttpConnection::page_not_found()
{
    std::ostringstream s;
    s << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>404</title>"
        << CSS << "</head><body>" << NAV
        << "<div class='card'><h1>404 &mdash; Not Found</h1></div>"
        << FOOTER << "</body></html>";
    return s.str();
}

std::string HttpConnection::page_bad_request()
{
    std::ostringstream s;
    s << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Bad Request</title>"
        << CSS << "</head><body>" << NAV
        << "<div class='card'><h1>Bad Request</h1>"
        "<p>Please enter a valid query (at least one word with 3+ characters).</p></div>"
        << FOOTER << "</body></html>";
    return s.str();
}

std::string HttpConnection::page_error(const std::string& title, const std::string& message)
{
    std::ostringstream s;
    s << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Error</title>"
        << CSS << "</head><body>" << NAV
        << "<div class='error-card'><h1>" << search_utils::html_escape(title) << "</h1>"
        << "<p>" << search_utils::html_escape(message) << "</p>"
        << "<p class='meta'>Check that PostgreSQL is running and database 'diplom' exists.</p>"
        << "</div>" << FOOTER << "</body></html>";
    return s.str();
}

std::string HttpConnection::page_stats()
{
    int files = 0, kw = 0, links = 0;
    try {
        files = db_->get_total_files();
        kw = db_->get_total_keywords();
        links = db_->get_total_relations();
    }
    catch (const std::exception& e) {
        return page_error("Database Error", e.what());
    }

    std::ostringstream s;
    s << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Stats</title>"
        << CSS << "</head><body>" << NAV
        << "<div class='card'><h1>Index Statistics</h1>"
        << "<p><span class='stat-num'>" << files << "</span> Indexed Files</p>"
        << "<p><span class='stat-num'>" << kw << "</span> Unique Keywords</p>"
        << "<p><span class='stat-num'>" << links << "</span> Word-Document Links</p>"
        << "<p class='meta'>Ranking algorithm: TF-IDF "
        "(Term Frequency &times; Inverse Document Frequency)</p>"
        << "</div>" << FOOTER << "</body></html>";
    return s.str();
}

std::string HttpConnection::page_history()
{
    std::ostringstream s;
    s << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>History</title>"
        << CSS << "</head><body>" << NAV
        << "<div class='card'><h1>Search History</h1>";

    try {
        auto rows = db_->get_search_history(50);
        if (rows.empty()) {
            s << "<p>No searches yet.</p>";
        }
        else {
            s << "<table>"
                << "<tr><th>Date</th><th>Query</th><th>Results</th><th>Time</th></tr>";
            for (const auto& [dt, query, cnt, elapsed] : rows) {
                s << "<tr>"
                    << "<td>" << search_utils::html_escape(dt) << "</td>"
                    << "<td><a href='/search?q=" << search_utils::html_escape(query) << "'>"
                    << search_utils::html_escape(query) << "</a></td>"
                    << "<td style='text-align:center'>" << cnt << "</td>"
                    << "<td style='text-align:center'>"
                    << std::fixed << std::setprecision(4) << elapsed << " s</td>"
                    << "</tr>";
            }
            s << "</table>";
        }
    }
    catch (const std::exception& e) {
        return page_error("Database Error", e.what());
    }

    s << "</div>" << FOOTER << "</body></html>";
    return s.str();
}

// ============================================================
//  Фильтрация по точному вхождению фразы
//
//  Логика перенесена из data_base (разделение ответственности):
//  data_base отвечает только за SQL, файловый I/O — здесь.
//
//  1. Получаем TF-IDF кандидатов из БД
//  2. Для каждого кандидата читаем файл через search_utils::read_file_as_utf8
//  3. Приводим текст к нижнему регистру через boost::locale::to_lower
//  4. Ищем точное вхождение фразы
//  5. Отбрасываем документы без совпадения
// ============================================================
std::vector<data_base::tfidf_result> HttpConnection::filter_exact_phrase(
    const std::vector<data_base::tfidf_result>& candidates,
    const std::string& lower_phrase)
{
    std::vector<data_base::tfidf_result> results;
    for (const auto& candidate : candidates)
    {
        std::string content = search_utils::read_file_as_utf8(candidate.url);
        if (content.empty()) continue;

        std::string lower_content = boost::locale::to_lower(content);

        if (lower_content.find(lower_phrase) != std::string::npos)
            results.push_back(candidate);
    }
    return results;
}

std::string HttpConnection::page_search_results(const std::string& query, bool exact, int page)
{
    auto t_start = std::chrono::steady_clock::now();

    auto keywords = search_utils::get_keywords(query);
    if (keywords.empty())
        return page_bad_request();

    std::vector<data_base::tfidf_result> results;

    try {
        // Всегда получаем кандидатов через TF-IDF из БД
        results = db_->search_tfidf(keywords);

        // Фильтрация по точной фразе — файловый I/O на уровне представления
        if (exact)
        {
            std::string lower_query = boost::locale::to_lower(query);
            results = filter_exact_phrase(results, lower_query);
        }
    }
    catch (const std::exception& e) {
        return page_error("Search Error", e.what());
    }

    auto t_end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();
    int total_count = static_cast<int>(results.size());

    try { db_->add_search_query(query, total_count, elapsed); }
    catch (...) {}

    if (page < 1) page = 1;
    int total_pages = (total_count + results_per_page_ - 1) / results_per_page_;
    if (total_pages < 1) total_pages = 1;
    if (page > total_pages) page = total_pages;
    int start_idx = (page - 1) * results_per_page_;
    int end_idx = std::min(start_idx + results_per_page_, total_count);

    std::ostringstream s;
    s << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Results</title>"
        << CSS << "</head><body>" << NAV;

    s << "<div class='card'>"
        "<form method='POST' action='/'>"
        "<input type='text' name='search' value='" << search_utils::html_escape(query) << "'>"
        " <button type='submit'>Search</button><br><br>"
        "<label class='checkbox-label'>"
        "<input type='checkbox' name='exact'" << (exact ? " checked" : "") << "> Exact phrase match"
        "</label>"
        "</form></div>";

    s << "<div class='card'><p>Found <b>" << total_count << "</b> result(s) for \""
        << search_utils::html_escape(query) << "\" in "
        << std::fixed << std::setprecision(4) << elapsed << " sec"
        << " &nbsp; <span class='meta'>("
        << (exact ? "exact phrase" : "TF-IDF ranking")
        << ")</span></p></div>";

    if (results.empty())
    {
        s << "<div class='card'><p>No matches found.";
        if (exact)
            s << " Try unchecking \"Exact phrase match\" for broader results.";
        s << "</p></div>";
    }
    else
    {
        for (int i = start_idx; i < end_idx; ++i)
        {
            const auto& r = results[i];
            std::string snippet = make_snippet(r.url, keywords);
            std::string highlighted = highlight_snippet(snippet, keywords);

            s << "<div class='card'>"
                << "<p><b>#" << (i + 1) << "</b> "
                << "<a href='file:///" << search_utils::html_escape(r.url) << "'>"
                << search_utils::html_escape(r.url) << "</a></p>"
                << "<p class='snippet'>" << highlighted << "</p>"
                << "<p class='score'>TF-IDF score: "
                << std::fixed << std::setprecision(4) << r.score << "</p>"
                << "</div>";
        }

        if (total_pages > 1)
        {
            std::string eq = search_utils::html_escape(query);
            std::string exact_param = exact ? "&exact=on" : "";
            s << "<div class='pagination'>";
            if (page > 1)
                s << "<a href='/search?q=" << eq << exact_param
                << "&page=" << (page - 1) << "'>&laquo; Prev</a>";
            for (int p = 1; p <= total_pages; ++p)
            {
                if (p == page)
                    s << "<span>" << p << "</span>";
                else
                    s << "<a href='/search?q=" << eq << exact_param
                    << "&page=" << p << "'>" << p << "</a>";
            }
            if (page < total_pages)
                s << "<a href='/search?q=" << eq << exact_param
                << "&page=" << (page + 1) << "'>Next &raquo;</a>";
            s << "</div>";
        }
    }

    s << FOOTER << "</body></html>";
    return s.str();
}

// ============================================================
//  Snippet & highlight
// ============================================================
std::string HttpConnection::make_snippet(const std::string& filepath,
    const std::vector<std::string>& kw, std::size_t ctx)
{
    std::string text = search_utils::read_file_as_utf8(filepath);
    if (text.empty()) return "(file not accessible)";

    std::string lower_text = boost::locale::to_lower(text);
    std::size_t best_pos = std::string::npos;
    for (const auto& k : kw)
    {
        auto p = lower_text.find(k);
        if (p < best_pos) best_pos = p;
    }
    if (best_pos == std::string::npos) best_pos = 0;

    std::size_t start = (best_pos > ctx) ? best_pos - ctx : 0;
    std::size_t len = ctx * 2 + 40;
    std::string snippet = text.substr(start, len);

    for (auto& c : snippet)
        if (c == '\n' || c == '\r') c = ' ';

    if (start > 0) snippet = "..." + snippet;
    if (start + len < text.size()) snippet += "...";

    return snippet;
}

std::string HttpConnection::highlight_snippet(const std::string& text,
    const std::vector<std::string>& kw)
{
    if (kw.empty()) return search_utils::html_escape(text);

    std::string lower_text = boost::locale::to_lower(text);

    struct match { std::size_t pos; std::size_t len; };
    std::vector<match> matches;

    for (const auto& k : kw)
    {
        std::size_t pos = 0;
        while ((pos = lower_text.find(k, pos)) != std::string::npos)
        {
            matches.push_back({ pos, k.size() });
            pos += k.size();
        }
    }

    std::sort(matches.begin(), matches.end(),
        [](const match& a, const match& b) { return a.pos < b.pos; });

    std::string result;
    result.reserve(text.size() + matches.size() * 13);
    std::size_t prev = 0;

    for (const auto& m : matches)
    {
        if (m.pos < prev) continue;
        result += search_utils::html_escape(text.substr(prev, m.pos - prev));
        result += "<mark>" + search_utils::html_escape(text.substr(m.pos, m.len)) + "</mark>";
        prev = m.pos + m.len;
    }
    result += search_utils::html_escape(text.substr(prev));

    return result;
}
