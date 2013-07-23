#include "main.hpp"

MPQs::blp blp;
MPQs::unpacker unpacker("/home/look/workspace/Maps/tmp", "", false);

MPQs::w3i w3i;

int main()
{
//	parse("/home/look/workspace/Maps", ".w3x");
//	parse("/home/look/workspace/Maps", ".w3m");
//	parse("/home/look/workspace/Maps", ".w3n");
//
//	unpacker.parse("/home/look/workspace/Maps/(8)ShamrockReef.w3x", "/home/look/workspace/Maps/tmp");
//
//	parse_blp("/home/look/workspace/Maps/tmp", ".blp");

//	parse("/home/look/workspace/Maps/tmp", ".w3i");

	w3i.read("/home/look/workspace/Maps/tmp/war3map.w3i");
	w3i.read_it();

}

void parse(const std::string &path, const std::string &extension)
{
	std::vector<std::string> maps = MPQs::IO::scan_extensions(path, extension);

	std::cout << "Найдено [" << maps.size() << "] файлов " << extension << std::endl;

	for (std::vector<std::string>::iterator it = maps.begin(), it_end = maps.end(); it != it_end; ++it)
	{
		std::cout << *it << std::endl;
	}
}

void parse_blp(const std::string &path, const std::string &extension)
{
	std::vector<std::string> images = MPQs::IO::scan_extensions(path, extension);

	std::cout << "Найдено [" << images.size() << "] файлов " << extension << std::endl;

	for (std::vector<std::string>::iterator it = images.begin(), it_end = images.end(); it != it_end; ++it)
	{
		blp.parse(*it);
	}
}
