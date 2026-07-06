#pragma once
#include "framework.h"
#include "Resource.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
using namespace std;
class FileManager
{
public:
	struct _File {
		wstring name;
		wstring path;
		HWND hWndButton;
		size_t hmenu;
	};
	
public:
	static FileManager& getInstance();
	void registerOnOpenFile(void (*fn)(HWND, size_t), HWND hWnd);
	void registerOnCloseFile(void (*fn)(HWND, size_t), HWND hWnd);
	void registerOnSelectFile(void (*fn)(HWND, size_t), HWND hWnd);
	void registerOnSaveFile(void (*fn)(HWND, size_t), HWND hWnd);
	void registerOnChangeDirectory(void (*fn)(HWND, wstring), HWND hWnd);
	void openFile(const wstring& path);
	void closeFile(size_t index);
	void setActiveFile(size_t index);
	void saveFile();
	void setDirectory(wstring path);
	void writeToFile(const wstring& text);
	void run();
	wstring getCurrentDirectory();
	wstring readFile(size_t index);
	unordered_map<size_t, _File>& getOpenedFiles();
	size_t getActiveFile();
private:
	wstring directory{ L""};
	size_t activeFile{wstring::npos};
	unordered_map<size_t, _File> openedFiles;
	vector<pair<void(*)(HWND, size_t), HWND>> onOpenFileCallback;
	vector<pair<void(*)(HWND, size_t), HWND>> onCloseFileCallback;
	vector<pair<void(*)(HWND, size_t), HWND>> onSelectFileCallback;
	vector<pair<void(*)(HWND, size_t), HWND>> onSaveFileCallback;
	vector<pair<void(*)(HWND, wstring), HWND>> onChangeDirectory;
	FileManager() = default;
	~FileManager() = default;
	FileManager(const FileManager&) = delete;
	FileManager& operator=(const FileManager&) = delete;
private:
	void static buildAndShowOutput(const wstring& path);
	bool checkAllowedExtension(const wstring& extension);
	wstring getExtension(const wstring& filename);
};

