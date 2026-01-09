#include "indexer.h"

#include <iostream>
#include <regex>

#include <boost/locale.hpp>

#include "http_utils.h"

indexer::indexer( std::shared_ptr<task_manager> mgr, std::shared_ptr<data_base> db ) : mgr_(mgr), db_(db)
{
	boost::locale::generator gen;
	std::locale loc = gen( "" );
	std::locale::global( loc );
	std::cout.imbue( loc );
}

std::map<std::string, unsigned int> indexer::clear_html_tag( const std::string& html )
{
	constexpr size_t min_word_size = 3;
	std::string word;
	std::map<std::string, unsigned int> result;

	auto process_word = [&result, min_word_size]( std::string& word )
		{
			if ( word.length() > min_word_size ) {
				word = boost::locale::to_lower( word );
				result[word]++;
			};
			word.clear();
		};

	auto skip_html_tag = []( const std::string& html, size_t start_pos ) -> size_t
		{
			size_t i = start_pos;
			const size_t length = html.length();

			while ( i < length && html[i] != '>' ) {
				++i;
			}

			if ( i >= length ) 
				return length;

			std::string_view tag_view( html.data() + start_pos + 1, i - start_pos - 1 );

			size_t space_pos = tag_view.find( ' ' );
			if ( space_pos != std::string_view::npos ) {
				tag_view = tag_view.substr( 0, space_pos );
			}

			if ( tag_view == "script" || tag_view == "style" ) {
				std::string closing_tag = "</";
				closing_tag.append( tag_view );
				closing_tag += '>';

				size_t close_pos = html.find( closing_tag, i + 1 );
				if ( close_pos != std::string::npos ) 
				{
					return close_pos + closing_tag.length() - 1;
				}
			}

			return i;
		};

	auto is_letter = []( char c )
		{
			return std::isalpha( c );
			//constexpr std::string_view letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
			//return letters.find_first_of( c ) != std::string::npos;
		};

	const size_t length = html.length();
	for ( size_t i = 0; i < length; ++i ) 
	{
		char current = html[i];
		if ( current == '<' )
		{
			process_word( word );
			i = skip_html_tag( html, i );
		}
		else
		{
			if ( is_letter( current ) )
			{
				word += current;
			}
			else 
			{
				process_word( word );
			}
		}
	}

	process_word( word );

	return result;
}

//std::map<std::string, unsigned int> indexer::clear_html_tag( const std::string& html )
//{
//	const std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
//	const int  size_wold = 3;
//	std::string word;
//	std::map<std::string, unsigned int> result;
//
//	size_t length = html.length();
//	for ( int i = 0; i < length; ++i ) {
//		if ( html.at( i ) == '<' )
//		{
//			if ( word.length() > size_wold ) {
//				word = boost::locale::to_lower( word );
//				result[word]++;
//			};
//			word.erase();
//
//			std::string tag_name = "";
//			while ( i + 1 < length && html.at( i + 1 ) != ' ' && html.at( i + 1 ) != '>' ) {
//				tag_name += html.at( i + 1 );
//				++i;
//			}
//			while ( i < length && html.at( i ) != '>' ) {
//				++i;
//			}
//			if ( tag_name == "script" || tag_name == "style" ) {
//				while ( i + 1 < length && html.at( i + 1 ) != '<' ) {
//					++i;
//				}
//			}
//		}
//		else
//		{
//			if ( letters.find_first_of( html.at( i ) ) != std::string::npos )
//			{
//				word += html.at( i );
//			}
//			else {
//				if ( word.length() > size_wold ) {
//					word = boost::locale::to_lower( word );
//					result[word]++;
//				};
//				word.erase();
//			}
//		}
//	}
//	return result;
//}

link indexer::make_first_link( const std::string& url ) const
{
	static std::regex ur( R"((https?)?(:?\/\/)?([[:alnum:]-_]+\..*?)?(\/.*))" );

	std::smatch sm;
	std::regex_search( url, sm, ur );

	link tmp_link;

	if ( sm[1].length() != 0 )
	{
		if ( sm[1].str() == "http" ) {
			tmp_link.protocol = ProtocolType::HTTP;
		}
		else if ( sm[1].str() == "https" ) {
			tmp_link.protocol = ProtocolType::HTTPS;
		}
	}
	else
	{
		throw "Bad first URL: not find protocol.";
	};

	if ( sm[3].length() != 0 )
	{
		tmp_link.host_name = sm[3].str();
	}
	else
	{
		throw "Bad first URL: not find host.";
	};

	if ( sm[4].length() != 0 ) {
		tmp_link.query = sm[4].str();
	}
	else {
		tmp_link.query = '/';
	};
	return tmp_link;
}

link indexer::make_link( const std::string& url, const link& current_link ) const
{
	static std::regex ur( R"((https?)?(:?\/\/)?([[:alnum:]_-]+\.[^\/]+)?(\/.*(#[^\/]+$)?))" );

	std::smatch sm;
	std::regex_search( url, sm, ur );

	link tmp_link;

	if ( sm[1].length() != 0 )
	{
		if ( sm[1].str() == "http" ) {
			tmp_link.protocol = ProtocolType::HTTP;
		}
		else if ( sm[1].str() == "https" ) {
			tmp_link.protocol = ProtocolType::HTTPS;
		}
	}
	else
	{
		tmp_link.protocol = current_link.protocol;
	};
	if ( sm[3].length() != 0 )
	{
		tmp_link.host_name = sm[3].str();
	}
	else
	{
		tmp_link.host_name = current_link.host_name;
	};
	if ( sm[4].length() != 0 ) {
		if ( sm[5].length() == 0 )
		{
			tmp_link.query = sm[4].str();
		}
		else
		{
			tmp_link.query = sm[4].str().substr( 0, sm[4].length() - sm[5].length() );
		}
	}
	else {
		tmp_link.query = '/';
	}
	return tmp_link;
}

std::vector<link> indexer::get_links( const std::string& html, const link& current_link ) const
{
	static std::regex html_link( "<a href=\"(.*?)\"" );
	std::vector<link> links_result;

	auto links_begin = std::sregex_iterator( html.begin(), html.end(), html_link );
	auto links_end = std::sregex_iterator();

	for ( std::sregex_iterator i = links_begin; i != links_end; ++i ) {
		std::smatch sm = *i;
		if ( sm[1].str().at( 0 ) != '#' )
		{
			link tmp_link = make_link( sm[1].str(), current_link );
			if ( std::find( links_result.begin(), links_result.end(), tmp_link ) == links_result.end() )
			{
				links_result.push_back( tmp_link );
			}
		}
	}
	return links_result;
}

void indexer::parse_link( const link& target_link, int depth )
{
	auto get_content = []( const link& target_link )
		{
			std::string html = http_utils::get_html_content( target_link );
			if ( html.empty() )
				std::cout << "Failed to get HTML Content from: " << to_string( target_link ) << std::endl;
			return html;
		};
	try 
	{
		std::string html;

		// Parse HTML
		auto url = to_string( target_link );
		if ( !db_->check_url( url ) )
		{
			// not indexed
			html = get_content( target_link );
			if ( html.empty() )
				return;

			std::map<std::string, unsigned int> word_and_count = clear_html_tag( html );
			if ( !word_and_count.empty() ) 
			{
				db_->add_keyword_link( word_and_count, url );
			}
		}

		// Collect more links from HTML code and add them to the parser
		if ( depth > 0 ) 
		{
			if ( html.empty() )
			{
				// indexed. get content for sub links
				html = get_content( target_link );
			}
			if ( html.empty() )
				return;

			std::vector<link> sub_links = get_links( html, target_link );
			std::vector<std::function<void()>> tasks;
			for ( auto const& sub_link : sub_links )
			{
				tasks.emplace_back( [sub_link, depth, this]() { parse_link( sub_link, depth - 1 ); } );
			}
			if ( mgr_ )
				mgr_->push_tasks( tasks );
		}
	}
	catch ( const std::exception& e )
	{
		std::cout << e.what() << std::endl;
	}
}