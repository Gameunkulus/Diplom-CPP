#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "http_ustils.h"
#include <functional>

#include "../Ini/ini_file.h"
#include "../data/data_base.h"

#include <boost/locale.hpp>
#include <regex>
#include <map>
#include <Windows.h>

std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;


void threadPoolWorker() {
	std::unique_lock<std::mutex> lock(mtx);
	while (!exitThreadPool || !tasks.empty()) {
		if (tasks.empty()) {
			cv.wait(lock);
		}
		else {
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}

std::map<std::string, unsigned int> clear_html_tag(const std::string& html)

{

	boost::locale::generator gen;
	std::locale loc = gen("");
	std::locale::global(loc);
	std::cout.imbue(loc);

	const std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const int  size_wold = 3;
	std::string word;
	std::map<std::string, unsigned int> result;

	for (int i = 0; i < html.length(); ++i) {
		if (html.at(i) == '<')
		{
			if (word.length() > size_wold) {
				word = boost::locale::to_lower(word);
				result[word]++;
			};
			word.erase();

			std::string tag_name = "";
			while (html.at(i + 1) != ' ' && html.at(i + 1) != '>') {
				tag_name += html.at(i + 1);
				++i;
			}
			while (html.at(i) != '>') {
				++i;
			}
			if (tag_name == "script" || tag_name == "style") {
				while (html.at(i + 1) != '<') {
					++i;
				}
			}
		}
		else
		{
			if (letters.find_first_of(html.at(i)) != std::string::npos)
			{
				word += html.at(i);
			}
			else {
				if (word.length() > size_wold) {
					word = boost::locale::to_lower(word);
					result[word]++;
				};
				word.erase();
			}
		}
	}
	return result;
}

Link make_first_link(const std::string& url)
{
	std::regex ur("(https?)?(:?\/\/)?([[:alnum:]-_]+\..*?)?(\/.*)");

	std::smatch sm;
	std::regex_search(url, sm, ur);

	Link tmp_link;


	if (sm[1].length() != 0)
	{
		if (sm[1].str() == "http") {
			tmp_link.protocol = ProtocolType::HTTP;
		}
		else if (sm[1].str() == "https") {
			tmp_link.protocol = ProtocolType::HTTPS;
		}
	}
	else
	{
		throw "Bad first URL: not find protocol.";
	};

	if (sm[3].length() != 0)
	{
		tmp_link.hostName = sm[3].str();
	}
	else
	{
		throw "Bad first URL: not find host.";
	};

	if (sm[4].length() != 0) {
		tmp_link.query = sm[4].str();
	}
	else {
		tmp_link.query = '/';
	};
	return tmp_link;
}

Link make_link(const std::string& url, const Link& current_link)
{
	std::regex ur("(https?)?(:?\/\/)?([[:alnum:]_-]+\.[^\/]+)?(\/.*(#[^\/]+$)?)");

	std::smatch sm;
	std::regex_search(url, sm, ur);

	Link tmp_link;


	if (sm[1].length() != 0)
	{
		if (sm[1].str() == "http") {
			tmp_link.protocol = ProtocolType::HTTP;
		}
		else if (sm[1].str() == "https") {
			tmp_link.protocol = ProtocolType::HTTPS;
		}
	}
	else
	{
		tmp_link.protocol = current_link.protocol;
	};
	if (sm[3].length() != 0)
	{
		tmp_link.hostName = sm[3].str();
	}
	else
	{
		tmp_link.hostName = current_link.hostName;
	};
	if (sm[4].length() != 0) {
		if (sm[5].length() == 0)
		{
			tmp_link.query = sm[4].str();
		}
		else
		{
			tmp_link.query = sm[4].str().substr(0, sm[4].length() - sm[5].length());
		}
	}
	else {
		tmp_link.query = '/';
	}
	return tmp_link;
}

std::vector<Link> get_link(const std::string& html, const Link& current_link)
{
	std::regex html_link("<a href=\"(.*?)\"");
	std::vector<Link> links_result;

	auto links_begin = std::sregex_iterator(html.begin(), html.end(), html_link);
	auto links_end = std::sregex_iterator();

	for (std::sregex_iterator i = links_begin; i != links_end; ++i) {
		std::smatch sm = *i;
		if (sm[1].str().at(0) != '#')
		{
			Link tmp_link = make_link(sm[1].str(), current_link);
			if (std::find(links_result.begin(), links_result.end(), tmp_link) == links_result.end())
			{
				links_result.push_back(tmp_link);
			}
		}
	}
	return links_result;
}

std::string link_to_string(const Link& link) {
	if (link.protocol == ProtocolType::HTTP) {
		return("http://" + link.hostName + link.query);
	}
	else {
		return("https://" + link.hostName + link.query);
	}
}

void parseLink(const Link& link, int depth, data_base& db)
{
	try {
		std::string html = getHtmlContent(link);

		if (html.size() == 0)
		{
			std::cout << ("Failed to get HTML Content from: " + link.hostName + link.query) << std::endl;
			return;
		}

		// TODO: Parse HTML code here on your own
		std::map<std::string, unsigned int> word_and_count(clear_html_tag(html));
		if (word_and_count.size() > 0) {

			std::lock_guard<std::mutex> lock(mtx);
			db.add_keyword_link(word_and_count, link_to_string(link));
		}

		// TODO: Collect more links from HTML code and add them to the parser like that:

		if (depth > 0) {
			std::vector<Link> links(get_link(html, link));
			std::lock_guard<std::mutex> lock(mtx);

			size_t count = links.size();
			size_t index = 0;
			for (auto& subLink : links)
			{
				if (!db.check_url(subLink.hostName + subLink.query))
				{
					tasks.push([subLink, depth, &db]() { parseLink(subLink, depth - 1, db); });
				}
			}
			cv.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

}


int main()
{
	setlocale(LC_ALL, "ru_RU.utf-8");
	try {
		ini_parser ini_file("C:/repos/Diplom/config.ini");

		std::string start_page = ini_file.get_value("Client.start_page");
		int recursion_depth = std::stoi(ini_file.get_value("Client.recursion_depth"));

		std::string initialize_connection_db("host=" + ini_file.get_value("DataBase.bd_host") +
			" port=" + ini_file.get_value("DataBase.bd_port") +
			" dbname=" + ini_file.get_value("DataBase.bd_name") +
			" user=" + ini_file.get_value("DataBase.bd_user") +
			" password=" + ini_file.get_value("DataBase.bd_pass"));

		data_base db(initialize_connection_db);

		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}

		Link link = make_first_link(start_page);

		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link, recursion_depth, &db]() { parseLink(link, recursion_depth, db); });
			cv.notify_one();
		}

		std::this_thread::sleep_for(std::chrono::seconds(2));


		{
			std::lock_guard<std::mutex> lock(mtx);
			exitThreadPool = true;
			cv.notify_all();
		}

		for (auto& t : threadPool) {
			t.join();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
