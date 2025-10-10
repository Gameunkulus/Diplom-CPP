#include "http_connection.h"

#include <stream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;



std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string convert_to_utf8(const std::string& str) {
	std::string url_decoded = url_decode(str);
	return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket, data_base* db)
	: socket_(std::move(socket)), db_{ db }
{
}


void HttpConnection::start()
{
	readRequest();
	checkDeadline();
}

void HttpConnection::print_not_found()
{
	response_.set(http::field::content_type, "text/html");
	beast::ostream(response_.body())
		<< "<html>\n"
		<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
		<< "<body>\n"
		<< "<h1>Search Engine</h1>\n"
		<< "<p>Response:<p>\n"
		<< "<ul>\n"
		<< "<p><b> No matches found </b></p>"
		<< "</ul>\n"
		<< "</body>\n"
		<< "</html>\n";
}

void HttpConnection::print_bad_request()
{
	response_.set(http::field::content_type, "text/html");
	beast::ostream(response_.body())
		<< "<html>\n"
		<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
		<< "<body>\n"
		<< "<h1>Search Engine</h1>\n"
		<< "<p>Response:<p>\n"
		<< "<ul>\n"
		<< "<p><b> Bad request </b></p>"
		<< "<p> The request must consist of English words longer than 3 characters </p>"
		<< "</ul>\n"
		<< "</body>\n"
		<< "</html>\n";
}

void HttpConnection::print_result(const std::vector<std::string>& searchResult)
{
	response_.set(http::field::content_type, "text/html");
	beast::ostream(response_.body())
		<< "<html>\n"
		<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
		<< "<body>\n"
		<< "<h1>Search Engine</h1>\n"
		<< "<p>Response:<p>\n"
		<< "<ul>\n";

	for (const auto& url : searchResult) {

		beast::ostream(response_.body())
			<< "<li><a href=\""
			<< url << "\">"
			<< url << "</a></li>\n";
	}

	beast::ostream(response_.body())
		<< "</ul>\n"
		<< "</body>\n"
		<< "</html>\n";
}
void HttpConnection::print_result(const std::multimap<int, std::string, std::greater<int>>& searchResult)
{
	int result_size = 10;
	response_.set(http::field::content_type, "text/html");
	beast::ostream(response_.body())
		<< "<html>\n"
		<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
		<< "<body>\n"
		<< "<h1>Search Engine</h1>\n"
		<< "<p>Response:<p>\n"
		<< "<table>\n";

	for (const auto& [key, value] : searchResult)
	{
		beast::ostream(response_.body())
			<< "<tr>\n<td><a href = \""
			<< value << "\">"
			<< value << "</a></td>\n"
			<< "<td>" << key << "</td>\n</tr>\n";
		--result_size;
		if (result_size == 0) break;
	}

	beast::ostream(response_.body())
		<< "</table>\n"
		<< "</body>\n"
		<< "</html>\n";
}

std::vector<std::string> HttpConnection::convert_multimap_to_vector(const std::multimap<int, std::string, std::greater<int>>& multimap)
{
	std::vector<std::string> result;
	int result_size = 10;

	for (auto [key, value] : multimap)
	{
		result.push_back(value);
		--result_size;
		if (result_size == 0) break;
	}
	return result;
}

// Asynchronously receive a complete request message.
void HttpConnection::readRequest()
{
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

// Determine what needs to be done with the request message.
void HttpConnection::processRequest()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	writeResponse();
}


void HttpConnection::createResponseGet()
{
	if (request_.target() == "/")
	{
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Welcome!<p>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Search:</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
			<< "    <input type=\"submit\" value=\"Search\">\n"
			<< "</form>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}


void HttpConnection::createResponsePost()
{
	if (request_.target() == "/")
	{
		std::string s = buffers_to_string(request_.body().data());

		std::cout << "POST data: " << s << std::endl;

		size_t pos = s.find('=');
		if (pos == std::string::npos)
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::string key = s.substr(0, pos);
		std::string value = s.substr(pos + 1);

		std::string utf8value = convert_to_utf8(value);

		if (key != "search")
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		// TODO: Fetch your own search results here

		std::vector<std::string> keywords = get_keyword(value);

		if (keywords.size() > 0)
		{
			std::map<std::string, int> db_result;
			for (const std::string& s : keywords) {
				for (const auto& [key, value] : db_->get_urls_qry(s))
				{
					db_result[key] += value;
				}
			}
			if (db_result.size() > 0)
			{
				std::multimap<int, std::string, std::greater<int>> db_result_sort;
				for (const auto& [key, value] : db_result)
				{
					db_result_sort.insert({ value, key });
				}
				print_result(db_result_sort);
				//print_result(convert_multimap_to_vector(db_result_sort));
			}
			else
			{
				print_not_found();
			}
		}
		else {
			print_bad_request();
		}
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

// Asynchronously transmit the response message.
void HttpConnection::writeResponse()
{
	auto self = shared_from_this();

	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

// Check whether we have spent enough time on this connection.
void HttpConnection::checkDeadline()
{
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if (!ec)
			{
				self->socket_.close(ec);
			}
		});
}

std::vector<std::string> HttpConnection::get_keyword(const std::string& str)
{
	const std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::vector<std::string> result;
	boost::locale::generator gen;
	std::locale loc = gen("");
	std::locale::global(loc);
	std::cout.imbue(loc);
	std::string value = boost::locale::to_lower(str);

	std::string::size_type pos_end = 0;
	std::string::size_type pos_begin = 0;
	do {
		pos_end = value.find('+', pos_begin);
		std::string sub_str = value.substr(pos_begin, pos_end - pos_begin);
		if (sub_str.find_first_not_of(letters) == std::string::npos && sub_str.length() > 3)
		{
			if (std::find(result.begin(), result.end(), sub_str) == result.end())
			{
				result.push_back(sub_str);
			}
		}
		pos_begin = pos_end + 1;
	} while (pos_end != std::string::npos);

	return result;
}