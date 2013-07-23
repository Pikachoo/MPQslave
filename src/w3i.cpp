#include "w3i.hpp"

W3I_18 MPQs::w3i::read_it() // TODO: bug is here
{
	W3I_18 W3I;

	_file.seekg(0, std::ios::beg);
	_file.read(reinterpret_cast<char*>(&W3I), sizeof(W3I));

	while (!_file.eof())
	{
		std::cout << W3I.FILE_FORMAT << std::endl;
		std::cout << W3I.NUM_OF_SAVES << std::endl;
		std::cout << W3I.EDITOR_VERSION << std::endl;
		std::cout << W3I.MAP_NAME << std::endl;
		std::cout << W3I.MAP_AUTHOR << std::endl;
		std::cout << W3I.MAP_DESCRIPTION << std::endl;
		std::cout << W3I.MAP_PLAYERS_RECOMMENDED << std::endl;

		_file.read(reinterpret_cast<char*>(&W3I), sizeof(W3I));
	}

	return W3I;
}
