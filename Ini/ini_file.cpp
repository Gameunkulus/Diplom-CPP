#include "ini_file.h"

#include<iostream>
#include<regex>

#define ALLOWED_CHAR_NAME "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_-"

ini_file::ini_file(const std::string& file_name)
{
	file_.open(file_name);
		read_file_();
}

ini_file::~ini_file()
{
	if (file_.is_open())
	{
		file_.close();
	}
}

std::string ini_file::get_value(const std::string& key)
{
	int position_dot = key.find('.');
	
	std::string name_section = key.substr(0, position_dot);
	std::string name_value = key.substr(position_dot + 1, key.length());
		
	return value_[name_section][name_value];
	
}

void ini_file::read_file_() 
{
	std::string word_from_ini;
	bool next_read_not_error = true;
	do {
		do
		{
			file_ >> word_from_ini;
			if (word_from_ini.at(0) == ';' || word_from_ini.at(0) == '#')
			{
				file_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				file_.unget();
			}
			else
			{
				if (word_from_ini.at(0) == '[')
				{
					get_section_(word_from_ini);
					next_read_not_error = false;
				}
				else if ((word_from_ini.at(0) >= 'A' && word_from_ini.at(0) <= 'Z') || (word_from_ini.at(0) >= 'a' && word_from_ini.at(0) <= 'z')) { 
					get_value_(word_from_ini);
					next_read_not_error = false;
				}
			}
			ignore_space_to_word();
		} while (file_.peek() != '\n' && !file_.eof());
		++current_read_line_;
		next_read_not_error = true;
	} while (!file_.eof());
}

void ini_file::get_section_(std::string& read_name)
{
	int close_char = read_name.find(']');
	
	read_name.erase(close_char);
	read_name.erase(0, 1);
	current_section_ = read_name;
	value_[current_section_];
	
}

void ini_file::get_value_(std::string& read_name)
{
	std::string word_from_ini;
	int eq_position = read_name.find('=');
	if (eq_position == std::string::npos)
	{
		current_value_ = read_name;
		file_ >> word_from_ini;
		if (word_from_ini.length() == 1)
		{
		ignore_space_to_word();
		file_ >> word_from_ini;
		value_[current_section_][current_value_] = word_from_ini;
		}
		else {
		word_from_ini.erase(0, 1);
		value_[current_section_][current_value_] = word_from_ini;
		}	
	}
	else {
		current_value_ = read_name.substr(0, eq_position);
		if (read_name.length() == eq_position + 1) {
			file_ >> word_from_ini;
			value_[current_section_][current_value_] = word_from_ini;
		}
		else {
			value_[current_section_][current_value_] = read_name.substr(eq_position + 1, read_name.length() - eq_position);
		}
	}
}

void ini_file::ignore_space_to_word()
{
	while (file_.peek() == ' ' || file_.peek() == '\t')
	{
		file_.get();
	}
}