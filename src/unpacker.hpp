#ifndef UNPACKER_HPP_
#define UNPACKER_HPP_

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

#include <sys/types.h>	// DIR type support
#include <dirent.h>		//

#include "StormLib.h"	// -lstorm

namespace MPQs
{
	class unpacker
	{
		private:
			std::map<std::string, bool> loadedFiles;
			std::vector<std::string> filesToLoad;

			bool DEBUG;
			std::string workingDirectory;
			//bool antivirus;					//whether to do "antivirus" checks on MPQ while extracting
			std::string mpqHeaderCopy;		//filename MPQ to copy header data from when creating MPQ
			bool compactMPQ;				//whether to compact when creating MPQ
			bool compress;					//whether to compress when creating MPQ
			bool insertW3MMD;				//whether to insert W3MMD code into JASS file
			bool noWrite;					//whether to suppress all file output while extracting
			bool searchFiles;				//whether to automatically search for files outside of list files while extracting

			typedef struct stat Stat;

		public:
			unpacker();

			void replaceAll(std::string& str, const std::string& from, const std::string& to);
			static inline std::string &ltrim(std::string &s);
			static inline std::string &rtrim(std::string &s);
			static inline std::string &trim(std::string &s);
			static bool invalidChar(char c);
			void stripUnicode(std::string & str);

			#if defined(_WIN32) || defined(_WIN64)
				void rdirfiles(std::string path, std::vector<std::string>& files);
			#else
				void rdirfiles(std::string dir, std::vector<std::string> &files, std::string prefix = "");
			#endif

			void do_mkdir(const char *path);
			void mkpath(const char *path);
			void getStringStream(char *array, int n, std::stringstream &ss, bool removeInvalid = false);
			std::istream &gettrimline(std::istream &is, std::string &str);
			void addLoadFile(std::string fileName);
			void addLoadFileAuto(std::string fileName);
			void addDefaultLoadFiles();
			void processList(std::string fileName, char *array, int n);
			bool loadListFile(std::string fileName);
			void writeW3MMD(std::ofstream &fout, char *array, int n);
			void saveFile(std::string fileName, char *array, int n);
			bool loadMPQ(std::string fileName);
			bool makeMPQ(std::string fileName);
			int main2(const char *from, const char *to);
	};
}
#endif /* UNPACKER_HPP_ */
