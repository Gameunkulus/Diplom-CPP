#pragma once 

#include <string>
#include <vector>
#include <map>

#include "link.h"
#include "task_manager.h"

#include "../data_base/data_base.h"

class indexer
{
public:
	indexer( std::shared_ptr<task_manager> mgr, std::shared_ptr<data_base> db );

	void parse_link( const link& current_link, int depth );
	std::vector<link> get_links( const std::string& html, const link& current_link ) const;

	link make_first_link( const std::string& url ) const;
	link make_link( const std::string& url, const link& current_link ) const;

private:
	std::map<std::string, unsigned int> clear_html_tag( const std::string& html );

	std::shared_ptr<task_manager> mgr_;
	std::shared_ptr<data_base> db_;
};