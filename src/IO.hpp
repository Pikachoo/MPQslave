#ifndef IO_HPP_
#define IO_HPP_

#include <string>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>	// -lboost_system -lboost_filesystem
#include <boost/regex.hpp>		// -lboost_regex

class IO
{
	private:
		std::vector<std::string> _file_list;

	public:
		std::vector<std::string> & scan_extensions(const std::string &scandir_path, const std::string &match);
};


#endif /* IO_HPP_ */
