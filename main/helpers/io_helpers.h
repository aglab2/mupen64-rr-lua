#pragma once

#include <Windows.h>
#include <string>
#include <vector>

std::vector<std::string> get_files_with_extension_in_directory(
	const std::string &directory, const std::string &extension);

std::vector<std::string> get_files_in_subdirectories(
	const std::string& directory);

std::wstring strip_extension(const std::wstring &path);
std::wstring get_extension(const std::wstring &path);

void copy_to_clipboard(HWND owner, const std::string &str);

std::wstring get_desktop_path();
