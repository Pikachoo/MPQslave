#ifndef MPQ_HPP_
#define MPQ_HPP_


#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include <sys/types.h>	// DIR type support
#include <dirent.h>		//

#include "StormLib.h"	// -lstorm

class MPQ
{
	private:


	public:
		static bool extract_file(const char *file_name);
		static void	writeW3MMD(std::ofstream &fout, char *array, int n);

		void processList(std::string fileName, char *array, int n);
		std::istream & gettrimline(std::istream &is, std::string &str);
		void mkpath(const char *path);
		void addLoadFileAuto(std::string fileName);
		void addLoadFile(std::string fileName);
		int main2(const char *from, const char *to);
};

#endif /* MPQ_HPP_ */
