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
			std::map<std::string, bool> _files_loaded;
			std::vector<std::string> _files_to_load;

			std::string _working_dir;
			std::string _header_src;			//file_name MPQ to copy header data from when creating MPQ

			bool _is_debug;						//whether to debug when running
			bool _is_compact;					//whether to compact when creating MPQ
			bool _is_compress;					//whether to compress when creating MPQ
			bool _required_W3MMD;				//whether to insert W3MMD code into JASS file
			bool _required_search;				//whether to automatically search for files outside of list files while extracting
			bool _dont_save;					//whether to suppress all file output while extracting

			typedef struct stat Stat;


			void replace_all(std::string &str, const std::string &from, const std::string &to);
			static inline std::string &ltrim(std::string &s);
			static inline std::string &rtrim(std::string &s);
			static inline std::string &trim(std::string &s);
			static bool invalid_char(char c);
			void strip_unicode(std::string & str);

			#if defined(_WIN32) || defined(_WIN64)
				void rdirfiles(std::string path, std::vector<std::string>& files);
			#else
				void rdirfiles(std::string dir, std::vector<std::string> &files, std::string prefix = "");
			#endif

			void do_mkdir(const char *path);
			void mkpath(const char *path);
			void get_string_stream(char *array, int n, std::stringstream &ss, bool removeInvalid = false);
			std::istream &gettrimline(std::istream &is, std::string &str);
			void add_load_file(std::string file_name);
			void add_load_file_auto(std::string file_name);
			void add_load_file_default();
			void list_process(std::string file_name, char *array, int n);
			bool list_load(std::string file_name);
			void write_W3MMD(std::ofstream &fout, char *array, int n);
			void file_save(std::string file_name, char *array, int n);
			bool MPQ_load(std::string file_name);
			bool MPQ_make(std::string file_name);

		public:
			unpacker(const std::string dir_out,
					 const std::string header_src = "",

					 bool is_debug			= true,
					 bool is_compact		= false,
					 bool is_compress		= false,
					 bool required_W3MMD	= false,
					 bool required_search	= true,
					 bool dont_save			= false);

			void parse(const char *from, const char *to);
	};
}
#endif /* UNPACKER_HPP_ */
