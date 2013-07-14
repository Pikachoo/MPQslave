#include "MPQ.hpp"

std::map<std::string, bool> loadedFiles;
std::vector<std::string> filesToLoad;

// CONFIGURATION (done through command line)
bool DEBUG = true;
std::string workingDirectory = "/home/look/workspace/Maps/tmp";
bool antivirus; //whether to do "antivirus" checks on MPQ while extracting
std::string mpqHeaderCopy = ""; //filename MPQ to copy header data from when creating MPQ
bool compactMPQ = false; //whether to compact when creating MPQ
bool compress = false; //whether to compress when creating MPQ
bool insertW3MMD = false; //whether to insert W3MMD code into JASS file
bool noWrite = false; //whether to suppress all file output while extracting
bool searchFiles = true; //whether to automatically search for files outside of list files while extracting

typedef struct stat Stat;

//string replace all
void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;

	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

// trim from start
static inline std::string &ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

bool invalidChar(char c)
{
	return !(c >= 32 && c <= 126);
}

//removes bad characters from filename
void stripUnicode(std::string & str)
{
	str.erase(remove_if(str.begin(), str.end(), invalidChar), str.end());
}

#if defined(_WIN32) || defined(_WIN64)
void rdirfiles(string path, vector<string>& files)
{
	if(!path.empty() && path[path.length() - 1] == '/')
	{
		path = path.substr(0, path.length() - 1);
	}

	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	string spec;
	stack<string> directories;

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
			if (strcmp(ffd.cFileName, ".") != 0 &&
					strcmp(ffd.cFileName, "..") != 0)
			{
				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					directories.push(path + "\\" );
					files.push_back(string(ffd.cFileName) + "/");
				}
				else
				{
					files.push_back(string(ffd.cFileName));
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
void rdirfiles(std::string dir, std::vector<std::string> &files, std::string prefix = "")
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

void do_mkdir(const char *path)
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
void mkpath(const char *path)
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

void getStringStream(char *array, int n, std::stringstream &ss, bool removeInvalid = false)
{
	std::string str(array, n);

	//change all \r to \n, but don't add \r\r
	replaceAll(str, "\r\n", "\r");
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

std::istream &gettrimline(std::istream &is, std::string &str)
{
	std::istream &ret = getline(is, str);
	stripUnicode(str);
	return ret;
}

void addLoadFile(std::string fileName)
{
	stripUnicode(fileName);
	fileName = trim(fileName);

	std::string lowerName = fileName;
	transform(lowerName.begin(), lowerName.end(), lowerName.begin(), (int (*)(int))tolower);

	//make sure we don't already have this file
if(	loadedFiles.find(lowerName) == loadedFiles.end())
	{
		filesToLoad.push_back(fileName);
		loadedFiles[lowerName] = true;
		if(DEBUG) std::cout << "Adding file [" << fileName << "]" << std::endl;
	}
}

//adds file but also autodetects other files based on it
void addLoadFileAuto(std::string fileName)
{
	if (fileName.length() > 300 || fileName.find('\"') != std::string::npos || fileName.find(';') != std::string::npos
			|| fileName.find('\'') != std::string::npos || fileName.find('[') != std::string::npos
			|| fileName.find(']') != std::string::npos)
	{
		return;
	}

	//replace two backslashes with one backslash
	replaceAll(fileName, "\\\\", "\\");
	replaceAll(fileName, "/", "\\");

	addLoadFile(fileName);

	//remove index
	std::string fileNameWithoutExtension = fileName;
	size_t index = fileName.rfind('.');

	if (index != std::string::npos)
	{
		fileNameWithoutExtension = fileName.substr(0, index);
	}

	addLoadFile(fileNameWithoutExtension + ".blp");
	addLoadFile(fileNameWithoutExtension + ".tga");
	addLoadFile(fileNameWithoutExtension + ".mdx");
	addLoadFile(fileNameWithoutExtension + ".mdl");
	addLoadFile(fileNameWithoutExtension + ".mp3");
	addLoadFile(fileNameWithoutExtension + ".wav");

	index = fileNameWithoutExtension.rfind('\\');
	std::string baseName = fileNameWithoutExtension;

	if (index != std::string::npos && fileName.length() >= index)
	{
		baseName = fileNameWithoutExtension.substr(index + 1);
	}

	addLoadFile("ReplaceableTextures\\CommandButtonsDisabled\\DIS" + baseName + ".blp");
	addLoadFile("ReplaceableTextures\\CommandButtonsDisabled\\DIS" + baseName + ".tga");
	addLoadFile("ReplaceableTextures\\CommandButtonsDisabled\\DISBTN" + baseName + ".blp");
	addLoadFile("ReplaceableTextures\\CommandButtonsDisabled\\DISBTN" + baseName + ".tga");
}

void addDefaultLoadFiles()
{
	addLoadFile("war3map.j");
	addLoadFile("Scripts\\war3map.j");
	addLoadFile("war3map.w3e");
	addLoadFile("war3map.wpm");
	addLoadFile("war3map.doo");
	addLoadFile("war3map.w3u");
	addLoadFile("war3map.w3b");
	addLoadFile("war3map.w3d");
	addLoadFile("war3map.w3a");
	addLoadFile("war3map.w3q");
	addLoadFile("war3map.w3u");
	addLoadFile("war3map.w3t");
	addLoadFile("war3map.w3d");
	addLoadFile("war3map.w3h");
	addLoadFile("(listfile)");
}

void processList(std::string fileName, char *array, int n)
{
	size_t index = fileName.rfind('.');
	std::string extension = "";

	if (index != std::string::npos)
	{
		extension = fileName.substr(index + 1);
	}

	if (fileName == "(listfile)")
	{
		std::stringstream ss;
		getStringStream(array, n, ss);
		std::string str;

		while (getline(ss, str))
		{
			addLoadFile(str);
		}
	}
	else if (extension == "txt" || extension == "slk")
	{
		std::stringstream ss;
		getStringStream(array, n, ss);
		std::string str;

		while (getline(ss, str))
		{
			if (str.length() < 300)
			{
				addLoadFileAuto(str);

				size_t firstQuote = str.rfind('\"');

				if (firstQuote != std::string::npos)
				{
					std::string sub = str.substr(0, firstQuote);
					size_t secondQuote = sub.rfind('\"');

					if (secondQuote != std::string::npos && secondQuote < sub.length())
					{
						addLoadFileAuto(sub.substr(secondQuote + 1));
					}
				}

				size_t equalIndex = str.rfind('=');

				if (equalIndex != std::string::npos && equalIndex < str.length())
				{
					addLoadFileAuto(str.substr(equalIndex + 1));
				}
			}
		}
	}
	else if (extension == "w3u" || extension == "w3t" || extension == "w3b" || extension == "w3d" || extension == "w3a"
			|| extension == "w3h" || extension == "w3q" || extension == "mdx" || extension == "w3i")
	{
		std::stringstream ss;
		getStringStream(array, n, ss, true);
		std::string str;

		while (getline(ss, str))
		{
			addLoadFileAuto(str);
		}
	}
	else if (extension == "j")
	{
		std::stringstream ss;
		getStringStream(array, n, ss);
		std::string str;

		while (getline(ss, str))
		{
			size_t firstQuote;

			while ((firstQuote = str.rfind('\"')) != std::string::npos)
			{
				str = str.substr(0, firstQuote);
				size_t secondQuote = str.rfind('\"');

				if (secondQuote != std::string::npos && secondQuote < str.length())
				{
					addLoadFileAuto(str.substr(secondQuote + 1));
					str = str.substr(0, secondQuote);
				}
				else
				{
					break;
				}
			}
		}
	}
}

bool loadListFile(std::string fileName)
{
	std::cout << "Loading list file [" << fileName << "]" << std::endl;

	std::ifstream fin(fileName.c_str());

	if (!fin.is_open())
	{
		std::cerr << "Warning: failed to open list file; only default files will be loaded" << std::endl;
		return false;
	}

	std::string str;

	while (getline(fin, str))
	{
		addLoadFile(str);
	}

	return true;
}

void writeW3MMD(std::ofstream &fout, char *array, int n)
{
	//ss is constructed from array, which contains the JASS script
	std::stringstream ss;
	getStringStream(array, n, ss);

	//open up input stream from w3mmd.txt that tells us how to insert W3MMD code
	std::ifstream fin("w3mmd.txt");

	if (!fin.is_open())
	{
		std::cerr << "Warning: failed to open w3mmd.txt for W3MMD insertion" << std::endl;
		return;
	}

	std::string str;
	bool startOutput; //whether first line detect from w3mmd.txt was reached

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
				std::cerr << "Warning: EOF reached before [" << lineDetect << "] line was reached" << std::endl;
			}
			else
			{
				fout << str << std::endl;
			}

			//now continue reading W3MMD from loop
			startOutput = true;
		}
		else if (startOutput)
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

void saveFile(std::string fileName, char *array, int n)
{
	if (searchFiles)
	{
		//process the file to search for more files
		processList(fileName, array, n);
	}

	replace(fileName.begin(), fileName.end(), '\\', '/');
	fileName = workingDirectory + fileName;

	//make directories if needed
	std::string directoryName = fileName;
	size_t index = fileName.rfind('/');

	if (index != std::string::npos)
	{
		directoryName = fileName.substr(0, index + 1);
	}

	mkpath(directoryName.c_str());

	//run antivirus check here on JASS
	//if we do in processList it won't be done if search is disabled
	//if we do in insertW3MMD then write may be disabled and it won't work
	if (antivirus && fileName.length() >= 9 && fileName.substr(fileName.length() - 9) == "war3map.j")
	{
		std::stringstream ss;
		getStringStream(array, n, ss);
		std::string str;

		while (getline(ss, str))
		{
			//check for preload exploit in map
			if (str.find("PreloadGenEnd") != std::string::npos)
			{
				std::cout << "Virus detected in map: " << str << std::endl;
			}
		}
	}

	//save the file, but only if noWrite is not enabled
	if (!noWrite)
	{
		std::ofstream fout(fileName.c_str());

		//if JASS, insert W3MMD if requested
		if (insertW3MMD && fileName.length() >= 9 && fileName.substr(fileName.length() - 9) == "war3map.j")
		{
			writeW3MMD(fout, array, n);
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
				std::cerr << "Warning: failed to save file [" << fileName << "]" << std::endl;
			}
		}
	}
}

bool loadMPQ(std::string fileName)
{
	HANDLE MapMPQ;

	if (SFileOpenArchive(fileName.c_str(), 0, MPQ_OPEN_FORCE_MPQ_V1 | STREAM_FLAG_READ_ONLY, &MapMPQ))
	{
		std::cout << "Loading MPQ [" << fileName << "]" << std::endl;
	}
	else
	{
		std::cerr << "Error: unable to load MPQ file [" << fileName << "]: " << GetLastError() << std::endl;
		return false;
	}

	for (unsigned int i = 0; i < filesToLoad.size(); i++)
	{
		std::string currentFile = filesToLoad[i];
//		if (DEBUG)
//			std::cout << "Loading file [" << currentFile << "]" << std::endl;
		HANDLE SubFile;

		if (SFileOpenFileEx(MapMPQ, currentFile.c_str(), 0, &SubFile))
		{
			if (DEBUG)
				std::cout << "Found file [" << currentFile << "]" << std::endl;
			unsigned int FileLength = SFileGetFileSize(SubFile, NULL);

			if (FileLength > 0 && FileLength != 0xFFFFFFFF)
			{
				char *SubFileData = new char[FileLength];
				DWORD BytesRead = 0;

				if (SFileReadFile(SubFile, SubFileData, FileLength, &BytesRead, NULL))
				{
					//since it succeeded, FileLength should equal BytesRead
					saveFile(currentFile, SubFileData, BytesRead);
				}

				delete[] SubFileData;
			}

			SFileCloseFile(SubFile);
		}
	}

	SFileCloseArchive(MapMPQ);

	return true;
}

bool makeMPQ(std::string fileName)
{
	std::vector<std::string> files;
	rdirfiles(workingDirectory, files);

	HANDLE MapMPQ;

	if (SFileCreateArchive(fileName.c_str(), MPQ_CREATE_ARCHIVE_V1, files.size() + 15, &MapMPQ))
	{
		std::cout << "Creating MPQ [" << fileName << "]" << std::endl;
	}
	else
	{
		std::cerr << "Error: unable to create MPQ file [" << fileName << "]: " << GetLastError() << std::endl;
		return false;
	}

	//parameters for adding file to archive
	int dwFlags = 0;
	int dwCompression = 0;
	int dwCompressionNext = 0;

	if (compress)
	{
		dwFlags = MPQ_FILE_COMPRESS;
		dwCompression = MPQ_COMPRESSION_ZLIB;
		dwCompressionNext = MPQ_COMPRESSION_ZLIB;
	}

	for (unsigned int i = 0; i < files.size(); i++)
	{
		std::string currentFile = files[i];

		if (currentFile.empty())
			continue;

		if (currentFile[currentFile.length() - 1] == '/')
		{
			if (DEBUG)
				std::cout << "Ignoring directory [" << currentFile << "]" << std::endl;
		}
		else if (currentFile == "(listfile)")
		{
			std::cout << "Ignoring listfile [" << currentFile << "]" << std::endl;
		}
		else
		{
			if (DEBUG)
				std::cout << "Adding file [" << currentFile << "]" << std::endl;

			//change to backslash that MPQ uses for the filename in MPQ
			std::string mpqFile = currentFile;
			replace(mpqFile.begin(), mpqFile.end(), '/', '\\');

			if (!SFileAddFileEx(MapMPQ, (workingDirectory + currentFile).c_str(), mpqFile.c_str(), dwFlags, dwCompression,
					dwCompressionNext))
			{
				std::cerr << "Warning: failed to add " << currentFile << std::endl;
			}
		}
	}

	if (compactMPQ)
	{
		if (!SFileCompactArchive(MapMPQ, NULL, 0))
		{
			std::cerr << "Warning: failed to compact archive: " << GetLastError() << std::endl;
		}
	}

	SFileCloseArchive(MapMPQ);

	if (!mpqHeaderCopy.empty())
	{
		std::cout << "Prepending header from [" << mpqHeaderCopy << "] to [" << fileName << "]" << std::endl;

		//copy header to the mpq, use temporary file with _ prepended
		std::string tempFileName = "_" + fileName;

		//read header first
		std::ifstream fin(mpqHeaderCopy.c_str());

		if (!fin.is_open())
		{
			std::cerr << "Error: failed to copy header data: error while reading [" << mpqHeaderCopy << "]" << std::endl;
			return false;
		}

		char *buffer = new char[512];
		fin.read(buffer, 512);

		if (fin.fail())
		{
			std::cerr << "Warning: header data could not be completely read" << std::endl;
		}

		fin.close();

		std::ofstream fout(tempFileName.c_str());

		if (!fout.is_open())
		{
			std::cerr << "Error: failed to copy header data: error while writing to [" << tempFileName << "]" << std::endl;
			delete[] buffer;
			return false;
		}

		fout.write(buffer, 512);

		//now add the actual MPQ data, use same buffer
		std::ifstream finMPQ(fileName.c_str());

		if (!finMPQ.is_open())
		{
			std::cerr << "Error: failed to prepend header to MPQ: error while reading MPQ [" << fileName << "]" << std::endl;
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

		if (rename(tempFileName.c_str(), fileName.c_str()) != 0)
		{
			std::cerr << "Error while renaming [" << tempFileName << "] to [" << fileName << "]" << std::endl;
			return false;
		}
	}

	return true;
}

bool MPQ::extract_file(const char *file_name)
{
	HANDLE MPQ = 0;            // Open archive handle
	HANDLE SubFile = 0;

	if (SFileOpenArchive(file_name, 0, 0, &MPQ))
	{
		std::cout << "Loading MPQ [" << file_name << "]" << std::endl;
	}
	else
	{
		std::cerr << "Error: unable to load MPQ file [" << file_name << "]: " << GetLastError() << std::endl;

		return false;
	}

	std::cout << "Signature: " << SFileVerifyArchive(MPQ) << std::endl;

	if (SFileOpenFileEx(MPQ, file_name, 0, &SubFile))
	{

		unsigned int FileLength = SFileGetFileSize(SubFile, 0);

		if (FileLength > 0 && FileLength != 0xFFFFFFFF)
		{
			char *SubFileData = new char[FileLength];
			DWORD BytesRead = 0;

			if (SFileReadFile(SubFile, SubFileData, FileLength, &BytesRead, NULL))
			{
				//since it succeeded, FileLength should equal BytesRead
				saveFile(file_name, SubFileData, BytesRead);
			}

			delete[] SubFileData;
		}
	}

	SFileCloseFile(SubFile);
	SFileCloseArchive(MPQ);

	return 0;
}

int MPQ::main2(const char *from, const char *to)
{
	addDefaultLoadFiles();

	workingDirectory = to;

	if (!workingDirectory.empty() && workingDirectory[workingDirectory.length() - 1] != '/')
	{
		workingDirectory = workingDirectory + "/";
	}

	loadListFile(from);

	std::string mpqfile = from;

	loadMPQ(mpqfile);

	return 0;
}
