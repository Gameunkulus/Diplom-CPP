#pragma once
#include <fstream>
#include <map>

class ini_file {
public: 
	ini_file() = delete;
	ini_file(const std::string& fileName);
	~ini_file();

	std::string get_value(std::string_view key) const;

private:
	std::ifstream file_;
	std::string current_section_;
	std::string current_value_;
	std::map<std::string, std::map<std::string, std::string, std::less<>>, std::less<>> value_;
	void read_file_();
	void read_section_(std::string_view read_name);
	void read_value_(std::string_view read_name);
	std::string ignore_space_(std::string_view value) const;
};