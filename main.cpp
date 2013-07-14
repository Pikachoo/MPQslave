#include "main.hpp"

IO IO;

MPQs::unpacker unpacker("/home/look/workspace/Maps/tmp", "", false);

int main()
{
	parse("/home/look/workspace/Maps", ".w3x");
	parse("/home/look/workspace/Maps", ".w3m");

	unpacker.parse("/home/look/workspace/Maps/Download/Battleships Crossfire4.60.w3x", "/home/look/workspace/Maps/tmp");

	parse_blp_main("/home/look/workspace/Maps/tmp", ".blp");
}

void parse(const std::string &path, const std::string &extension)
{
	std::vector<std::string> maps = IO.scan_extensions(path, extension);

	std::cout << "Найдено [" << maps.size() << "] файлов " << extension << std::endl;

	for (std::vector<std::string>::iterator it = maps.begin(), it_end = maps.end(); it != it_end; ++it)
	{
		std::cout << *it << std::endl;
	}
}

void parse_blp_main(const std::string &path, const std::string &extension)
{
	std::vector<std::string> images = IO.scan_extensions(path, extension);

	std::cout << "Найдено [" << images.size() << "] файлов " << extension << std::endl;

	for (std::vector<std::string>::iterator it = images.begin(), it_end = images.end(); it != it_end; ++it)
	{
		blp_parse(*it);
	}
}
