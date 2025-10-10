#pragma once
#include <pqxx/pqxx>
#include <string>
#include <map>

class data_base {
public:
	data_base(const std::string& idb);
	void add_keyword_link(const std::map<std::string, unsigned int>& words, const std::string& link);
	bool check_url(const std::string& url);
	bool check_keyword(const std::string& keyword);
	std::map<std::string, int> get_urls_qry(const std::string& keyword);
private:
	pqxx::connection conn_db_;
	void create_db_();
};