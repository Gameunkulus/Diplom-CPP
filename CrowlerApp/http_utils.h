#pragma once 

#include <string>
#include "link.h"

namespace http_utils
{
	std::string get_html_content(const link& link);
}