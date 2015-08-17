#include <iostream>

#include <boost/variant/static_visitor.hpp>

#include "http/parse_request.hpp"

using namespace std;
using namespace http;

int main()
{
	request req;
	parse_status result = parse_request(cin, req);
	if(result != PARSE_SUCCESS)
	{
		cerr << "Could not parse input." << endl;
	}

	cout << "Request method: ";
	switch(req.method)
	{
	case request_method::GET:
		cout << "GET\n";
		break;
	case request_method::HEAD:
		cout << "HEAD\n";
		break;
	case request_method::POST:
		cout << "POST\n";
		break;
	case request_method::PUT:
		cout << "PUT\n";
		break;
	case request_method::DELETE:
		cout << "DELETE\n";
		break;
	case request_method::CONNECT:
		cout << "CONNECT\n";
		break;
	case request_method::OPTIONS:
		cout << "OPTIONS\n";
		break;
	case request_method::TRACE:
		cout << "TRACE\n";
		break;
	default:
		cout << "Unknown\n";
	}

	cout << "Target: ";
		
	struct print_target_visitor : public boost::static_visitor<>
	{
		void operator()(const request_target_origin_form& of) const
		{
			cout << of.absolute_path;
			if(of.query)
			{
				cout << *of.query;
			}
		}
		void operator()(const request_target_absolute_form& af) const
		{
			cout << "scheme " << af.scheme << ", host " << af.authority << ", path ";
			(*this)(af.origin_part);
		}
		void operator()(const request_target_authority_form& af) const
		{
			cout << af.authority;
		}
		void operator()(const request_target_asterisk_form& af) const
		{
			cout << "*";
		}
	};

	boost::apply_visitor(print_target_visitor(), req.target);
	cout << "\n";

	for(auto& entry : req.header_fields)
	{
		cout << entry.first << ": " << entry.second << endl;
	}
}
