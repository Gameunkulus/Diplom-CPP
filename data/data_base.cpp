#include "data_base.h"
#include <format>

data_base::data_base( const std::string& host, const std::string& port, const std::string& dbname, const std::string& user, const std::string& password ) 
	: data_base( std::string{ "host=" + host + " " + "port=" + port + " " + "dbname=" + dbname + " " + "user=" + user + " " + "password=" + password } )
{
}

data_base::data_base( const std::string& connection_string ) : conn_db_( pqxx::connection( connection_string ) )
{
	create_db_();
	conn_db_.prepare( query::add_keyword.data(), "INSERT INTO keywords (keyword) VALUES ( $1 )" );
	conn_db_.prepare( query::add_url.data(), "INSERT INTO urls (url) VALUES ( $1 )" );
	conn_db_.prepare( query::add_qty.data(), "INSERT INTO keyword_urs_qty (id_keyword, id_url, quantity) VALUES ("
		"(SELECT id FROM keywords WHERE keyword = $1),"
		"(SELECT id FROM urls WHERE url = $2), $3)" );
}


bool data_base::add_keyword_link(const std::map<std::string, unsigned int>& words, const std::string& link)
{
	if (check_url(link))
		return false;
	pqxx::work tx{ conn_db_ };
	tx.exec(pqxx::prepped(pqxx::zview(query::add_url)), link);
	tx.commit();
	for (const auto& [key, value] : words)
	{
		if (!check_keyword(key))
		{
			tx.exec(pqxx::prepped(pqxx::zview(query::add_keyword)), key);
			tx.commit();
		}
		tx.exec(pqxx::prepped(pqxx::zview(query::add_qty)), pqxx::params{key, link, std::to_string(value)});
	}
	tx.commit();
	return true;
}

bool data_base::check_url(std::string_view url)
{
	pqxx::read_transaction tx{ conn_db_ };
	bool check_id = false;
	check_id = tx.query_value<bool>(R"(SELECT EXISTS(SELECT id FROM urls WHERE url = $1);)", pqxx::params{url});
	return check_id;
}

bool data_base::check_keyword(std::string_view keyword)
{
	pqxx::read_transaction tx{ conn_db_ };
	bool check_id = false;
	check_id = tx.query_value<bool>(R"(SELECT EXISTS(SELECT id FROM keywords WHERE keyword = $1);)", pqxx::params{keyword});
	return check_id;
}

std::map<std::string, int> data_base::get_urls_qry(std::string_view keyword)
{
	pqxx::read_transaction tx{ conn_db_ };
	std::map<std::string, int> result;
	std::string key_esc = tx.esc(keyword);
	for (auto const& [url, qty] : tx.query<std::string, int>
		("SELECT u.url, kuq.quantity "
			"FROM urls u "
			"JOIN keyword_urs_qty kuq ON u.id = kuq.id_url "
			"JOIN keywords k ON kuq.id_keyword = k.id "
			"WHERE k.keyword = $1 ", pqxx::params{key_esc}))
	{
		result[url] = qty;
	};
	return result;
}

void data_base::create_db_()
{
	pqxx::work tx{ conn_db_ };
	tx.exec("CREATE TABLE IF NOT EXISTS keywords( "
		"id SERIAL PRIMARY KEY, "
		"keyword TEXT NOT NULL UNIQUE)"
	);
	tx.exec("CREATE TABLE IF NOT EXISTS urls( "
		"id SERIAL PRIMARY KEY, "
		"url TEXT NOT NULL UNIQUE)"
	);
	tx.exec("CREATE TABLE IF NOT EXISTS keyword_urs_qty( "
		"id_keyword INTEGER REFERENCES keywords(id), "
		"id_url INTEGER REFERENCES urls(id), "
		"quantity INTEGER NOT NULL, "
		"CONSTRAINT k_u PRIMARY KEY(id_keyword, id_url))"
	);
	tx.commit();
}