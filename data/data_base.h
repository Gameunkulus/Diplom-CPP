#pragma once
#include <pqxx/pqxx>
#include <string>
#include <map>

namespace query
{
	constexpr std::string_view add_keyword = "add_keyword";
	constexpr std::string_view add_url = "add_url";
	constexpr std::string_view add_qty = "add_qty";
}

class data_base {
public:
	data_base(const std::string& host, const std::string& port, const std::string& dbname, const std::string& user, const std::string& password);
	data_base(const std::string& connection_string);
	bool add_keyword_link(const std::map<std::string, unsigned int>& words, const std::string& link);
	bool check_url(std::string_view url);
	bool check_keyword(std::string_view keyword);
	std::map<std::string, int> get_urls_qry(std::string_view keyword);
private:
	pqxx::connection conn_db_;
	void create_db_();
};