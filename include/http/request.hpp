#ifndef _HTTP_REQUEST_HPP_
#define _HTTP_REQUEST_HPP_

#include <map>
#include <string>

#include <boost/variant.hpp>
#include <boost/optional.hpp>

namespace http
{

enum class request_method
{
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE
};

struct request_target_origin_form
{
	std::string absolute_path;
	boost::optional<std::string> query;
};

struct request_target_absolute_form
{
	std::string scheme;
	std::string authority;
	request_target_origin_form origin_part;
};

struct request_target_authority_form
{
	std::string authority;
};

struct request_target_asterisk_form
{
	// Just used as a tag
};

using request_target = boost::variant<request_target_origin_form, request_target_absolute_form, request_target_authority_form, request_target_asterisk_form>;

struct request
{
	request_method method;
	request_target target;
	std::map<std::string, std::string> header_fields;
};

}

#endif // _HTTP_REQUEST_HPP_
