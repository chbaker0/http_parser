#include <fstream>
#include <iostream>

#include <boost/variant/static_visitor.hpp>

#include "http/parse_request.hpp"

using namespace std;

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		cerr << "Not enough arguments; call as" << endl;
		cerr << "\t" << argv[0] << ' ' << "<file-to-parse>" << endl;
		return 1;
	}

	for(unsigned int i = 0; i < 100000; ++i)
	{
		ifstream infile(argv[1]);
		if(!infile)
		{
			cerr << "Could not open file " << argv[1] << "." << endl;
			return 1;
		}

		http::request req;
		http::parse_status result = http::parse_request(infile, req);
		if(result != http::PARSE_SUCCESS)
		{
			cerr << "Could not parse." << endl;
			return 1;
		}
	}
	return 0;
}
