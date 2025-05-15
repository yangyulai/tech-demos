// main.cpp
#include <windows.h>
#include <commdlg.h>
#include <winhttp.h>
#include <string>
#include <fstream>
#include <map>
#include <lua/lua.hpp>
// 简单的 UTF8->UTF16 转换
static std::wstring utf8_to_wstring(const std::string& u8) {
    if (u8.empty()) return {};
    int sz = MultiByteToWideChar(CP_UTF8, 0, u8.data(), (int)u8.size(), nullptr, 0);
    std::wstring out(sz, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.data(), (int)u8.size(), &out[0], sz);
    return out;
}
// 控件 ID
enum {
    ID_URL_EDIT      = 1001,
    ID_BROWSE_BUTTON = 1002,
    ID_FILE_EDIT     = 1003,
    ID_SEND_BUTTON   = 1004,
    ID_CLEAR_BUTTON  = 1005,
    ID_LOG_EDIT      = 1006,
};

HWND hUrlEdit, hBrowseBtn, hFileEdit, hSendBtn, hClearBtn, hLogEdit;

HWND hGroupUrl, hLabelUrl;
HWND hGroupFile, hLabelFile;
HFONT hGuiFont;

HINSTANCE hInst;

// 将文本追加到日志框
void AppendLog(const std::wstring& text) {
    int len = GetWindowTextLengthW(hLogEdit);
    SendMessageW(hLogEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

std::wstring ReadFileToWString(const std::wstring& path) {
    std::ifstream fs(path, std::ios::binary);
    if (!fs) return L"";
    std::string buf((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    // 简单从 UTF-8 转为 UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)buf.size(), nullptr, 0);
    std::wstring wbuf(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)buf.size(), &wbuf[0], wlen);
    return wbuf;
}

// 发送 HTTP POST 请求
void DoHttpRequest(const std::wstring& url, const std::wstring& body) {
    URL_COMPONENTS uc = {0};
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength    = (DWORD)-1;
    uc.dwHostNameLength  = (DWORD)-1;
    uc.dwUrlPathLength   = (DWORD)-1;
    uc.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &uc)) {
        AppendLog(L"[错误] 无效的 URL");
        return;
    }

    std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
    std::wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
    if (uc.lpszExtraInfo && uc.dwExtraInfoLength)
        path.append(uc.lpszExtraInfo, uc.dwExtraInfoLength);

    HINTERNET hSession = WinHttpOpen(L"WinHTTPGuiDemo/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        AppendLog(L"[错误] WinHttpOpen 失败");
        return;
    }
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
                                        uc.nPort, 0);
    if (!hConnect) {
        AppendLog(L"[错误] WinHttpConnect 失败");
        WinHttpCloseHandle(hSession);
        return;
    }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
                                            path.c_str(),
                                            nullptr,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) {
        AppendLog(L"[错误] WinHttpOpenRequest 失败");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }
    BOOL ok = WinHttpSendRequest(hRequest,
                                 L"Content-Type: application/octet-stream",
                                 (DWORD)-1,
                                 (LPVOID)body.data(),
                                 (DWORD)(body.size() * sizeof(wchar_t)),
                                 (DWORD)(body.size() * sizeof(wchar_t)),
                                 0);
    if (!ok) {
        AppendLog(L"[错误] WinHttpSendRequest 失败");
    } else {
        WinHttpReceiveResponse(hRequest, nullptr);
        DWORD status = 0, sz = sizeof(status);
        WinHttpQueryHeaders(hRequest,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            nullptr, &status, &sz, nullptr);
        AppendLog(L"[状态码] " + std::to_wstring(status));
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            std::string chunk(bytesAvailable, 0);
            DWORD bytesRead = 0;
            WinHttpReadData(hRequest, &chunk[0], bytesAvailable, &bytesRead);
            int wlen = MultiByteToWideChar(CP_UTF8, 0, chunk.data(), bytesRead, nullptr, 0);
            std::wstring wchunk(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, chunk.data(), bytesRead, &wchunk[0], wlen);
            AppendLog(L"[响应] " + wchunk);
        }
    }
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}
static lua_State* L = nullptr;
static std::map<int, std::string> btn_callbacks;
// 根据 lua 的 buttons 表创建所有按钮，并收集回调名
void LoadButtonsFromLua(HWND hwnd) {
    // 1) 新建并初始化 Lua
    L = luaL_newstate();
    luaL_openlibs(L);
    if (luaL_dofile(L, "ui_config.lua") != LUA_OK) {
        MessageBoxA(hwnd, lua_tostring(L, -1), "Lua 载入错误", MB_OK | MB_ICONERROR);
        return;
    }

    // 2) 获取全局 buttons
    lua_getglobal(L, "buttons");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    // 3) 遍历 buttons 表：1..#buttons
    int len = (int)luaL_len(L, -1);
    HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    for (int i = 1; i <= len; ++i) {
        lua_geti(L, -1, i);           // stack: ... buttons tbl buttons[i]
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            continue;
        }

        // 读出各字段
        lua_getfield(L, -1, "label");
        const char* label = lua_tostring(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "id");
        int id = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "x");
        int x = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "y");
        int y = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "w");
        int w = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "h");
        int h = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "action");
        const char* action = lua_tostring(L, -1);
        lua_pop(L, 1);
        // 4) CreateWindow 创建按钮
        HWND hb = CreateWindowW(
            L"BUTTON",
            utf8_to_wstring(label).c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            x, y, w, h,
            hwnd,
            (HMENU)id,
            GetModuleHandle(nullptr),
            nullptr
        );
        SendMessageW(hb, WM_SETFONT, (WPARAM)hf, TRUE);
        // 5) 收集回调名，供 WM_COMMAND 时使用
        if (action) {
            btn_callbacks[id] = action;
        }
        lua_pop(L, 1); // 弹出 buttons[i] 表
    }
    lua_pop(L, 1); // 弹出 buttons 整体
}
static HWND hGroupDesc, hDescEdit;
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
	    {
        LoadButtonsFromLua(hwnd);
        const int M = 10;             // 外间距
        const int CH = 24;   // 控件高度
        RECT rc;
        GetClientRect(hwnd, &rc);
        HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        // 只读多行 Edit 作为说明面板
        hDescEdit = CreateWindowExW(0, L"STATIC",
            L"在这里填写工具的使用说明，支持多行文本显示…\n在这里填写工具的使用说明，支持多行文本显示…\n在这里填写工具的使用说明，支持多行文本显示…\n在这里填写工具的使用说明，支持多行文本显示…\n",
            WS_CHILD
            | WS_VISIBLE
            | SS_LEFT            // 靠左对齐
            | SS_NOPREFIX        // 禁止 '&' 被当成快捷键
            ,M, 100,
            rc.right - 2 * M, 60,
            hwnd, nullptr, hInst, nullptr);

        // 字体
        SendMessageW(hGroupDesc, WM_SETFONT, (WPARAM)hf, TRUE);
        SendMessageW(hDescEdit, WM_SETFONT, (WPARAM)hf, TRUE);

        // 日志区
        int y4 = 50 + CH + M;
        hLogEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL | WS_CLIPSIBLINGS,
            M, 174,
            rc.right - 2 * M, rc.bottom - y4 - M,
            hwnd, (HMENU)ID_LOG_EDIT, hInst, nullptr
        );
	    }break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            int id = LOWORD(wParam);
            auto it = btn_callbacks.find(id);
            if (it != btn_callbacks.end()) {

            }
        }break;
    case WM_DESTROY:
        DeleteObject(hGuiFont);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;                 // 大小改变时重绘
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);             // 用系统 “窗口背景色” 画刷
    wc.lpszClassName = L"MyHttpSenderClass";
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPED       // 标准顶端标题栏
        | WS_CAPTION         // 标题、边框
        | WS_SYSMENU         // 系统菜单（右上角的 X）
        | WS_MINIMIZEBOX     // 允许最小化
        // <-- 删掉 WS_THICKFRAME（可调大小边框）
		//| WS_MAXIMIZEBOX     // 如果你也不想要最大化按钮，可以不加
		| WS_CLIPCHILDREN;   // 保持你原来的子窗口剪裁

    HWND hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"后端配置提交工具",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr, nullptr, hInstance, nullptr
    );
    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
