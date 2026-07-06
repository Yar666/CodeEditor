#pragma once
#include "framework.h"
#include "Resource.h"
#include <iostream>
#include <string>
using namespace std;

LRESULT CALLBACK FileExplorerProc(HWND, UINT, WPARAM, LPARAM);
ATOM FileExplorerRegisterClass(HINSTANCE);
HWND CreateFileExplorer(HWND hwnd, int x, int y, int width, int height, HINSTANCE hInst);
