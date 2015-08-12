/* Copyright (c) 2015 Collin Baker
 *
 * This file is licensed under the MIT license
 * http://opensource.org/licenses/MIT
 */

#ifndef _HTTP_PARSE_REQUEST_HPP_
#define _HTTP_PARSE_REQUEST_HPP_

#include <iostream>
#include "request.hpp"

namespace http
{

enum parse_status
{
	PARSE_SUCCESS = 0,
	PARSE_FAILURE
};

parse_status parse_request(std::istream& src, request& req);

} // namespace http

#endif // _HTTP_PARSE_REQUEST_HPP_
