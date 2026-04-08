#pragma once
#include <pqxx/pqxx>
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <memory>

class connection_pool
{
public:
    connection_pool(const std::string& conn_string, size_t pool_size)
    {
        for (size_t i = 0; i < pool_size; ++i)
            pool_.push(std::make_unique<pqxx::connection>(conn_string));
    }

    // RAII-обёртка: получаем соединение из пула, при разрушении — возвращаем
    class scoped_connection
    {
    public:
        scoped_connection(connection_pool& pool)
            : pool_(pool), conn_(pool.acquire()) {}
        ~scoped_connection() { pool_.release(std::move(conn_)); }

        pqxx::connection& get() { return *conn_; }
        pqxx::connection* operator->() { return conn_.get(); }

        scoped_connection(const scoped_connection&) = delete;
        scoped_connection& operator=(const scoped_connection&) = delete;
    private:
        connection_pool& pool_;
        std::unique_ptr<pqxx::connection> conn_;
    };

    scoped_connection get_connection() { return scoped_connection(*this); }

private:
    std::unique_ptr<pqxx::connection> acquire()
    {
        std::unique_lock lock(mtx_);
        cv_.wait(lock, [this] { return !pool_.empty(); });
        auto conn = std::move(pool_.front());
        pool_.pop();
        return conn;
    }

    void release(std::unique_ptr<pqxx::connection> conn)
    {
        std::unique_lock lock(mtx_);
        pool_.push(std::move(conn));
        cv_.notify_one();
    }

    std::queue<std::unique_ptr<pqxx::connection>> pool_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

class data_base {
public:
    data_base(const std::string& host, const std::string& port,
        const std::string& dbname, const std::string& user, const std::string& password,
        size_t pool_size = 4);
    data_base(const std::string& connection_string, size_t pool_size = 4);

    // Индексация
    bool add_keyword_link(const std::map<std::string, unsigned int>& words,
        const std::string& link, long long modified_time);
    bool check_url(std::string_view url);
    long long get_url_modified_time(std::string_view url);
    void remove_url(const std::string& url);
    int get_total_documents();

    // Поиск
    std::map<std::string, int> get_urls_qry(std::string_view keyword);
    int get_document_frequency(std::string_view keyword);

    struct tfidf_result {
        std::string url;
        double score;
    };
    std::vector<tfidf_result> search_tfidf(const std::vector<std::string>& keywords);

    // Статистика
    int get_total_files();
    int get_total_keywords();
    int get_total_relations();

    // История поиска
    void add_search_query(const std::string& query, int results_count, double elapsed_sec);
    std::vector<std::tuple<std::string, std::string, int, double>> get_search_history(int limit = 50);

private:
    std::string conn_string_;
    std::unique_ptr<connection_pool> pool_;
    void create_db_();
};
