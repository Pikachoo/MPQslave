#include "IO.hpp"

std::vector<std::string> MPQs::IO::scan_extensions(const std::string &scandir_path, const std::string &match)
{
	std::vector<std::string> file_list;

	for (boost::filesystem::recursive_directory_iterator it(scandir_path), end_itr; it != end_itr; ++it)
	{
		std::string extension = it->path().extension().c_str();
		boost::algorithm::to_lower(extension);

		if (!boost::filesystem::is_regular_file(it->status())) continue;	// Not a file
		if (extension != match) continue;									// Wrong match

		file_list.push_back(it->path().string());							// Match
	}

	return file_list;
}

std::string MPQs::IO::get_parent_dir(const std::string &file_name)
{
	boost::filesystem::path path(file_name);

	return path.parent_path().string() + "/";
}
