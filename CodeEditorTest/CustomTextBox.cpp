#include "CustomTextBox.h"
#include "FileManager.h"
#include <iostream>
#include <unordered_map>
using namespace std;
ATOM MyTextBoxRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = TextBoxProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(30, 30, 30));
    wcex.lpszClassName = L"CustomTextBox";
    return RegisterClassExW(&wcex);
}
HWND CreateCustomTextBox(HWND hwnd, int x, int y, int width, int height, HINSTANCE hInst) {
    return CreateWindow(L"CustomTextBox", L"", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL, x, y, width, height, hwnd, NULL, hInst, NULL);
}
struct Caret {
    int x;
    int y;
    size_t pos;
};
struct SelectedText {
    size_t from;
    size_t to;
    bool isSelected;
    bool direction; // true (forward ->). false (backward <-).
    bool normalize() {
        if (from > to) {
            size_t tmp{ to };
            to = from;
            from = tmp;
            direction = !direction;
            return true;
        }
        return false;
    }
};
struct TextBoxInfo {
    wstring text{ L"" };
    size_t pos{ 0 };
    bool hasFocus{ false };
};
struct ColoredWord {
    size_t end;
    COLORREF color;
};
void setFontSize(HFONT& hFont,HDC &hdc, int sizeYTextBox) {
    int fontHeight = -MulDiv(sizeYTextBox/1.5, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    hFont = CreateFont(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Consolas");    
}
unordered_map<size_t, ColoredWord> makeColored(const wstring& text) {
    const static unordered_map<wstring, COLORREF> keywords{
        {L"int", RGB(252, 169, 3)},
        {L"void", RGB(252, 169, 3)},
        {L"return", RGB(252, 169, 3)},
        {L"class", RGB(252, 169, 3)},
        {L"const", RGB(0, 200, 255)},
        {L"static", RGB(0, 200, 255)},
        {L"main", RGB(202, 252, 3)},
        {L"#include", RGB(202, 252, 3)},
        {L"cout", RGB(3, 252, 198)},
        {L"using", RGB(3, 252, 198)},
        {L"namespace", RGB(3, 252, 198)},
        {L"endl", RGB(3, 252, 198)},
        {L"if", RGB(237, 104, 231)},
        {L"else", RGB(237, 104, 231)},
        {L"switch", RGB(237, 104, 231)},
        {L"case", RGB(237, 104, 231)},
        {L"while", RGB(237, 104, 231)},
        {L"for", RGB(237, 104, 231)},
        {L"RGB", RGB(235, 80, 209)},
    };
    unordered_map<size_t, ColoredWord> cw{};
    size_t inx{};
    // Покраска ключевых слов
    for (auto& i: keywords) {
        size_t findResult{ text.find(i.first) };
        size_t offset{};
        while (findResult != wstring::npos) {
            size_t begin{ findResult };
            size_t end(begin + i.first.size());
            offset = end;
            cw.emplace(begin, ColoredWord{ end, i.second });
            findResult = text.find(i.first, offset);
        }
    }
    // Покраска строк ""
    size_t pos = 0;
    while ((pos = text.find(L'"', pos)) != wstring::npos) {
        size_t start = pos++;
        size_t end = text.find(L'"', pos);
        if (end != wstring::npos) {
            if (start+1 < text.size() && start > 0 && text[start - 1] == '\'' &&
                start + 1 <= end && text[start + 1] == '\'') {
                pos = end + 1;
                continue;
            }
            cw.emplace(start, ColoredWord{ end + 1, RGB(11, 255 , 3) });
            pos = end + 1;
        }
        else {
            break;
        }
    }
    return cw;
}

int getCharWidth(HDC hdc, HFONT& hFont, wchar_t& ch) {
    static unordered_map<wchar_t, int> charWidthCache;

    if (charWidthCache.count(ch)) return charWidthCache[ch];
    SIZE sz;
    HGDIOBJ oldFont = SelectObject(hdc, hFont);
    GetTextExtentPoint32(hdc, &ch, 1, &sz);
    SelectObject(hdc, oldFont);
    return charWidthCache[ch] = sz.cx;
}
int getTextWidth(HDC hdc, HFONT& hFont, wstring text) {
    int result{};
    for (wchar_t& i : text) {
        result += getCharWidth(hdc, hFont, i);
    }
    return result;
}
int setWindowScrollH(HWND hWnd, HFONT& hFont, wstring& text, SCROLLINFO& siH) {
    int maxWidthTextBox{};
    HDC hdc = GetDC(hWnd);
    size_t i{};
    size_t nextReturn{ text.find_first_of('\r', i) };
    while (nextReturn != wstring::npos) {
        int _{ getTextWidth(hdc, hFont, text.substr(i, nextReturn-i)) };
        if (_ > maxWidthTextBox) {
            maxWidthTextBox = _;
        }
        i = nextReturn + 1;
        nextReturn = text.find_first_of('\r', i);
    }
    ReleaseDC(hWnd, hdc);
    RECT rt;
    GetClientRect(hWnd, &rt);
    siH.nMin = 0;
    siH.nMax = maxWidthTextBox;
    siH.nPage = rt.right;
    SetScrollInfo(hWnd, SB_HORZ, &siH, TRUE);
    return maxWidthTextBox;
}
int setWindowScrollV(HWND hWnd, int enterAmount, int sizeYTextBox, SCROLLINFO& siV) {
    int maxHeightTextBox{ enterAmount * sizeYTextBox };
    RECT rt;
    GetClientRect(hWnd, &rt);

    siV.nMax = maxHeightTextBox;
    siV.nPage = rt.bottom;
    SetScrollInfo(hWnd, SB_VERT, &siV, TRUE);
    return maxHeightTextBox;
}
int replaceSelectedText(TextBoxInfo& textBox, SelectedText& sl, wstring newText) { // return amount of enters
    int erasedEnterAmount{ (int)count(textBox.text.begin() + sl.from, textBox.text.begin() + sl.to, '\r') };
    textBox.text.erase(textBox.text.begin() + sl.from, textBox.text.begin() + sl.to);
    textBox.pos = sl.from;
    
    textBox.text.insert(textBox.pos, newText);
    textBox.pos += newText.size();
    sl.isSelected = false;
    return erasedEnterAmount;
}
POINT PosToXY(HWND hWnd,size_t pos, HFONT& hFont, SCROLLINFO siH, SCROLLINFO siV, const TextBoxInfo& textBox, int enterAmount, int sizeYTextBox) {
    HDC hdc = GetDC(hWnd);
    RECT rt;
    GetClientRect(hWnd, &rt);
    GetScrollInfo(hWnd, SB_HORZ, &siH);
    GetScrollInfo(hWnd, SB_VERT, &siV);
    int curXPos{ 0 - siH.nPos };
    int lineSpacing{ 2 };
    wstring text{ textBox.text };
    int charWidth{};
    size_t curTextPos{};
    for (int i{}; i < enterAmount; i++) {
        int y{ (i * sizeYTextBox) - siV.nPos };
        if (y + sizeYTextBox < rt.top) {
            size_t nextReturn{ text.find_first_of('\r', curTextPos) };
            if (nextReturn != wstring::npos) {
                curTextPos = nextReturn + 1;
                continue;
            }
        }
        else if (y > rt.bottom) {
            i = enterAmount;
            continue;
        }
        while (curTextPos <= text.size()) {
            if (pos == curTextPos) {
                return { curXPos, y };
            }
            if (text.c_str()[curTextPos] == '\r' || curTextPos == text.size()) {
                curTextPos++;
                break;
            }
            charWidth = getCharWidth(hdc, hFont, text[curTextPos]);        
            curXPos += charWidth;
            curTextPos++;
        }
        curXPos = 0 - siH.nPos;
    }
    ReleaseDC(hWnd, hdc);
    return {-1,-1};
}
Caret MousePosToCaret(HWND hWnd, int mouseX, int mouseY,HFONT& hFont, SCROLLINFO siH, SCROLLINFO siV, const TextBoxInfo& textBox, int enterAmount, int sizeYTextBox) {
    HDC hdc = GetDC(hWnd);
    GetScrollInfo(hWnd, SB_HORZ, &siH);
    GetScrollInfo(hWnd, SB_VERT, &siV);
    int curXPos{ 0 - siH.nPos };
    int lineSpacing{ 2 };
    wstring text{ textBox.text };
    int charWidth{};
    size_t curTextPos{};
    for (int i{}; i < enterAmount; i++) {
        int y{ (i * sizeYTextBox) - siV.nPos };
        bool isLineSelected = (mouseY >= y && mouseY <= y + sizeYTextBox);
        if (!isLineSelected) {
            size_t nextReturn{ text.find_first_of('\r', curTextPos) };
            if (nextReturn != wstring::npos) {
                curTextPos = nextReturn + 1;
                continue;
            }
        }
        while (curTextPos <= text.size()) {
            if (text.c_str()[curTextPos] == '\r' || curTextPos == text.size()) {
                if (isLineSelected) {
                    return { curXPos, y, curTextPos};
                }
                curTextPos++;
                break;
            }

            charWidth = getCharWidth(hdc, hFont, text[curTextPos]);
            if (mouseX <= curXPos + charWidth && mouseX >= curXPos) {
                //wcout << "Char: " << text[curTextPos] << " x: " << curXPos << " y: " << y << endl;
                return { curXPos, y, curTextPos + 1 > textBox.text.size() ? curTextPos : curTextPos + 1 };
            }
            curXPos += charWidth;
            curTextPos++;
        }
        curXPos = 0 - siH.nPos;
    }
    ReleaseDC(hWnd, hdc);
    return { 0,0, wstring::npos };
}
class HWND_DATA {
public:
    TextBoxInfo textBox;
    bool isFileOpen{ false };
    unordered_map<size_t, ColoredWord> cw{};
    int sizeYTextBox{ 20 };
    int maxWidthTextBox{};
    int maxHeightTextBox{};
    int enterAmount{ (int)count(textBox.text.begin(), textBox.text.end(), '\r') + 1 };
    int mouseClickedPosX{}, mouseClickedPosY{};
    SelectedText selectedText{ 0,0,false, true };
    HFONT hFont{};
    SCROLLINFO siH{ sizeof(SCROLLINFO), SIF_ALL };
    SCROLLINFO siV{ sizeof(SCROLLINFO), SIF_ALL };
};
static unordered_map<size_t, HWND_DATA> datamap;
HWND_DATA& FileIndexToData(size_t index) {
    if (datamap.count(index)) {
        return datamap[index];
    }
    datamap[index] = HWND_DATA{};
    if (FileManager::getInstance().getActiveFile() != wstring::npos) {
        datamap[index].isFileOpen = true;
        datamap[index].textBox.text = FileManager::getInstance().readFile(index);
        datamap[index].enterAmount = (int)count(datamap[index].textBox.text.begin(), datamap[index].textBox.text.end(), '\r') + 1;
    }
    
    return datamap[index];
}
void onTextBoxSelectFile(HWND hWnd, size_t index) {
    HWND_DATA& data{ FileIndexToData(index) };
    data.cw = makeColored(data.textBox.text);
    data.textBox.hasFocus = true; 
    setWindowScrollV(hWnd, data.enterAmount, data.sizeYTextBox, data.siV);
    setWindowScrollH(hWnd, data.hFont, data.textBox.text, data.siH);
    SetFocus(hWnd); 
    InvalidateRect(hWnd, NULL, TRUE);
}
void onTextBoxOpenFile(HWND hWnd, size_t index) {
    FileManager::getInstance().setActiveFile(index);
}
void onTextBoxSaveFile(HWND hWnd, size_t index) {
    FileManager::getInstance().writeToFile(FileIndexToData(index).textBox.text);
}
void onTextBoxCloseFile(HWND hWnd, size_t index) {
    datamap.erase(index);
    InvalidateRect(hWnd, NULL, TRUE);
}
LRESULT CALLBACK TextBoxProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    FileManager& manager{ FileManager::getInstance() };
    HWND_DATA& data{ FileIndexToData(manager.getActiveFile()) };
    TextBoxInfo& textBox{ data.textBox };
    unordered_map<size_t, ColoredWord>& cw{ data.cw};
    int& sizeYTextBox{ data.sizeYTextBox };
    int& maxWidthTextBox{ data.maxWidthTextBox };
    int& maxHeightTextBox{ data.maxHeightTextBox };
    int& enterAmount{ data.enterAmount };
    int& mouseClickedPosX{ data.mouseClickedPosX }, mouseClickedPosY{ data.mouseClickedPosY };
    SelectedText& selectedText{ data.selectedText };
    HFONT& hFont{ data.hFont };
    SCROLLINFO& siH{ data.siH };
    SCROLLINFO& siV{ data.siV }; 
    switch (message) {
        case WM_CREATE: {
            manager.registerOnSelectFile(onTextBoxSelectFile, hWnd);
            manager.registerOnOpenFile(onTextBoxOpenFile, hWnd);
            manager.registerOnSaveFile(onTextBoxSaveFile, hWnd);
            manager.registerOnCloseFile(onTextBoxCloseFile, hWnd);
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            setFontSize(hFont, hdc, sizeYTextBox);
            EndPaint(hWnd, &ps);
            RECT rt;
            GetClientRect(hWnd, &rt);
            setWindowScrollH(hWnd, hFont, textBox.text, siH);
            setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
            cw = makeColored(textBox.text);
        }
        case WM_MOUSEWHEEL: {
            if (!data.isFileOpen) { break; }
            int delta{ GET_WHEEL_DELTA_WPARAM(wParam) };
            cout << "TextBox WM_MOUSEWHEEL: delta " << delta << endl;
            for (size_t i{}; i < abs(delta / 120); i++) {
                if (delta < 0) {
                    SendMessage(hWnd, WM_VSCROLL, SB_LINERIGHT, NULL);
                }else {
                    SendMessage(hWnd, WM_VSCROLL, SB_LINELEFT, NULL);
                }
                
            }
            break;
        }
        case WM_HSCROLL: {
            if (!data.isFileOpen) { break; }
            cout << "Called HSCROLL" << endl;
            //SCROLLINFO si{ sizeof(SCROLLINFO), SIF_ALL };
            GetScrollInfo(hWnd, SB_HORZ, &siH);
            int oldPos{ siH.nPos };
            switch (LOWORD(wParam)) {
                case SB_LINELEFT:
                    siH.nPos -= 30;
                    break;
                case SB_LINERIGHT:
                    siH.nPos += 30;
                    break;
                case SB_PAGELEFT:
                    siH.nPos -= siH.nPage;
                    break;
                case SB_PAGERIGHT:
                    siH.nPos += siH.nPage;
                    break;
                case SB_THUMBTRACK:
                    int movedPos{ HIWORD(wParam) };

                    if (movedPos - oldPos >= 10 || oldPos - movedPos >= 10||movedPos == 0 || movedPos == siH.nMax) {
                        siH.nPos = movedPos;
                    }
                    break;
            }
            siH.nPos = max(siH.nMin, min(siH.nPos, siH.nMax));
            if (siH.nPos != oldPos) {
                SetScrollInfo(hWnd, SB_HORZ, &siH, TRUE);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }
        case WM_VSCROLL: {
            if (!data.isFileOpen) { break; }
            cout << "Called VSCROLL" << endl;
            SCROLLINFO si{ sizeof(SCROLLINFO), SIF_ALL };
            GetScrollInfo(hWnd, SB_VERT, &si);
            int oldPos{ si.nPos };
            switch (LOWORD(wParam)) {
            case SB_LINELEFT:
                si.nPos -= 10;
                break;
            case SB_LINERIGHT:
                si.nPos += 10;
                break;
            case SB_PAGELEFT:
                si.nPos -= si.nPage;
                break;
            case SB_PAGERIGHT:
                si.nPos += si.nPage;
                break;
            case SB_THUMBTRACK:
                int movedPos{ HIWORD(wParam) };
                if (movedPos - oldPos >= sizeYTextBox || oldPos-movedPos >= sizeYTextBox || movedPos == 0 || movedPos==si.nMax) {
                    si.nPos = movedPos;
                }
                
                break;
            }
            si.nPos = max(si.nMin, min(si.nPos, si.nMax));
            if (si.nPos != oldPos) {
                SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }
        case WM_LBUTTONUP: {
            if (!data.isFileOpen) { break; }
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            Caret cr = MousePosToCaret(hWnd, pt.x, pt.y, hFont, siH, siV, textBox, enterAmount, sizeYTextBox);
            if (cr.pos == wstring::npos) {
                break;
            }
            selectedText.to = cr.pos;
            selectedText.normalize();
            if (selectedText.to != selectedText.from) {
                selectedText.isSelected = true;
            }
            textBox.pos = cr.pos;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        case WM_LBUTTONDOWN: {
            if (!data.isFileOpen) { break; }
            mouseClickedPosX = LOWORD(lParam);
            mouseClickedPosY = HIWORD(lParam);
            cout << "MouseDown x: " << mouseClickedPosX << " y: " << mouseClickedPosY << endl;
            Caret cr = MousePosToCaret(hWnd, mouseClickedPosX, mouseClickedPosY, hFont, siH, siV, textBox, enterAmount, sizeYTextBox);
            if (cr.pos == wstring::npos) {
                break;
            }
            
            selectedText.from = cr.pos;
            selectedText.isSelected = false;
            break;
        }
        case WM_KEYDOWN: {
            if (!data.isFileOpen) { break; }
            if (GetKeyState(VK_CONTROL) < 0) { // горячие клавиши
                if (wParam == 'A') { // ctrl + a
                    selectedText.from = 0;
                    selectedText.to = textBox.text.size();
                    selectedText.direction = true; // forward ->
                    selectedText.isSelected = true;
                    textBox.pos = textBox.text.size();
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                if (wParam == 'V') { // ctrl + v
                    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                        if (OpenClipboard(0)) {
                            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                            if (!hData) {
                                cout << "Clipboard data is NULL!" << endl;
                            }
                            if (hData) {
                                wchar_t* hBuff = static_cast<wchar_t*>(GlobalLock(hData));
                                if (hBuff) {
                                    wstring tmp{ hBuff };
                                    tmp.erase(remove(tmp.begin(), tmp.end(), L'\n'), tmp.end());
                                    if (selectedText.isSelected) {
                                        enterAmount -= replaceSelectedText(textBox, selectedText, tmp);
                                        setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                                    }
                                    else {
                                        textBox.text.insert(textBox.pos, tmp);
                                        textBox.pos += tmp.size();
                                    }
                                    enterAmount =  (int)count(textBox.text.begin(), textBox.text.end(), '\r') + 1 ;
                                    cw = makeColored(textBox.text);
                                    InvalidateRect(hWnd, NULL, TRUE);
                                }
                                GlobalUnlock(hData);
                            }
                            CloseClipboard();
                        }
                    }
                    setWindowScrollH(hWnd, hFont, textBox.text, siH);
                    setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                }
                if (wParam == 'C') { // ctrl + c
                    if (!selectedText.isSelected) {
                        break;
                    }
                    if (OpenClipboard(0)) {
                        wstring slText{ textBox.text.substr(selectedText.from, selectedText.to - selectedText.from) };
                        size_t size = (slText.size() + 1) * sizeof(wchar_t);
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size); 
                        if (!hMem) {
                            cout << "Failed to allocate memory!" << endl;
                        }
                        wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
                        if (pMem) {
                            wcscpy_s(pMem, slText.size() + 1, slText.c_str());
                            GlobalUnlock(hMem);
                        }
                        if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
                            GlobalFree(hMem);
                            cout << "Failed to set clipboard data!" << endl;
                        }
                        CloseClipboard();
                    }
                }
                break;
            }
            switch (wParam) {
                if (!data.isFileOpen) { break; }
                case VK_ESCAPE: {
                    selectedText.isSelected = false;
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;
                }
                case VK_TAB: {
                    wstring tab{ L"    " };
                    if (selectedText.isSelected) {
                        enterAmount -= replaceSelectedText(textBox, selectedText, tab);
                        setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                    }
                    else {
                        textBox.text.insert(textBox.pos, tab);
                        textBox.pos += tab.size();
                    }
                    setWindowScrollH(hWnd, hFont, textBox.text, siH);
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;
                }
                case VK_BACK: {
                    cout << "Pressed backspace" << endl;
                    if (textBox.text.size() == 0 || textBox.pos == 0) {
                        break;
                    }
                    wchar_t ch{ textBox.text.c_str()[textBox.pos - 1] };
                    if (ch == '\r') {
                        enterAmount--;
                        setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                    }
                    if (selectedText.isSelected) {
                        enterAmount -= replaceSelectedText(textBox, selectedText, L"");
                        setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                    }
                    else {
                        textBox.text = textBox.text.erase(textBox.pos - 1, 1);
                        textBox.pos -= 1;
                    }
                    setWindowScrollH(hWnd, hFont, textBox.text, siH);
                    cw = makeColored(textBox.text);
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;
                }
                case VK_DELETE: {
                    cout << "Pressed delete" << endl;
                    if (textBox.text.size() == 0 || textBox.pos == textBox.text.size() ) {
                        break;
                    }
                    wchar_t ch{ textBox.text.c_str()[textBox.pos] };
                    if ((ch == '\r') && enterAmount != 1) {
                        enterAmount--;
                        setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                    }
                    if (selectedText.isSelected) {
                        enterAmount -= replaceSelectedText(textBox, selectedText, L"");
                        setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                    } else {
                        textBox.text = textBox.text.erase(textBox.pos, 1);
                    }
                    setWindowScrollH(hWnd, hFont, textBox.text, siH);
                    cw = makeColored(textBox.text);
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;
                }
                case VK_RIGHT: {
                    size_t textSize{ textBox.text.size() };
                    if (textBox.pos + 1 > textSize) {
                        break;
                    }
                    if (GetKeyState(VK_SHIFT) < 0) {
                        //  VK_SHIFT нажат
                        if (!selectedText.isSelected) {
                            selectedText.isSelected = true;
                            selectedText.from = textBox.pos;
                            selectedText.to = textBox.pos+1;
                            selectedText.direction = true; // backward
                        }else if (selectedText.direction) { // forward ->
                            cout << "Right arrow(forward)" << endl;
                            selectedText.to += 1;
                        }
                        else { // backward <-
                            cout << "Right arrow(backward)" << endl;
                            selectedText.from += 1;
                        }
                        if (selectedText.normalize()) {
                            textBox.pos = selectedText.from;
                        }
                        else {
                            textBox.pos = selectedText.to;
                        }
                    }
                    else {
                        if (selectedText.isSelected) {
                            selectedText.isSelected = false;
                        }
                        textBox.pos += 1;
                    }
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;
                }
                case VK_LEFT: {
                    if (textBox.pos == 0) {
                        break;
                    }
                    if (GetKeyState(VK_SHIFT) < 0) {
                        //  VK_SHIFT нажат
                        if (!selectedText.isSelected) {
                            selectedText.isSelected = true;
                            selectedText.from = textBox.pos-1;
                            selectedText.to = textBox.pos;
                            selectedText.direction = false; // backward <-
                        }else if (selectedText.direction) { // forward ->
                            cout << "Left arrow(forward)" << endl;
                            selectedText.to -= 1;
                        }
                        else { // backward <-
                            cout << "Left arrow(backward)" << endl;
                            selectedText.from -= 1;
                        }
                        if (selectedText.normalize()) {
                            textBox.pos = selectedText.to;
                        }
                        else {
                            textBox.pos = selectedText.from;
                        }
                    }
                    else {
                        if (selectedText.isSelected) {
                            selectedText.isSelected = false;
                        }
                        textBox.pos -= 1;
                    }
                    
                    InvalidateRect(hWnd, NULL, TRUE);
                    break;
                }
            }
            break;
        }
        case WM_CHAR: {
            
            if (!data.isFileOpen || !textBox.hasFocus || ((char)wParam == 27) || ((char)wParam == 8) || ((char)wParam == '\t') || GetKeyState(VK_CONTROL) < 0) {
                break;
            }
            RECT toUpdate, rt;
            GetClientRect(hWnd, &rt);
            bool useUpdateRect{ false };
            cout << "Pressed " << (char)wParam << " button" << endl;
            wstring tmp{ static_cast<wchar_t>(wParam) };
            
            if (tmp == L"\r") {
                enterAmount++;
                setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
                cout << "Amount of enters: " << enterAmount << endl;
            }
            POINT insertedCharXY{ PosToXY(hWnd,textBox.pos, hFont, siH, siV, textBox, enterAmount, sizeYTextBox) };
            if (selectedText.isSelected) {
                enterAmount -= replaceSelectedText(textBox, selectedText, tmp);
                setWindowScrollV(hWnd, enterAmount, sizeYTextBox, siV);
            }
            else {
                if (insertedCharXY.x != -1) {
                    
                    if (tmp == L"\r") {
                        toUpdate = RECT{ rt.left, insertedCharXY.y, rt.right, rt.bottom };
                    }
                    else {
                        toUpdate = RECT{ rt.left, insertedCharXY.y, rt.right, insertedCharXY.y + sizeYTextBox + 2 };
                    }
                    useUpdateRect = true;
                }
                textBox.text.insert(textBox.pos, tmp);
                textBox.pos += 1;
            }  
            setWindowScrollH(hWnd, hFont, textBox.text, siH);
            cw = makeColored(textBox.text);
            if (useUpdateRect) {
                //HDC hdc = GetDC(hWnd);
                //Rectangle(hdc, toUpdate.left, toUpdate.top, toUpdate.right, toUpdate.bottom);
                //ReleaseDC(hWnd, hdc);
                InvalidateRect(hWnd, &toUpdate, TRUE);
            }
            else if(insertedCharXY.x != -1) { // Нет смысла рисовать то, что находится за пределами окна
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }
        case WM_PAINT: {
            if (!data.isFileOpen) { break; }
            PAINTSTRUCT ps;
            RECT rt, charRt;
            HDC hdc = BeginPaint(hWnd, &ps);
            HPEN hpen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HBRUSH hbrush = CreateSolidBrush(RGB(255, 0, 0)); 
            GetScrollInfo(hWnd, SB_HORZ, &siH);
            GetScrollInfo(hWnd, SB_VERT, &siV);
            GetClientRect(hWnd, &rt);

            SelectObject(hdc, hpen);
            SetBkColor(hdc, RGB(3, 111, 252));
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255,255,255));

            bool isChangedColorFlag{ false };
            bool isChangedBackgroundFlag{ false };
            bool isShouldUpdateScrollbarFlag{ false };
            int sizeXTextBox{ rt.right };
            int lineSpacing{ 2 };
            int charWidth{};
            int curXPos{ 0 - siH.nPos };
            if (curXPos < 0) {
                cout << "curXPos: " << curXPos << endl;
            }
            wstring text{ textBox.text };
            SelectObject(hdc, hbrush);
            HGDIOBJ oldFont = SelectObject(hdc, hFont);
            size_t curTextPos{};
            size_t paintTo{};

            for (int i{}; i < enterAmount; i++) {
                int y{ (i * sizeYTextBox)- siV.nPos };
                if (y+sizeYTextBox < rt.top) {
                    size_t nextReturn{ text.find_first_of('\r', curTextPos) };
                    if (nextReturn != wstring::npos) {
                        curTextPos = nextReturn + 1;
                        continue;
                    }
                }
                else if (y > rt.bottom) {
                    i = enterAmount;
                    continue;
                }
                //Rectangle(hdc , 0, y + lineSpacing, sizeXTextBox, ((i+1) * sizeYTextBox) - siV.nPos);
                while (curTextPos <= text.size()){
                    if (textBox.pos == curTextPos && textBox.hasFocus) {
                        MoveToEx(hdc, curXPos, y + lineSpacing, NULL);
                        LineTo(hdc, curXPos, y + lineSpacing + 20);
                    }
                    if (text.c_str()[curTextPos] == '\r') {
                        curTextPos++;
                        break;
                    }
                    if (!isChangedColorFlag && cw.count(curTextPos)) {
                        paintTo = cw.at(curTextPos).end;
                        SetTextColor(hdc, cw.at(curTextPos).color);
                        isChangedColorFlag = true;
                    }
                    charWidth = getCharWidth(hdc, hFont, text[curTextPos]);
                    charRt = { curXPos,y + lineSpacing, curXPos + charWidth, y + lineSpacing + sizeYTextBox };
                    bool isCharSelected = selectedText.isSelected && (selectedText.from <= curTextPos && curTextPos+1 <= selectedText.to);
                    if (isCharSelected && !isChangedBackgroundFlag) {
                        SetBkMode(hdc, OPAQUE);
                        if (!textBox.hasFocus) {
                            SetBkColor(hdc, RGB(84, 94, 112));
                        }
                        isChangedBackgroundFlag = true;
                    }else if (!isCharSelected && isChangedBackgroundFlag) {
                        isChangedBackgroundFlag = false;
                        SetBkMode(hdc, TRANSPARENT);
                    }

                    TextOut(hdc, curXPos, y + lineSpacing, &text[curTextPos], 1);
                    
                    
                    curXPos += charWidth;
                    curTextPos++;
                    if (isChangedColorFlag && curTextPos == paintTo) {
                        paintTo = 0;
                        SetTextColor(hdc, RGB(255, 255, 255));
                        isChangedColorFlag = false;
                    }
                }
                curXPos = 0 - siH.nPos;
            }
            SelectObject(hdc, oldFont);
            DeleteObject(hbrush);
            DeleteObject(hpen);
            EndPaint(hWnd, &ps);
            break;
        }
        
        case WM_SETFOCUS: {

            cout << "Set focus" << endl;
            textBox.hasFocus = true;
            break;
        }
        case WM_ACTIVATE: {
            cout << "WM_ACTIVATE" << endl;
            if (LOWORD(wParam) == WA_INACTIVE) {
                cout << "Window lost focus" << endl;
                textBox.hasFocus = false;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }
        case WM_KILLFOCUS: {
            if (!textBox.hasFocus) {
                break;
            }
            cout << "Kill focus" << endl;
            textBox.hasFocus = false;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        case WM_MOUSEACTIVATE: {
            cout << "Textbox WM_MOUSEACTIVATE" << endl;
            SetFocus(hWnd);
            return MA_ACTIVATE;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
