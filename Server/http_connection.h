#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost.locale.hpp>
#include <regex>

#include "../data_base/data_base.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
protected:

	tcp::socket socket_;

	beast::flat_buffer buffer_{ 8192 };

	http::request<http::dynamic_body> request_;

	http::response<http::dynamic_body> response_;


	net::steady_timer deadline_{
		socket_.get_executor(), std::chrono::seconds(60) };

	void readRequest();
	void processRequest();

	void createResponseGet();

	void createResponsePost();
	void writeResponse();
	void checkDeadline();

	std::vector<std::string> get_keyword(const std::string& str);

public:
	HttpConnection(tcp::socket socket, data_base* db);
	void start();
private:
	data_base* db_;
	void print_not_found();
	void print_bad_request();
	void print_result(const std::vector<std::string>& searchResult);
	void print_result(const std::multimap<int, std::string, std::greater<int>>& searchResult);
	std::vector<std::string> convert_multimap_to_vector(const std::multimap<int, std::string, std::greater<int>>& multimap);
};