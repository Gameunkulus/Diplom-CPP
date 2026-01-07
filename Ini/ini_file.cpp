#include "ini_file.h"

#include <iostream>
#include <regex>

constexpr auto ALLOWED_CHAR_NAME = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_-";

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

std::string ini_file::get_value(std::string_view key) const
{
	size_t position_dot = key.find('.');
	
	std::string_view name_section = key.substr(0, position_dot);
	auto section_it = value_.find( name_section );
	if ( section_it != value_.end() )
	{
		std::string_view name_value = key.substr(position_dot + 1, key.length());
		auto value_it = section_it->second.find( name_value );
		if ( value_it != section_it->second.end() )
		{
			return value_it->second;
		}
	}
	return std::string();
}

void ini_file::read_file_() 
{
	do
	{
		std::string line;
		std::getline(file_, line);
		if ( !line.empty() )
		{
			if ( line.at( 0 ) == ';' || line.at( 0 ) == '#' )
			{
				continue;
			}
			else
			{
				if ( line.at( 0 ) == '[' )
				{
					read_section_( line );
				}
				else if ( (line.at( 0 ) >= 'A' && line.at( 0 ) <= 'Z') || (line.at( 0 ) >= 'a' && line.at( 0 ) <= 'z') ) {
					read_value_( line );
				}
			}
		}
	} while (!file_.eof());
}

void ini_file::read_section_(std::string_view read_name)
{
	size_t close_char = read_name.find(']');
	if (close_char != std::string_view::npos)
	{
		current_section_ = read_name.substr( 1, close_char - 1 );
		value_[current_section_];
	}
}

void ini_file::read_value_(std::string_view read_name)
{
	size_t eq_position = read_name.find('=');
	if (eq_position == std::string_view::npos)
	{
		current_value_ = ignore_space_( read_name );
		value_[current_section_][current_value_];
	}
	else 
	{
		auto param_name = read_name.substr(0, eq_position);
		current_value_ = ignore_space_( param_name );
		
		auto param_value = read_name.substr( eq_position + 1, read_name.length() - eq_position );
		value_[current_section_][current_value_] = ignore_space_( param_value );
	}
}

std::string ini_file::ignore_space_( std::string_view value ) const
{
	auto is_not_space = []( auto c ) { return !std::isspace( c ); };
	std::string trimmed_value;
	std::copy_if( value.begin(), value.end(), std::back_inserter( trimmed_value ), is_not_space );
	return trimmed_value;
}
