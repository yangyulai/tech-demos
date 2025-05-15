// MainWindow.cpp
#include "MainWindow.h"
#include <commdlg.h>
#include <winhttp.h>
#include <sstream>

// 静态成员定义
constexpr wchar_t MainWindow::CLASS_NAME_[];

// UTF‑8 宽字符转换
static std::wstring utf8_to_wstring(const std::string& u8) {
    if (u8.empty()) return {};
    int sz = MultiByteToWideChar(CP_UTF8, 0, u8.data(), (int)u8.size(), nullptr, 0);
    std::wstring out(sz, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8.data(), (int)u8.size(), &out[0], sz);
    return out;
}

MainWindow::MainWindow(HINSTANCE hInst)
  : hInst_(hInst), hwnd_(nullptr), L_(nullptr) {
    LOGFONTW lf = {};
    wcscpy_s(lf.lfFaceName, L"Microsoft YaHei UI");
    // 把高度设大（单位：逻辑像素），> 实际字体点大小
    lf.lfHeight = -12;
    font_ = CreateFontIndirectW(&lf);
}

MainWindow::~MainWindow() {
    if (L_) lua_close(L_);
    if (font_) DeleteObject(font_);
}

bool MainWindow::Initialize(int nCmdShow) {
    RegisterWindowClass();
    CreateWindowInstance(nCmdShow);
    return hwnd_ != nullptr;
}

int MainWindow::Run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

void MainWindow::RegisterWindowClass() {
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProcThunk;
    wc.hInstance     = hInst_;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME_;
    RegisterClassExW(&wc);
}

void MainWindow::CreateWindowInstance(int nCmdShow) {
    DWORD style =
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    hwnd_ = CreateWindowExW(
          0, CLASS_NAME_, L"后端配置提交工具",
          style,
          CW_USEDEFAULT, CW_USEDEFAULT, 900, 800,
          nullptr, nullptr, hInst_, this  // 把 this 传给 WM_CREATE
    );
    if (hwnd_) {
        ShowWindow(hwnd_, nCmdShow);
        UpdateWindow(hwnd_);
    }
}

void MainWindow::CreateControls() {
    RECT rc; GetClientRect(hwnd_, &rc);
    const int M = 10, CH = 24;

    // 工具说明静态文本
    hDescStatic_ = CreateWindowW(L"STATIC",
        descText_.c_str(),
        WS_CHILD|WS_VISIBLE|SS_LEFT,
        M, CH + 9*M, rc.right - 2*M, 80,
        hwnd_, nullptr, hInst_, nullptr);
    SendMessageW(hDescStatic_, WM_SETFONT, (WPARAM)font_, TRUE);

    // 日志区
    hLogEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY|WS_VSCROLL,
        M, CH + 18 * M,
        rc.right - 2*M, 550,
        hwnd_, (HMENU)1006, hInst_, nullptr);
    SendMessageW(hLogEdit_, WM_SETFONT, (WPARAM)font_, TRUE);
}

void MainWindow::LoadButtonsFromLua() {
    L_ = luaL_newstate();
    luaL_openlibs(L_);
    if (luaL_dofile(L_, "ui_config.lua") != LUA_OK) {
        MessageBoxA(hwnd_, lua_tostring(L_,-1), "Lua 载入错误", MB_OK|MB_ICONERROR);
        return;
    }

    lua_getglobal(L_, "buttons");
    if (!lua_istable(L_, -1)) { lua_pop(L_,1); return; }
    int len = static_cast<int>(luaL_len(L_, -1));

    for (int i = 1; i <= len; ++i) {
        lua_geti(L_, -1, i);
        if (!lua_istable(L_, -1)) { lua_pop(L_,1); continue; }

        // 读取字段
        lua_getfield(L_,-1,"label");    auto label = lua_tostring(L_,-1); lua_pop(L_,1);
        lua_getfield(L_,-1,"id");       int  id    = (int)lua_tointeger(L_,-1); lua_pop(L_,1);
        lua_getfield(L_,-1,"x");        int  x     = (int)lua_tointeger(L_,-1); lua_pop(L_,1);
        lua_getfield(L_,-1,"y");        int  y     = (int)lua_tointeger(L_,-1); lua_pop(L_,1);
        lua_getfield(L_,-1,"w");        int  w     = (int)lua_tointeger(L_,-1); lua_pop(L_,1);
        lua_getfield(L_,-1,"h");        int  h     = (int)lua_tointeger(L_,-1); lua_pop(L_,1);
        lua_getfield(L_,-1,"action");   auto act   = lua_tostring(L_,-1); lua_pop(L_,1);

        // 创建按钮
        HWND btn = CreateWindowW(
            L"BUTTON", utf8_to_wstring(label).c_str(),
            WS_CHILD|WS_VISIBLE|WS_TABSTOP,
            x, y, w, h,
            hwnd_, (HMENU)id, hInst_, nullptr
        );
        SendMessageW(btn, WM_SETFONT, (WPARAM)font_, TRUE);

        if (act) btnCallbacks_[id] = act;
        lua_pop(L_,1);
    }
    lua_pop(L_,1);

    lua_getglobal(L_, "describe");
    if (!lua_isstring(L_, -1)) { lua_pop(L_, 1); return; }
    descText_ = utf8_to_wstring(lua_tostring(L_, -1));
	lua_pop(L_, 1);
}

void MainWindow::AppendLog(const std::wstring& text) {
    int len = GetWindowTextLengthW(hLogEdit_);
    SendMessageW(hLogEdit_, EM_SETSEL, len, len);
    SendMessageW(hLogEdit_, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(hLogEdit_, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

LRESULT CALLBACK MainWindow::WndProcThunk(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    MainWindow* self = nullptr;
    if (msg == WM_CREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lp);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        self->hwnd_ = hwnd;
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (self) return self->WndProc(hwnd,msg,wp,lp);
    return DefWindowProc(hwnd,msg,wp,lp);
}

LRESULT MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:    return OnCreate();
    case WM_COMMAND:   return OnCommand(wp);
    case WM_DESTROY:   return OnDestroy();
    }
    return DefWindowProc(hwnd,msg,wp,lp);
}

LRESULT MainWindow::OnCreate() {
    LoadButtonsFromLua();
    CreateControls();
    return 0;
}

LRESULT MainWindow::OnCommand(WPARAM wParam) {
    int id = LOWORD(wParam);
    auto it = btnCallbacks_.find(id);
    if (it != btnCallbacks_.end()) {
        if (id== 4016)
        {
            SetWindowTextW(hLogEdit_, L"");
            return 0;
        }
        if (it->second.empty())
        {
            for (auto& [btnId,name]:btnCallbacks_)
            {
                if (name.empty()) continue;
                std::wstring request = L"http://172.16.1.237:8080/to_json?name=" + utf8_to_wstring(name);
                //std::wstring request = L"http://127.0.0.1:8080/to_json?name=" + utf8_to_wstring(name);
                DoHttpRequest(request, L"");
            }
            return 0;
        }else
        {
            std::wstring request = L"http://172.16.1.237:8080/to_json?name=" + utf8_to_wstring(it->second);
            //std::wstring request = L"http://127.0.0.1:8080/to_json?name=" + utf8_to_wstring(it->second);
            DoHttpRequest(request, L"");
        }
    }
    return 0;
}

LRESULT MainWindow::OnDestroy() {
    PostQuitMessage(0);
    return 0;
}
void MainWindow::DoHttpRequest(const std::wstring& url, const std::wstring& body)
{
    URL_COMPONENTS uc = { sizeof(uc) };
    uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUrlPathLength = uc.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &uc)) {
        AppendLog(L"[错误] 无效 URL");
        return;
    }
    std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
    std::wstring path(uc.lpszUrlPath, uc.dwUrlPathLength);
    if (uc.lpszExtraInfo) path += std::wstring(uc.lpszExtraInfo, uc.dwExtraInfoLength);

    auto session = WinHttpOpen(L"WinHTTPGuiDemo/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) { AppendLog(L"[错误] WinHttpOpen 失败"); return; }
    auto connect = WinHttpConnect(session, host.c_str(), uc.nPort, 0);
    if (!connect) { AppendLog(L"[错误] WinHttpConnect 失败"); WinHttpCloseHandle(session); return; }
    auto request = WinHttpOpenRequest(connect, L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!request) { AppendLog(L"[错误] WinHttpOpenRequest 失败"); WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return; }

    BOOL ok = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        nullptr, 0, 0, 0);
    if (!ok) AppendLog(L"[错误] WinHttpSendRequest 失败");
    else {
        WinHttpReceiveResponse(request, nullptr);
        DWORD status = 0, sz = sizeof(status);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            nullptr, &status, &sz, nullptr);
        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(request, &avail) && avail > 0) {
            std::string chunk(avail, 0);
            DWORD read = 0;
            WinHttpReadData(request, &chunk[0], avail, &read);
            AppendLog(utf8_to_wstring(chunk));
        }
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
}