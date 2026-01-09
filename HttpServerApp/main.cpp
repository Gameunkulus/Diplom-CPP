#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <map>

#include "http_connection.h"
#include <Windows.h>

#include "../ini_file/ini_file.h"
#include "../data_base/data_base.h"

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket, std::shared_ptr<data_base> db)
{
	acceptor.async_accept(socket,
		[&acceptor, &socket, db](beast::error_code ec)
		{
			if (!ec)
				std::make_shared<HttpConnection>(std::move(socket), db)->start();
			httpServer(acceptor, socket, db);
		});
}

int main(int argc, char* argv[])
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try
	{
		ini_file ini_file("config.ini");

		auto db = std::make_shared<data_base>( ini_file.get_value( "DataBase.bd_host" ), ini_file.get_value( "DataBase.bd_port" ),
			ini_file.get_value( "DataBase.bd_name" ), ini_file.get_value( "DataBase.bd_user" ), ini_file.get_value( "DataBase.bd_pass" ) );

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
	return 0;
}