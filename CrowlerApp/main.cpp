#include <iostream>

#include "../ini_file/ini_file.h"
#include "../data_base/data_base.h"

#define NOMINMAX
#include <Windows.h>

#include "indexer.h"

int main()
{
	setlocale(LC_ALL, "ru_RU.utf-8");
	std::string db_name;
	try 
	{
		ini_file ini_file("config.ini");

		std::string start_page = ini_file.get_value("Client.start_page");
		int recursion_depth = std::stoi(ini_file.get_value("Client.recursion_depth"));

		db_name = ini_file.get_value( "DataBase.bd_name" );
		auto db = std::make_shared<data_base>( ini_file.get_value( "DataBase.bd_host" ), ini_file.get_value( "DataBase.bd_port" ),
			db_name, ini_file.get_value( "DataBase.bd_user" ), ini_file.get_value( "DataBase.bd_pass" ) );

		unsigned int num_threads = std::thread::hardware_concurrency();
		auto mgr = std::make_shared<task_manager>( num_threads );
		indexer ind( mgr, db );

		mgr->push_task( [start_page, recursion_depth, &ind]()
			{
				ind.parse_link( ind.make_first_link( start_page ), recursion_depth );
			} );

		std::this_thread::sleep_for(std::chrono::seconds(2));

		mgr->wait();
	}
	catch ( const pqxx::broken_connection& e )
	{
		std::cout << e.what() << std::endl;
		std::cout << "possible reasons: " << std::endl 
			<< "\tconnection is anavailable;" << std::endl
			<< "\tserver has no db with name '" << db_name << "'; " << std::endl
			<< "\tpermission denied." << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
