#ifndef READABLE_HPP_
#define READABLE_HPP_

#include <iostream>
#include <fstream>

#include <vector>
#include <string>

namespace MPQs
{
	class readable
	{
		protected:
			std::ifstream	 _file;
			std::filebuf	*_file_buffer;

			int				 _file_length;
			char			*_buffer;

		public:
			readable();
			~readable();

			bool read(const char *file_name);
			bool read(const std::string &file_name);
	};
}

#endif /* READABLE_HPP_ */
