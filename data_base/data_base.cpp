#include "data_base.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// ============================================================
//  Конструкторы
// ============================================================
data_base::data_base(const std::string& host, const std::string& port,
    const std::string& dbname, const std::string& user, const std::string& password,
    size_t pool_size)
    : data_base(std::string("host=" + host + " port=" + port + " dbname=" + dbname +
        " user=" + user + " password=" + password), pool_size)
{
}

data_base::data_base(const std::string& connection_string, size_t pool_size)
    : conn_string_(connection_string)
    , pool_(std::make_unique<connection_pool>(connection_string, pool_size))
{
    create_db_();
}

// ============================================================
//  add_keyword_link — добавление слов документа в индекс
// ============================================================
bool data_base::add_keyword_link(const std::map<std::string, unsigned int>& words,
    const std::string& link, long long modified_time)
{
    auto conn = pool_->get_connection();
    pqxx::work tx{ conn.get() };

    tx.exec(
        "INSERT INTO urls (url, modified_time) VALUES ($1, $2) "
        "ON CONFLICT (url) DO UPDATE SET modified_time = $2",
        pqxx::params{ link, modified_time });

    tx.exec(
        "DELETE FROM keyword_url_qty WHERE id_url = "
        "(SELECT id FROM urls WHERE url = $1)",
        pqxx::params{ link });

    for (const auto& [word, count] : words)
    {
        tx.exec(
            "INSERT INTO keywords (keyword) VALUES ($1) ON CONFLICT DO NOTHING",
            pqxx::params{ word });

        tx.exec(
            "INSERT INTO keyword_url_qty (id_keyword, id_url, quantity) VALUES ("
            "(SELECT id FROM keywords WHERE keyword = $1),"
            "(SELECT id FROM urls WHERE url = $2), $3) "
            "ON CONFLICT (id_keyword, id_url) DO UPDATE SET quantity = $3",
            pqxx::params{ word, link, static_cast<int>(count) });
    }

    tx.commit();
    return true;
}

bool data_base::check_url(std::string_view url)
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    return tx.query_value<bool>(
        "SELECT EXISTS(SELECT 1 FROM urls WHERE url = $1)",
        pqxx::params{ std::string(url) });
}

long long data_base::get_url_modified_time(std::string_view url)
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    try {
        return tx.query_value<long long>(
            "SELECT modified_time FROM urls WHERE url = $1",
            pqxx::params{ std::string(url) });
    }
    catch (...) { return 0; }
}

void data_base::remove_url(const std::string& url)
{
    auto conn = pool_->get_connection();
    pqxx::work tx{ conn.get() };
    tx.exec(
        "DELETE FROM keyword_url_qty WHERE id_url = "
        "(SELECT id FROM urls WHERE url = $1)",
        pqxx::params{ url });
    tx.exec("DELETE FROM urls WHERE url = $1",
        pqxx::params{ url });
    tx.commit();
}

int data_base::get_total_documents()
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    return tx.query_value<int>("SELECT COUNT(*) FROM urls");
}

std::map<std::string, int> data_base::get_urls_qry(std::string_view keyword)
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    std::map<std::string, int> result;
    for (auto const& [url, qty] : tx.query<std::string, int>(
        "SELECT u.url, kuq.quantity "
        "FROM urls u "
        "JOIN keyword_url_qty kuq ON u.id = kuq.id_url "
        "JOIN keywords k ON kuq.id_keyword = k.id "
        "WHERE k.keyword = $1",
        pqxx::params{ std::string(keyword) }))
    {
        result[url] = qty;
    }
    return result;
}

int data_base::get_document_frequency(std::string_view keyword)
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    try {
        return tx.query_value<int>(
            "SELECT COUNT(DISTINCT id_url) FROM keyword_url_qty "
            "WHERE id_keyword = (SELECT id FROM keywords WHERE keyword = $1)",
            pqxx::params{ std::string(keyword) });
    }
    catch (...) { return 0; }
}

// ============================================================
//  TF-IDF поиск (bag-of-words)
//
//  Алгоритм:
//  1. N = общее число документов
//  2. Для каждого документа total_words = SUM(quantity)
//  3. Для каждого ключевого слова q_i:
//     a) DF = COUNT(DISTINCT документов с q_i)
//     b) IDF = log(N / DF)
//     c) Для каждого документа d: TF = quantity(q_i, d) / total_words(d)
//     d) score(d) += TF × IDF
//  4. Сортировка по score DESC
// ============================================================
std::vector<data_base::tfidf_result> data_base::search_tfidf(
    const std::vector<std::string>& keywords)
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };

    int N = tx.query_value<int>("SELECT COUNT(*) FROM urls");
    if (N == 0) return {};

    std::map<int, std::pair<std::string, int>> docs;
    for (auto const& [id, url, total] : tx.query<int, std::string, int>(
        "SELECT u.id, u.url, COALESCE(SUM(kuq.quantity), 1) "
        "FROM urls u LEFT JOIN keyword_url_qty kuq ON u.id = kuq.id_url "
        "GROUP BY u.id, u.url"))
    {
        docs[id] = { url, total };
    }

    std::map<int, double> scores;

    for (const auto& kw : keywords)
    {
        int df = 0;
        try {
            df = tx.query_value<int>(
                "SELECT COUNT(DISTINCT id_url) FROM keyword_url_qty "
                "WHERE id_keyword = (SELECT id FROM keywords WHERE keyword = $1)",
                pqxx::params{ kw });
        }
        catch (...) { continue; }

        if (df == 0) continue;

        double idf = std::log(static_cast<double>(N) / static_cast<double>(df));

        for (auto const& [url_id, qty] : tx.query<int, int>(
            "SELECT id_url, quantity FROM keyword_url_qty "
            "WHERE id_keyword = (SELECT id FROM keywords WHERE keyword = $1)",
            pqxx::params{ kw }))
        {
            auto it = docs.find(url_id);
            if (it == docs.end()) continue;
            double tf = static_cast<double>(qty) / static_cast<double>(it->second.second);
            scores[url_id] += tf * idf;
        }
    }

    std::vector<tfidf_result> results;
    results.reserve(scores.size());
    for (const auto& [url_id, score] : scores)
    {
        auto it = docs.find(url_id);
        if (it != docs.end())
            results.push_back({ it->second.first, score });
    }

    std::sort(results.begin(), results.end(),
        [](const tfidf_result& a, const tfidf_result& b) { return a.score > b.score; });

    return results;
}

int data_base::get_total_files()
{
    return get_total_documents();
}

int data_base::get_total_keywords()
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    return tx.query_value<int>("SELECT COUNT(*) FROM keywords");
}

int data_base::get_total_relations()
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    return tx.query_value<int>("SELECT COUNT(*) FROM keyword_url_qty");
}

void data_base::add_search_query(const std::string& query, int results_count, double elapsed_sec)
{
    auto conn = pool_->get_connection();
    pqxx::work tx{ conn.get() };
    tx.exec(
        "INSERT INTO search_history (query, results_count, elapsed_sec) VALUES ($1, $2, $3)",
        pqxx::params{ query, results_count, elapsed_sec });
    tx.commit();
}

std::vector<std::tuple<std::string, std::string, int, double>> data_base::get_search_history(int limit)
{
    auto conn = pool_->get_connection();
    pqxx::nontransaction tx{ conn.get() };
    std::vector<std::tuple<std::string, std::string, int, double>> result;
    for (auto const& [ts, query, cnt, elapsed] :
        tx.query<std::string, std::string, int, double>(
            "SELECT TO_CHAR(searched_at, 'YYYY-MM-DD HH24:MI:SS'), query, results_count, elapsed_sec "
            "FROM search_history ORDER BY searched_at DESC LIMIT $1",
            pqxx::params{ limit }))
    {
        result.emplace_back(ts, query, cnt, elapsed);
    }
    return result;
}

void data_base::create_db_()
{
    auto conn = pool_->get_connection();
    pqxx::work tx{ conn.get() };
    tx.exec("CREATE TABLE IF NOT EXISTS keywords("
        "id SERIAL PRIMARY KEY, "
        "keyword TEXT NOT NULL UNIQUE)");
    tx.exec("CREATE TABLE IF NOT EXISTS urls("
        "id SERIAL PRIMARY KEY, "
        "url TEXT NOT NULL UNIQUE, "
        "modified_time BIGINT NOT NULL DEFAULT 0)");
    tx.exec("DO $$ BEGIN "
        "IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name='keyword_urs_qty') "
        "AND NOT EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name='keyword_url_qty') "
        "THEN ALTER TABLE keyword_urs_qty RENAME TO keyword_url_qty; END IF; END $$;");
    tx.exec("CREATE TABLE IF NOT EXISTS keyword_url_qty("
        "id_keyword INTEGER REFERENCES keywords(id), "
        "id_url INTEGER REFERENCES urls(id), "
        "quantity INTEGER NOT NULL, "
        "CONSTRAINT k_u PRIMARY KEY(id_keyword, id_url))");
    tx.exec("CREATE TABLE IF NOT EXISTS search_history("
        "id SERIAL PRIMARY KEY, "
        "query TEXT NOT NULL, "
        "results_count INTEGER NOT NULL DEFAULT 0, "
        "elapsed_sec DOUBLE PRECISION NOT NULL DEFAULT 0, "
        "searched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
    tx.commit();
}
