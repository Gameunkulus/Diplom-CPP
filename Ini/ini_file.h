#pragma once
#include<fstream>
#include<map>

class ini_file {
public: 
	ini_file() = delete;
	ini_file(const std::string& fileName);
	~ini_file();

	std::string get_value(const std::string& key);
private:

	std::ifstream file_;
	int current_read_line_ = 1;
	std::string current_section_ = "";
	std::string current_value_ = "";
	std::map<std::string, std::map<std::string, std::string>> value_;
	void read_file_();
	void get_section_(std::string& read_name);
	void get_value_(std::string& read_name);
	void ignore_space_to_word();
};