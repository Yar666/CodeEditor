#pragma once
#include "framework.h"
#include "Resource.h"
#include <string>
LRESULT CALLBACK WindowAreaProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
ATOM WindowAreaRegisterClass(HINSTANCE hInstance, COLORREF color);
