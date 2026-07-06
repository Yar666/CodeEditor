#pragma once
#include "framework.h"
#include "Resource.h"
#include <string>
using namespace std;

LRESULT CALLBACK TextBoxProc(HWND, UINT, WPARAM, LPARAM);
ATOM MyTextBoxRegisterClass(HINSTANCE);
HWND CreateCustomTextBox(HWND hwnd, int x, int y, int width, int height, HINSTANCE hInst);
