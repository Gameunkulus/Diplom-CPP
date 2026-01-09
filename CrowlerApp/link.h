#pragma once 
#include <string>
#include <unordered_set>

enum class ProtocolType
{
	HTTP = 0,
	HTTPS = 1
};

struct link
{
	ProtocolType protocol;
	std::string host_name;
	std::string query;

	bool operator==(const link& l) const
	{
		return protocol == l.protocol
			&& host_name == l.host_name
			&& query == l.query;
	}
};

inline std::string to_string( const link& link ) 
{
	if ( link.protocol == ProtocolType::HTTP ) {
		return("http://" + link.host_name + link.query);
	}
	else {
		return("https://" + link.host_name + link.query);
	}
}