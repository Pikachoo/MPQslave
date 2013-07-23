#ifndef IO_HPP_
#define IO_HPP_

#include <string>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>	// -lboost_system -lboost_filesystem
#include <boost/regex.hpp>		// -lboost_regex

namespace MPQs
{
	class IO
	{
		private:

		public:
			static std::vector<std::string> scan_extensions(const std::string &scandir_path, const std::string &match);
			static std::string get_parent_dir(const std::string &file_name);
	};
}

#endif /* IO_HPP_ */
