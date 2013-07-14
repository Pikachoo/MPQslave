#include "IO.hpp"

std::vector<std::string> & IO::scan_extensions(const std::string &scandir_path, const std::string &match)
{
	_file_list.clear();

	for (boost::filesystem::directory_iterator it(scandir_path), end_itr; it != end_itr; ++it)
	{
		std::string extension = it->path().extension().c_str();
		boost::algorithm::to_lower(extension);

		if (!boost::filesystem::is_regular_file(it->status())) continue;	// Not a file
		if (extension != match) continue;									// Wrong match

		_file_list.push_back(it->path().string());							// Match
	}

	return _file_list;
}
