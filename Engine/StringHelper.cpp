#include "StringHelper.h"
#include <algorithm>

std::wstring StringHelper::StringToWide(std::string str)
{
	std::wstring wide_string(str.begin(), str.end());
	return wide_string;
}

std::string StringHelper::GetDirectoryFromPath(const std::string & filepath)
{
	size_t off1 = filepath.find_last_of('\\');
	size_t off2 = filepath.find_last_of('/');
	if (off1 == std::string::npos && off2 == std::string::npos) //If no slash or backslash in path?
	{
		return "";
	}
	if (off1 == std::string::npos)
	{
		return filepath.substr(0, off2);
	}
	if (off2 == std::string::npos)
	{
		return filepath.substr(0, off1);
	}
	//If both exists, need to use the greater offset
	return filepath.substr(0, std::max(off1, off2));
}

std::string StringHelper::GetFileExtension(const std::string & filename)
{
	size_t off = filename.find_last_of('.');
	if (off == std::string::npos)
	{
		return {};
	}
	return std::string(filename.substr(off + 1));
}

std::string StringHelper::GetFilenameFromDirectory(const std::string& filename)
{
	std::string result = filename;

	// Erase everything up to the beginning of the filename
	const size_t last_slash_idx = result.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
		result.erase(0, last_slash_idx + 1);

	return result;
}

std::string StringHelper::GetFilenameFromDirectoryNoExtension(const std::string& filename)
{
	std::string result = filename;

	// Erase everything up to the beginning of the filename
	const size_t last_slash_idx = result.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
		result.erase(0, last_slash_idx + 1);

	// Erase the file extension
	const size_t period_idx = result.rfind('.');
	if (std::string::npos != period_idx)
		result.erase(period_idx);

	return result;
}
