#include "parse_request.hpp"

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

parse_status parse_request_target_origin_form(std::istream& src, request_target_origin_form& of)
{
	int ch;
	std::string tempstr;

	ch = src.get();
	while(ch != ' ' && ch != '?')
	{
		if(ch == EOF)
		{
			return PARSE_FAILURE;
		}
		of.absolute_path.push_back(ch);
		ch = src.get();
	}
	if(ch == '?')
	{
		while((ch = src.get()) != ' ')
		{
			if(ch == EOF)
			{
				return PARSE_FAILURE;
			}
			tempstr.push_back(ch);
		}
		of.query = std::move(tempstr);
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
		while(ch != ':')
		{
			if(ch == EOF || ch == ' ')
			{
				return PARSE_FAILURE;
			}
			af.scheme.push_back(ch);
			ch = w.src.get();
		}
		// Consume // as in http://www.example.com/
		while((ch = w.src.get()) == '/')
			;
		
		if(ch == EOF || ch == ' ')
		{
			return PARSE_FAILURE;
		}

		while((ch = w.src.get()) != '/')
		{
			if(ch == EOF || ch == ' ')
			{
				return PARSE_FAILURE;
			}
			af.host.push_back(ch);
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
	while((ch = w.src.get()) != ' ')
	{		
		if(ch == EOF)
		{
			return PARSE_FAILURE;
		}
		tempstr.push_back(ch);
	}
	// Space is consumed

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
	parse_status result = parse_request_target(w);
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

	ch = src.get();
	while(ch != EOF && is_tchar(ch))
	{
		token.push_back(ch);
		ch = src.get();
	}
	if(ch != EOF)
	{
		src.putback(ch);
	}

	return token.empty() ? PARSE_FAILURE : PARSE_SUCCESS;
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
