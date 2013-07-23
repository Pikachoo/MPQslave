#include "readable.hpp"

MPQs::readable::readable()
{
	_file_buffer = 0;
	_file_length = 0;

	_buffer		 = 0;
}

MPQs::readable::~readable()
{
	/* ============================== *\
	 * Free buffer's allocated memory *
	\* ============================== */

	delete[] _buffer;
}

bool MPQs::readable::read(const char *file_name)
{
	_file.open(file_name, std::ios::in | std::ios::binary);

	if (_file.is_open())
	{
		/* ======================================= *\
		 * Get pointer to associated buffer object *
		\* ======================================= */

		_file_buffer = _file.rdbuf();


		/* ==================================== *\
		 * Get file size using buffer's members *
		\* ==================================== */

		_file_length = _file_buffer->pubseekoff(0, _file.end, _file.in);


		/* ===================== *\
		 * Memory (re)allocation *
		\* ===================== */

		if (_buffer)
		{
			delete[] _buffer;
		}

		_buffer = new char[_file_length];	// allocate memory to contain file data


		/* ===================== *\
		 * File read into buffer *
		\* ===================== */

		//_file.read(_buffer, _file_length);	// read binary file into buffer

		return true;
	}

	return false;
}

bool MPQs::readable::read(const std::string &file_name)
{
	return read(file_name.c_str());
}
