/* Copyright (c) 2015 Collin Baker
 *
 * This file is licensed under the MIT license
 * http://opensource.org/licenses/MIT
 */

#include "http/parse_request.hpp"

#include <new>
#include <string>
#include <utility>

#include <cctype>

namespace http
{

namespace
{

struct wrapper
{
	std::istream& src;
	request& req;
};

int hex_to_int(int ch)
{
	if(ch >= '0' && ch <= '9')
		return ch - '0';
	else if(ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if(ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return -1;
}

int parse_pchar(std::istream& src)
{
	int ch = src.get();

	if(std::isalpha(ch) || std::isdigit(ch))
	{
	        return ch;
	}

	switch(ch)
	{
	case '-': case '.': case '_': case '~': case ':': case '@':
	case '!': case '$': case '&': case '\'': case '(': case ')':
	case '*': case '+': case ',': case ';': case '=':
		return ch;
	default:
		break;
	}

	if(ch != '%')
	{
		return -1;
	}

	ch = 0;

	int temp = hex_to_int(src.get());
	if(temp < 0)
	{
		return -1;
	}
	ch += temp * 16;
	temp = hex_to_int(src.get());
	if(temp < 0)
	{
		return -1;
	}
	return ch + temp;
}

bool is_tchar(int c)
{
	switch(c)
	{
	case '!': case '#': case '$': case '%': case '&': case '\'': case '*':
	case '+': case '-': case '.': case '^': case '_': case '`': case '|': case '~':
		return true;
	default:
		break;
	}
	return std::isalnum(c);
}

parse_status parse_token(std::istream& src, std::string& token)
{
	int ch;

	token.clear();

	while(is_tchar(ch = src.get()))
	{
		token.push_back(ch);
	}
	if(ch != EOF)
	{
		src.putback(ch);
	}

	return token.empty() ? PARSE_FAILURE : PARSE_SUCCESS;
}

parse_status parse_request_target_origin_form(std::istream& src, request_target_origin_form& of)
{
	int ch;

	ch = src.get();
	if(ch != '/')
	{
		return PARSE_FAILURE;
	}
	while(true)
	{
		of.absolute_path.push_back(ch);
		ch = src.get();
		if(ch == '?')
		{
			break;
		}
		else if(ch == ' ')
		{
			return PARSE_SUCCESS;
		}
		else if(ch == EOF)
		{
			return PARSE_FAILURE;
		}
		else if(ch != '/')
		{
			src.putback(ch);
			ch = parse_pchar(src);
			if(ch < 0)
			{
				return PARSE_FAILURE;
			}
		}
	}

	of.query = std::string{};

	while(true)
	{
		ch = src.get();
		if(ch == '/' || ch == '?')
		{
			of.query->push_back(ch);
		}
		else if(ch == ' ')
		{
			break;
		}
		else
		{
			src.putback(ch);
			ch = parse_pchar(src);
			if(ch < 0)
			{
				return PARSE_FAILURE;
			}
			of.query->push_back(ch);
		}
	}

	return PARSE_SUCCESS;
}

parse_status parse_request_target(wrapper& w)
{
	int ch;
	std::string tempstr;

	ch = w.src.get();
	if(ch == '*')
	{
		w.req.target = request_target_asterisk_form{};
		w.src.get(); // Consume space
	}
	else if(ch == '/')
	{
		request_target_origin_form of;
		w.src.putback(ch);
		parse_status result = parse_request_target_origin_form(w.src, of);
		if(result != PARSE_SUCCESS)
		{
			return result;
		}
		w.req.target = std::move(of);
	}
	else
	{
		request_target_absolute_form af;

		// Parse the scheme
		while(ch != ':')
		{
			ch = std::tolower(ch);
			if(!(std::isalpha(ch) || std::isdigit(ch) || ch == '+' || ch == '-' || ch == '.'))
			{
				return PARSE_FAILURE;
			}
			af.scheme.push_back(ch);
			ch = w.src.get();
		}
		// An empty scheme is incorrect
		if(af.scheme.empty())
		{
			return PARSE_FAILURE;
		}
		
		// Consume double-slash before authority
		// since an absolute URI for http this usage
		// must have an authority
		if(w.src.get() != '/' || w.src.get() != '/')
		{
			return PARSE_FAILURE;
		}

		// Parse authority
		// TODO: make this follow RFC 3986 definition of 'authority'
		while((ch = w.src.get()) != '/')
		{
			if(!std::isgraph(ch))
			{
				return PARSE_FAILURE;
			}
			af.authority.push_back(ch);
		}
		w.src.putback(ch);

		parse_status result = parse_request_target_origin_form(w.src, af.origin_part);
		if(result != PARSE_SUCCESS)
		{
			return result;
		}
		w.req.target = std::move(af);
	}

	return PARSE_SUCCESS;
}
	
parse_status parse_request_line(wrapper& w)
{
	int ch;
	std::string tempstr;
	
	// First parse the method
	parse_status result = parse_token(w.src, tempstr);
	if(result != PARSE_SUCCESS)
	{
		return result;
	}

	// Consume space
	if((ch = w.src.get()) != ' ')
	{
		return PARSE_FAILURE;
	}

	if(tempstr == "GET")
		w.req.method = request_method::GET;
	else if(tempstr == "HEAD")
		w.req.method = request_method::HEAD;
	else if(tempstr == "POST")
		w.req.method = request_method::POST;
	else if(tempstr == "PUT")
		w.req.method = request_method::PUT;
	else if(tempstr == "DELETE")
		w.req.method = request_method::DELETE;
	else if(tempstr == "CONNECT")
		w.req.method = request_method::CONNECT;
	else if(tempstr == "OPTIONS")
		w.req.method = request_method::OPTIONS;
	else if(tempstr == "TRACE")
		w.req.method = request_method::TRACE;
	else
		return PARSE_FAILURE;

	// Next parse the target
	result = parse_request_target(w);
	if(result != PARSE_SUCCESS)
	{
		return result;
	}

	// Last consume the HTTP version and CRLF
	// Out of laziness I will ignore the HTTP version string and the CR and just consume the LF
	while((ch = w.src.get()) != '\n')
	{
		if(ch == EOF)
		{
			return PARSE_FAILURE;
		}
	}

	return PARSE_SUCCESS;
}

bool is_ows(int c)
{
	switch(c)
	{
	case ' ': case '\t':
		return true;
	default:
		return false;
	}
}

void consume_ows(std::istream& src)
{
	int ch;
	do
	{
		ch = src.get();
	} while(is_ows(ch));
	
	if(ch != EOF)
	{
		src.putback(ch);
	}
}

parse_status parse_header_field_value(std::istream& src, std::string& value)
{
	int ch = src.get();

	while(std::isgraph(ch))
	{
		value.push_back(ch);
		
		while(is_ows(ch = src.get()))
		{
			value.push_back(ch);
		}
	}

	if(ch != EOF)
	{
		src.putback(ch);
	}

	return PARSE_SUCCESS;
}

parse_status parse_header_field(wrapper& w)
{
	int ch;
	std::string tempstr;
	
	parse_status result = parse_token(w.src, tempstr);
	if(result != PARSE_SUCCESS)
	{
		return PARSE_FAILURE;
	}

	auto h_it = w.req.header_fields.emplace(std::move(tempstr), "").first;
		
	ch = w.src.get();
	if(ch != ':')
	{
		return PARSE_FAILURE;
	}
	consume_ows(w.src);
	
	result = parse_header_field_value(w.src, h_it->second);
	if(result != PARSE_SUCCESS)
	{
		return result;
	}

	while((ch = w.src.get()) != '\n')
	{
		if(ch == EOF)
		{
			return PARSE_FAILURE;
		}
	}

	return PARSE_SUCCESS;
}

parse_status parse_header_fields(wrapper& w)
{
	int ch;

	w.req.header_fields.clear();

	while((ch = w.src.get()) != '\n')
	{
		if(ch == EOF)
		{
			return PARSE_FAILURE;
		}
		else if(ch == '\r')
		{
			ch = w.src.get();
			if(ch != '\n')
			{
				return PARSE_FAILURE;
			}
			break;
		}
		w.src.putback(ch);

		parse_status result = parse_header_field(w);
		if(result != PARSE_SUCCESS)
		{
			return result;
		}
	}

	return PARSE_SUCCESS;
}

} // anonymous namespace

parse_status parse_request(std::istream& src, request& req)
{
	wrapper w = {src, req};

	try
	{
		parse_status result = parse_request_line(w);
		if(result != PARSE_SUCCESS)
		{
			return result;
		}

		result = parse_header_fields(w);
		if(result != PARSE_SUCCESS)
		{
			return result;
		}
	}
	catch(std::bad_alloc& e)
	{
		return PARSE_FAILURE;
	}

	return PARSE_SUCCESS;
}

} // namespace http
