#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <map>

#include <Windows.h>
#include "http_connection.h"

#include "../ini_parser/ini_parser.h"
#include "../data_base/data_base.h"

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket, data_base& db)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
				std::make_shared<HttpConnection>(std::move(socket), &db)->start();
			httpServer(acceptor, socket, db);
		});
}


int main(int argc, char* argv[])
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try
	{
		ini_parser ini_file("C:/repos/Diplom/config.ini");

		std::string initialize_connection_db("host=" + ini_file.get_value("DataBase.bd_host") +
			" port=" + ini_file.get_value("DataBase.bd_port") +
			" dbname=" + ini_file.get_value("DataBase.bd_name") +
			" user=" + ini_file.get_value("DataBase.bd_user") +
			" password=" + ini_file.get_value("DataBase.bd_pass"));

		data_base db(initialize_connection_db);

		unsigned short port = static_cast<unsigned short>(std::stoi(ini_file.get_value("Server.server_port")));
		auto const address = net::ip::make_address("0.0.0.0");

		net::io_context ioc{ 1 };

		tcp::acceptor acceptor{ ioc, { address, port } };
		tcp::socket socket{ ioc };
		httpServer(acceptor, socket, db);

		std::cout << "Open browser and connect to http://localhost:" << port << " to see the web server operating" << std::endl;

		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}