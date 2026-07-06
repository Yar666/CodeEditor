#include "FileManager.h"
FileManager& FileManager::getInstance() {
	static FileManager instance{};
	return instance;
}
void FileManager::registerOnOpenFile(void (*fn)(HWND, size_t), HWND hWnd) {
	onOpenFileCallback.push_back(make_pair(fn, hWnd));
}
void FileManager::registerOnCloseFile(void (*fn)(HWND, size_t), HWND hWnd) {
	onCloseFileCallback.push_back(make_pair(fn, hWnd));
}
void FileManager::registerOnSelectFile(void (*fn)(HWND, size_t), HWND hWnd) {
	onSelectFileCallback.push_back(make_pair(fn, hWnd));
}
void FileManager::registerOnSaveFile(void (*fn)(HWND, size_t), HWND hWnd) {
	onSaveFileCallback.push_back(make_pair(fn, hWnd));
}
void FileManager::registerOnChangeDirectory(void (*fn)(HWND, wstring), HWND hWnd) {
	onChangeDirectory.push_back(make_pair(fn, hWnd));
}
void FileManager::openFile(const wstring& path) {
	static size_t _{ 1 };
	wstring name = path.substr(path.find_last_of(L"/\\") + 1);
	if (!checkAllowedExtension(getExtension(name))) {
		return;
	}
	for (auto& i : openedFiles) {
		if (i.second.path == path) {
			return;
		}
	}
	openedFiles[_] = _File{ name, path, NULL, _ };
	for (auto& i : onOpenFileCallback) {
		i.first(i.second, _);
	}
	_++;
}
void FileManager::saveFile() {
	for (auto& i : onSaveFileCallback) {
		i.first(i.second, activeFile);
	}
}
void FileManager::closeFile(size_t index) {
	DestroyWindow(openedFiles[index].hWndButton);
	openedFiles[index].hWndButton = NULL;
	openedFiles.erase(index);

	if (activeFile == index) {
		activeFile = wstring::npos;
	}
	for (auto& i : onCloseFileCallback) {
		i.first(i.second, index);
	}
}
wstring FileManager::getCurrentDirectory() {
	return directory;
}

wstring FileManager::readFile(size_t index) {
	wstring ifWillBeError{};
	if (!openedFiles.count(index)) {
		ifWillBeError = L"File isn't open!";
		return ifWillBeError;
	}
	wstring filePath{ openedFiles[index].path };
	HANDLE hF = CreateFileW(filePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hF == INVALID_HANDLE_VALUE) {
		ifWillBeError = L"Failed to open file: " + filePath;
		return ifWillBeError;
	}

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hF, &fileSize)) {
		CloseHandle(hF);
		ifWillBeError = L"Failed to get file size: " + filePath;
		return ifWillBeError;
	}

	vector<char> buffer(static_cast<size_t>(fileSize.QuadPart));

	DWORD bytesRead;
	if (!ReadFile(hF, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, NULL)) {
		CloseHandle(hF);
		ifWillBeError = L"Failed to read file: " + filePath;
		return ifWillBeError;
	}

	CloseHandle(hF);
	wstring fileContent(buffer.begin(), buffer.end());
	fileContent.erase(remove(fileContent.begin(), fileContent.end(), L'\n'), fileContent.end());
	//std::wcout << L"File content:\n" << fileContent << std::endl;

	return fileContent;
}
void FileManager::writeToFile(const wstring& text) {
	HANDLE hFile = CreateFileW(openedFiles[activeFile].path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		wcout << L"Failed to open file for writing: " << openedFiles[activeFile].path << std::endl;
		return;
	}

	DWORD bytesWritten;
	string _text{ text.begin(), text.end() };
	BOOL result = WriteFile(hFile, _text.c_str(), static_cast<DWORD>(_text.size()), &bytesWritten, NULL);
	if (!result) {
		wcout << L"Failed to write to file: " << openedFiles[activeFile].path << endl;
	}
	else {
		wcout << L"Successfully wrote to file: " << openedFiles[activeFile].path << endl;
	}

	CloseHandle(hFile);
}
void FileManager::run() {
	if (activeFile == wstring::npos) {
		cout << "Specify the file to run" << endl;
		return;
	}
	saveFile();
	DWORD tId;
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)buildAndShowOutput, &openedFiles[activeFile].path, 0, &tId);
	if (hThread != NULL) {
		CloseHandle(hThread);
	}

}


unordered_map<size_t, FileManager::_File>& FileManager::getOpenedFiles() {
	return openedFiles;
}
size_t FileManager::getActiveFile() {
	return activeFile;
}
void FileManager::setActiveFile(size_t index) {
	activeFile = index;
	for (auto& i : onSelectFileCallback) {
		i.first(i.second, index);
	}
}
void FileManager::setDirectory(wstring path) {
	directory = path;
	for (auto& i : onChangeDirectory) {
		i.first(i.second, directory);
	}
}
void FileManager::buildAndShowOutput(const wstring& path) {
	static bool isBuildActive{ false };
	if (isBuildActive) {
		return;
	}
	isBuildActive = true;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	wstring cmd{ L"g++ " + path + L" -o " + path.substr(0, path.find_last_of(L"\\")) + L"\\program.exe" };
	STARTUPINFO siBuild = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION piBuild;
	siBuild.dwFlags |= STARTF_USESTDHANDLES;
	cout << "Bulid started..." << endl;
	if (!CreateProcess(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &siBuild, &piBuild)) {
		CloseHandle(piBuild.hProcess);
		CloseHandle(piBuild.hThread);
		cout << "Process start error! Error code: " << GetLastError() << endl;
		isBuildActive = false;
		return;
	}

	WaitForSingleObject(piBuild.hProcess, INFINITE);


	cout << "Build has been completed" << endl;
	DWORD exitCode;
	if (!GetExitCodeProcess(piBuild.hProcess, &exitCode)) {
		cout << "Failed to get compile completion code!" << endl;
		CloseHandle(piBuild.hProcess);
		CloseHandle(piBuild.hThread);
		isBuildActive = false;
		return;
	}
	CloseHandle(piBuild.hProcess);
	CloseHandle(piBuild.hThread);
	if (exitCode != 0) {
		cout << "Compilation ended with an error! Error code: " << exitCode << endl;
		isBuildActive = false;
		return;
	}
	STARTUPINFO siRun = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION piRun;

	siRun.dwFlags |= STARTF_USESTDHANDLES;

	wstring exePath = path.substr(0, path.find_last_of(L"\\")) + L"\\program.exe ";
	wstring cmdCommand = L"cmd.exe /K \"" + exePath + L"\"";
	cout << "Running an executable file..." << endl;
	if (!CreateProcess(NULL, (LPWSTR)cmdCommand.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &siRun, &piRun)) {
		CloseHandle(piRun.hProcess);
		CloseHandle(piRun.hThread);
		wcout << L"Process start error!" << endl;
		isBuildActive = false;
		return;
	}
	//WaitForSingleObject(piRun.hProcess, INFINITE);
	cout << "The executable has finished executing" << endl;
	CloseHandle(piRun.hProcess);
	CloseHandle(piRun.hThread);

	isBuildActive = false;
}
bool FileManager::checkAllowedExtension(const wstring& extension) {
	static unordered_set<wstring> allowedExtension{
		L"cpp",
		L"h"
	};
	if (allowedExtension.count(extension)) {
		return true;
	}
	return false;
}
wstring FileManager::getExtension(const wstring& filename) {
	size_t dotPosition = filename.find_last_of(L'.');
	if (dotPosition != std::wstring::npos) {
		return filename.substr(dotPosition + 1);
	}
	else {
		return L"No extension"; // Нет расширения
	}
}