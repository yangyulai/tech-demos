// main.cpp
#include <windows.h>
#include <commdlg.h>
#include <winhttp.h>
#include <string>
#include <fstream>

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // URL 输入
        CreateWindowW(L"STATIC", L"URL:", WS_CHILD|WS_VISIBLE,
                      10, 10, 30, 20, hWnd, nullptr, hInst, nullptr);
        hUrlEdit = CreateWindowW(L"EDIT", L"https://example.com/api",
                                 WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT,
                                 50, 10, 580, 20, hWnd, (HMENU)ID_URL_EDIT, hInst, nullptr);
        // 文件选择
        hBrowseBtn = CreateWindowW(L"BUTTON", L"...", WS_CHILD|WS_VISIBLE,
                                   460, 40, 30, 20, hWnd, (HMENU)ID_BROWSE_BUTTON, hInst, nullptr);
        hFileEdit = CreateWindowW(L"EDIT", L"",
                                 WS_CHILD|WS_VISIBLE|WS_BORDER|ES_READONLY,
                                 10, 40, 440, 20, hWnd, (HMENU)ID_FILE_EDIT, hInst, nullptr);
        // 按钮：发送 / 清空
        hSendBtn  = CreateWindowW(L"BUTTON", L"发送 HTTP",
                                  WS_CHILD|WS_VISIBLE,
                                  10, 70, 100, 25, hWnd, (HMENU)ID_SEND_BUTTON, hInst, nullptr);
        hClearBtn = CreateWindowW(L"BUTTON", L"清空日志",
                                  WS_CHILD|WS_VISIBLE,
                                  120, 70, 100, 25, hWnd, (HMENU)ID_CLEAR_BUTTON, hInst, nullptr);
        // 日志输出框
        hLogEdit = CreateWindowW(L"EDIT", nullptr,
                                 WS_CHILD|WS_VISIBLE|WS_BORDER|
                                 ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
                                 10, 110, 480, 280, hWnd, (HMENU)ID_LOG_EDIT, hInst, nullptr);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BROWSE_BUTTON: {
            WCHAR szFile[MAX_PATH] = {0};
            OPENFILENAMEW ofn = { sizeof(ofn) };
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"All Files\0*.*\0";
            if (GetOpenFileNameW(&ofn)) {
                SetWindowTextW(hFileEdit, szFile);
            }
            break;
        }
        case ID_CLEAR_BUTTON:
            SetWindowTextW(hLogEdit, L"");
            break;
        case ID_SEND_BUTTON: {
            // 读取 URL 和 文件
            WCHAR url[512] = {0};
            GetWindowTextW(hUrlEdit, url, _countof(url));
            WCHAR filePath[MAX_PATH] = {0};
            GetWindowTextW(hFileEdit, filePath, _countof(filePath));
            std::wstring body;
            if (filePath[0]) {
                body = ReadFileToWString(filePath);
            }
            AppendLog(L"[开始请求] " + std::wstring(url));
            DoHttpRequest(url, body);
            break;
        }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSW wc = { };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"WinHTTPGuiDemo";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);
    HWND hWnd = CreateWindowW(wc.lpszClassName, L"HTTP 请求发送器",
                              WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              800, 600, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
