#pragma once
#include "framework.h"
#include "Resource.h"
#include <string>
LRESULT CALLBACK CodeEditorMenuProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
ATOM CodeEditorMenuRegisterClass(HINSTANCE hInstance);
HWND CreateCodeEditorMenu(HWND hwnd, int x, int y, int width, int height, HINSTANCE hInst);

