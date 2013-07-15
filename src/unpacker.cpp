#include "unpacker.hpp"

MPQs::unpacker::unpacker(const std::string dir_out,
						 const std::string header_src,

						 bool is_debug,
						 bool is_compact,
						 bool is_compress,
						 bool required_W3MMD,
						 bool required_search,
						 bool dont_save)
{
	_working_dir = dir_out;				// working directory
	_header_src = header_src;			// file_name MPQ to copy header data from when creating MPQ
	_is_debug = is_debug;				// whether to debug when running
	_is_compact = is_compact;			// whether to compact when creating MPQ
	_is_compress = is_compress;			// whether to compress when creating MPQ
	_required_W3MMD = required_W3MMD;	// whether to insert W3MMD code into JASS file
	_required_search = required_search;	// whether to automatically search for files outside of list files while extracting
	_dont_save = dont_save;				// whether to suppress all file output while extracting
}

//string replace all
void MPQs::unpacker::replace_all(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;

	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

// trim from start
inline std::string & MPQs::unpacker::ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
inline std::string & MPQs::unpacker::rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
inline std::string & MPQs::unpacker::trim(std::string &s)
{
	return ltrim(rtrim(s));
}

bool MPQs::unpacker::invalid_char(char c)
{
	return !(c >= 32 && c <= 126);
}

//removes bad characters from file_name
void MPQs::unpacker::strip_unicode(std::string &str)
{
	str.erase(remove_if(str.begin(), str.end(), &MPQs::unpacker::invalid_char), str.end());
}

#if defined(_WIN32) || defined(_WIN64)
void MPQs::unpacker::rdirfiles(std::string path, std::vector<std::string>& files)
{
	if(!path.empty() && path[path.length() - 1] == '/')
	{
		path = path.substr(0, path.length() - 1);
	}

	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	std::string spec;
	stack<std::string> directories;

	directories.push(path);
	files.clear();

	while (!directories.empty())
	{
		path = directories.top();
		spec = path + "\\*";
		directories.pop();

		hFind = FindFirstFile(spec.c_str(), &ffd);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			return;
		}

		do
		{
			if (strcmp(ffd.cfile_name, ".") != 0 &&
					strcmp(ffd.cfile_name, "..") != 0)
			{
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					directories.push(path + "\\" );
					files.push_back(string(ffd.cfile_name) + "/");
				}
				else
				{
					files.push_back(string(ffd.cfile_name));
				}
			}
		}while (FindNextFile(hFind, &ffd) != 0);

		if (GetLastError() != ERROR_NO_MORE_FILES)
		{
			FindClose(hFind);
			return;
		}

		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}
}
#else
//recursively retrieves files from directory
//dir is the directory that we started searching, while prefix is the parent directories in recursion
void MPQs::unpacker::rdirfiles(std::string dir, std::vector<std::string> &files, std::string prefix)
{
	//add slash if needed
	if (!dir.empty() && dir[dir.length() - 1] != '/')
	{
		dir += "/";
	}

	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir((dir + prefix).c_str())) == NULL)
	{
		return;
	}

	while ((dirp = readdir(dp)) != NULL)
	{
		if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
		{
			Stat st;
			stat((dir + prefix + std::string(dirp->d_name)).c_str(), &st);
			if (st.st_mode & S_IFDIR)
			{
				files.push_back(prefix + std::string(dirp->d_name) + "/");
				rdirfiles(dir, files, prefix + std::string(dirp->d_name) + "/");
			}
			else if (st.st_mode & S_IFREG)
			{
				files.push_back(prefix + std::string(dirp->d_name));
			}
		}
	}
	closedir(dp);
}
#endif

void MPQs::unpacker::do_mkdir(const char *path)
{
#if defined(_WIN32) || defined(_WIN64)
	if ((GetFileAttributes(path)) == INVALID_FILE_ATTRIBUTES)
	{
		_mkdir(path);
	}
#else
	Stat st;

	if (stat(path, &st) != 0)
	{
		mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
#endif
}

/**
 ** mkpath - ensure all directories in path exist
 ** Algorithm takes the pessimistic view and works top-down to ensure
 ** each directory in path exists, rather than optimistically creating
 ** the last element and working backwards.
 */
void MPQs::unpacker::mkpath(const char *path)
{
	char *pp;
	char *sp;
	char *copypath = strdup(path);

	pp = copypath;
	while ((sp = strchr(pp, '/')) != 0)
	{
		if (sp != pp)
		{
			/* Neither root nor double slash in path */
			*sp = '\0';
			do_mkdir(copypath);
			*sp = '/';
		}
		pp = sp + 1;
	}

	delete[] copypath;
}

void MPQs::unpacker::get_string_stream(char *array, int n, std::stringstream &ss, bool removeInvalid)
{
	std::string str(array, n);

	//change all \r to \n, but don't add \r\r
	replace_all(str, "\r\n", "\r");
	replace(str.begin(), str.end(), '\r', '\n');

	if (removeInvalid)
	{
		//change invalid unicode characters to \n for parsing
		for (int i = 0; i < 32; i++)
		{
			replace(str.begin(), str.end(), (char) i, '\n');
		}

		replace(str.begin(), str.end(), (char) 127, '\n');
	}

	//set the string for stringstream
	ss.str(str);
}

std::istream & MPQs::unpacker::gettrimline(std::istream &is, std::string &str)
{
	std::istream &ret = getline(is, str);
	strip_unicode(str);
	return ret;
}

void MPQs::unpacker::add_load_file(std::string file_name)
{
	strip_unicode(file_name);
	file_name = trim(file_name);

	std::string lower_name = file_name;
	transform(lower_name.begin(), lower_name.end(), lower_name.begin(), (int (*)(int))tolower);

	//make sure we don't already have this file
	if (_files_loaded.find(lower_name) == _files_loaded.end())
	{
		_files_to_load.push_back(file_name);
		_files_loaded[lower_name] = true;

		if (_is_debug)
			std::cout << "Adding file [" << file_name << "]" << std::endl;
	}
}

//adds file but also autodetects other files based on it
void MPQs::unpacker::add_load_file_auto(std::string file_name)
{
	if (file_name.length() > 300					||
		file_name.find('\"') != std::string::npos	||
		file_name.find(';')  != std::string::npos	||
		file_name.find('\'') != std::string::npos	||
		file_name.find('[')  != std::string::npos	||
		file_name.find(']')  != std::string::npos)
	{
		return;
	}

	//replace two backslashes with one backslash
	replace_all(file_name, "\\\\", "\\");
	replace_all(file_name, "/", "\\");

	add_load_file(file_name);

	//remove index
	std::string file_name_without_extension = file_name;
	size_t index = file_name.rfind('.');

	if (index != std::string::npos)
	{
		file_name_without_extension = file_name.substr(0, index);
	}

	add_load_file(file_name_without_extension + ".blp");
	add_load_file(file_name_without_extension + ".tga");
	add_load_file(file_name_without_extension + ".mdx");
	add_load_file(file_name_without_extension + ".mdl");
	add_load_file(file_name_without_extension + ".mp3");
	add_load_file(file_name_without_extension + ".wav");

	index = file_name_without_extension.rfind('\\');
	std::string base_name = file_name_without_extension;

	if (index != std::string::npos && file_name.length() >= index)
	{
		base_name = file_name_without_extension.substr(index + 1);
	}

	add_load_file("ReplaceableTextures\\CommandButtonsDisabled\\DIS" + base_name + ".blp");
	add_load_file("ReplaceableTextures\\CommandButtonsDisabled\\DIS" + base_name + ".tga");
	add_load_file("ReplaceableTextures\\CommandButtonsDisabled\\DISBTN" + base_name + ".blp");
	add_load_file("ReplaceableTextures\\CommandButtonsDisabled\\DISBTN" + base_name + ".tga");
}

void MPQs::unpacker::add_load_file_default()
{
	add_load_file("war3map.j");
	add_load_file("Scripts\\war3map.j");
	add_load_file("war3map.w3e");
	add_load_file("war3map.wpm");
	add_load_file("war3map.doo");
	add_load_file("war3map.w3u");
	add_load_file("war3map.w3b");
	add_load_file("war3map.w3d");
	add_load_file("war3map.w3a");
	add_load_file("war3map.w3q");
	add_load_file("war3map.w3u");
	add_load_file("war3map.w3t");
	add_load_file("war3map.w3d");
	add_load_file("war3map.w3h");
	add_load_file("(listfile)");
}

void MPQs::unpacker::list_process(std::string file_name, char *array, int n)
{
	size_t index = file_name.rfind('.');
	std::string extension = "";

	if (index != std::string::npos)
	{
		extension = file_name.substr(index + 1);
	}

	if (file_name == "(listfile)")
	{
		std::stringstream ss;
		get_string_stream(array, n, ss);
		std::string str;

		while (getline(ss, str))
		{
			add_load_file(str);
		}
	}
	else if (extension == "txt" || extension == "slk")
	{
		std::stringstream ss;
		get_string_stream(array, n, ss);
		std::string str;

		while (getline(ss, str))
		{
			if (str.length() < 300)
			{
				add_load_file_auto(str);

				size_t quote_first = str.rfind('\"');

				if (quote_first != std::string::npos)
				{
					std::string sub = str.substr(0, quote_first);
					size_t quote_second = sub.rfind('\"');

					if (quote_second != std::string::npos && quote_second < sub.length())
					{
						add_load_file_auto(sub.substr(quote_second + 1));
					}
				}

				size_t equalIndex = str.rfind('=');

				if (equalIndex != std::string::npos && equalIndex < str.length())
				{
					add_load_file_auto(str.substr(equalIndex + 1));
				}
			}
		}
	}
	else if (extension == "w3u" ||
			 extension == "w3t" ||
			 extension == "w3b" ||
			 extension == "w3d" ||
			 extension == "w3a" ||
			 extension == "w3h" ||
			 extension == "w3q" ||
			 extension == "mdx" ||
			 extension == "w3i")
	{
		std::stringstream ss;
		get_string_stream(array, n, ss, true);
		std::string str;

		while (getline(ss, str))
		{
			add_load_file_auto(str);
		}
	}
	else if (extension == "j")
	{
		std::stringstream ss;
		get_string_stream(array, n, ss);
		std::string str;

		while (getline(ss, str))
		{
			size_t quote_first;

			while ((quote_first = str.rfind('\"')) != std::string::npos)
			{
				str = str.substr(0, quote_first);
				size_t quote_second = str.rfind('\"');

				if (quote_second != std::string::npos && quote_second < str.length())
				{
					add_load_file_auto(str.substr(quote_second + 1));
					str = str.substr(0, quote_second);
				}
				else
				{
					break;
				}
			}
		}
	}
}

bool MPQs::unpacker::list_load(std::string file_name)
{
	if (_is_debug)
		std::cout << "Loading list file [" << file_name << "]" << std::endl;

	std::ifstream fin(file_name.c_str());

	if (!fin.is_open())
	{
		if (_is_debug)
			std::cerr << "Warning: failed to open list file; only default files will be loaded" << std::endl;

		return false;
	}

	std::string str;

	while (getline(fin, str))
	{
		add_load_file(str);
	}

	return true;
}

void MPQs::unpacker::write_W3MMD(std::ofstream &fout, char *array, int n)
{
	//ss is constructed from array, which contains the JASS script
	std::stringstream ss;
	get_string_stream(array, n, ss);

	//open up input stream from w3mmd.txt that tells us how to insert W3MMD code
	std::ifstream fin("w3mmd.txt");

	if (!fin.is_open())
	{
		if (_is_debug)
			std::cerr << "Warning: failed to open w3mmd.txt for W3MMD insertion" << std::endl;

		return;
	}

	std::string str;
	bool start_output; //whether first line detect from w3mmd.txt was reached

	//loop on the W3MMD file
	while (gettrimline(fin, str))
	{
		//ignore comments
		if (!str.empty() && str[0] == '#')
		{
			continue;
		}

		//colons indicate to wait until a certain line is arrived at
		if (str[0] == ':')
		{
			std::string lineDetect = str.substr(1);

			//don't need str anymore so reuse
			//read from JASS script until we reach the line
			while (gettrimline(ss, str) && str != lineDetect)
			{
				fout << str << std::endl;
			}

			//if we have EOF, print warning
			//otherwise write the last line
			if (ss.eof())
			{
				if (_is_debug)
					std::cerr << "Warning: EOF reached before [" << lineDetect << "] line was reached" << std::endl;
			}
			else
			{
				fout << str << std::endl;
			}

			//now continue reading W3MMD from loop
			start_output = true;
		}
		else if (start_output)
		{
			fout << str << std::endl;
		}
	}

	//write the remainder of JASS script out
	while (gettrimline(ss, str))
	{
		fout << str << std::endl;
	}
}

void MPQs::unpacker::file_save(std::string file_name, char *array, int n)
{
	if (_required_search)
	{
		//process the file to search for more files
		list_process(file_name, array, n);
	}

	replace(file_name.begin(), file_name.end(), '\\', '/');
	file_name = _working_dir + file_name;

	//make directories if needed
	std::string dir_name = file_name;
	size_t index = file_name.rfind('/');

	if (index != std::string::npos)
	{
		dir_name = file_name.substr(0, index + 1);
	}

	mkpath(dir_name.c_str());

	//save the file, but only if _dont_save is not enabled
	if (!_dont_save)
	{
		std::ofstream fout(file_name.c_str());

		//if JASS, insert W3MMD if requested
		if (_required_W3MMD && file_name.length() >= 9 && file_name.substr(file_name.length() - 9) == "war3map.j")
		{
			write_W3MMD(fout, array, n);
		}
		else
		{
			if (fout.is_open())
			{
				fout.write(array, n);
				fout.close();
			}
			else
			{
				if (_is_debug)
					std::cerr << "Warning: failed to save file [" << file_name << "]" << std::endl;
			}
		}
	}
}

bool MPQs::unpacker::MPQ_load(std::string file_name)
{
	HANDLE MapMPQ;

	if (SFileOpenArchive(file_name.c_str(), 0, MPQ_OPEN_FORCE_MPQ_V1 | STREAM_FLAG_READ_ONLY, &MapMPQ))
	{
		if (_is_debug)
			std::cout << "Loading MPQ [" << file_name << "]" << std::endl;
	}
	else
	{
		if (_is_debug)
			std::cerr << "Error: unable to load MPQ file [" << file_name << "]: " << GetLastError() << std::endl;

		return false;
	}

	for (unsigned int i = 0; i < _files_to_load.size(); i++)
	{
		std::string file_current = _files_to_load[i];

		if (_is_debug)
			std::cout << "Loading file [" << file_current << "]" << std::endl;

		HANDLE SubFile;

		if (SFileOpenFileEx(MapMPQ, file_current.c_str(), 0, &SubFile))
		{
			if (_is_debug)
				std::cout << "Found file [" << file_current << "]" << std::endl;

			unsigned int FileLength = SFileGetFileSize(SubFile, NULL);

			if (FileLength > 0 && FileLength != 0xFFFFFFFF)
			{
				char *SubFileData = new char[FileLength];
				DWORD BytesRead = 0;

				if (SFileReadFile(SubFile, SubFileData, FileLength, &BytesRead, NULL))
				{
					//since it succeeded, FileLength should equal BytesRead
					file_save(file_current, SubFileData, BytesRead);
				}

				delete[] SubFileData;
			}

			SFileCloseFile(SubFile);
		}
	}

	SFileCloseArchive(MapMPQ);

	return true;
}

bool MPQs::unpacker::MPQ_make(std::string file_name)
{
	std::vector<std::string> files;
	rdirfiles(_working_dir, files);

	HANDLE MapMPQ;

	if (SFileCreateArchive(file_name.c_str(), MPQ_CREATE_ARCHIVE_V1, files.size() + 15, &MapMPQ))
	{
		if (_is_debug)
			std::cout << "Creating MPQ [" << file_name << "]" << std::endl;
	}
	else
	{
		if (_is_debug)
			std::cerr << "Error: unable to create MPQ file [" << file_name << "]: " << GetLastError() << std::endl;

		return false;
	}

	//parameters for adding file to archive
	int dwFlags = 0;
	int dwCompression = 0;
	int dwCompressionNext = 0;

	if (_is_compress)
	{
		dwFlags = MPQ_FILE_COMPRESS;
		dwCompression = MPQ_COMPRESSION_ZLIB;
		dwCompressionNext = MPQ_COMPRESSION_ZLIB;
	}

	for (unsigned int i = 0; i < files.size(); i++)
	{
		std::string file_current = files[i];

		if (file_current.empty())
			continue;

		if (file_current[file_current.length() - 1] == '/')
		{
			if (_is_debug)
				std::cout << "Ignoring directory [" << file_current << "]" << std::endl;
		}
		else if (file_current == "(listfile)")
		{
			if (_is_debug)
				std::cout << "Ignoring listfile [" << file_current << "]" << std::endl;
		}
		else
		{
			if (_is_debug)
				std::cout << "Adding file [" << file_current << "]" << std::endl;

			//change to backslash that MPQ uses for the file_name in MPQ
			std::string mpqFile = file_current;
			replace(mpqFile.begin(), mpqFile.end(), '/', '\\');

			if (!SFileAddFileEx(MapMPQ, (_working_dir + file_current).c_str(), mpqFile.c_str(), dwFlags, dwCompression,
					dwCompressionNext))
			{
				if (_is_debug)
					std::cerr << "Warning: failed to add " << file_current << std::endl;
			}
		}
	}

	if (_is_compact)
	{
		if (!SFileCompactArchive(MapMPQ, NULL, 0))
		{
			if (_is_debug)
				std::cerr << "Warning: failed to compact archive: " << GetLastError() << std::endl;
		}
	}

	SFileCloseArchive(MapMPQ);

	if (!_header_src.empty())
	{
		if (_is_debug)
			std::cout << "Prepending header from [" << _header_src << "] to [" << file_name << "]" << std::endl;

		//copy header to the mpq, use temporary file with _ prepended
		std::string tempfile_name = "_" + file_name;

		//read header first
		std::ifstream fin(_header_src.c_str());

		if (!fin.is_open())
		{
			if (_is_debug)
				std::cerr << "Error: failed to copy header data: error while reading [" << _header_src << "]" << std::endl;

			return false;
		}

		char *buffer = new char[512];
		fin.read(buffer, 512);

		if (fin.fail())
		{
			if (_is_debug)
				std::cerr << "Warning: header data could not be completely read" << std::endl;
		}

		fin.close();

		std::ofstream fout(tempfile_name.c_str());

		if (!fout.is_open())
		{
			if (_is_debug)
				std::cerr << "Error: failed to copy header data: error while writing to [" << tempfile_name << "]" << std::endl;

			delete[] buffer;
			return false;
		}

		fout.write(buffer, 512);

		//now add the actual MPQ data, use same buffer
		std::ifstream finMPQ(file_name.c_str());

		if (!finMPQ.is_open())
		{
			if (_is_debug)
				std::cerr << "Error: failed to prepend header to MPQ: error while reading MPQ [" << file_name << "]" << std::endl;

			delete[] buffer;
			return false;
		}

		while (!finMPQ.eof())
		{
			finMPQ.read(buffer, 512);
			fout.write(buffer, finMPQ.gcount()); //use gcount in case less than 512 bytes were read
		}

		delete[] buffer;
		fout.close();
		finMPQ.close();

		if (rename(tempfile_name.c_str(), file_name.c_str()) != 0)
		{
			if (_is_debug)
				std::cerr << "Error while renaming [" << tempfile_name << "] to [" << file_name << "]" << std::endl;

			return false;
		}
	}

	return true;
}

void MPQs::unpacker::parse(const char *from, const char *to)
{
	add_load_file_default();

	_working_dir = to;

	if (!_working_dir.empty() && _working_dir[_working_dir.length() - 1] != '/')
	{
		_working_dir = _working_dir + "/";
	}

	list_load(from);

	MPQ_load(from);
}
