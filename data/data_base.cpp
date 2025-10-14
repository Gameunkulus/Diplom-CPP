#include "../data/data_base.h"

data_base::data_base(const std::string& idb) :conn_db_{ pqxx::connection(idb) }
{
	create_db_();
	conn_db_.prepare("add_keyword", "INSERT INTO keywords (keyword) VALUES ( $1 )");
	conn_db_.prepare("add_url", "INSERT INTO urls (url) VALUES ( $1 )");
	conn_db_.prepare("add_qty", "INSERT INTO keyword_urs_qty (id_keyword, id_url, quantity) VALUES ("
		"(SELECT id FROM keywords WHERE keyword = $1),"
		"(SELECT id FROM urls WHERE url = $2), $3)");
}

void data_base::add_keyword_link(const std::map<std::string, unsigned int>& words, const std::string& link)
{
	if (check_url(link))
	{
		return;
	}
	pqxx::work tx{ conn_db_ };
	tx.exec_prepared("add_url", link);
	tx.commit();
	for (const auto& [key, value] : words)
	{
		if (!check_keyword(key))
		{
			tx.exec_prepared("add_keyword", key);
			tx.commit();
		}
		tx.exec_prepared("add_qty", key, link, std::to_string(value));
	}
	tx.commit();
}

bool data_base::check_url(const std::string& url)
{
	pqxx::work tx{ conn_db_ };
	bool check_id = false;
	check_id = tx.query_value<bool>("SELECT EXISTS(SELECT id FROM urls WHERE url = \'" + url + "\');");
	return check_id;
}

bool data_base::check_keyword(const std::string& keyword)
{
	pqxx::work tx{ conn_db_ };
	bool check_id = false;
	check_id = tx.query_value<bool>("SELECT EXISTS(SELECT id FROM keywords WHERE keyword = \'" + keyword + "\');");
	return check_id;
}

std::map<std::string, int> data_base::get_urls_qry(const std::string& keyword)
{
	pqxx::work tx{ conn_db_ };
	std::map<std::string, int> result;
	std::string key_esc = tx.esc(keyword);
	for (auto [url, qty] : tx.query<std::string, int>
		("SELECT u.url, kuq.quantity "
			"FROM urls u "
			"JOIN keyword_urs_qty kuq ON u.id = kuq.id_url "
			"JOIN keywords k ON kuq.id_keyword = k.id "
			"WHERE k.keyword = '" + key_esc + "' "))
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