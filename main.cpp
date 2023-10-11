
#define _CRT_SECURE_NO_WARNINGS 
#include <Windows.h>
#include <vector>
#include <shlwapi.h>
#include "wke.h"
#include <xstring>
#include <locale>
#include <codecvt>


typedef struct {
    wkeWebView window;
    std::wstring url;
} Application;

Application app;

bool isOneInstance()
{
    HANDLE mutex = CreateMutexW(NULL, TRUE, L"Hello Miniblink");
    if ((mutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS)) {
        ReleaseMutex(mutex);
        return false;
    }
    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void handleDocumentReady(wkeWebView webWindow, void* param)
{
    wkeShowWindow(webWindow, true);
}

void readJsFile(const wchar_t* path, std::vector<char>* buffer)
{
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        DWORD dwError = GetLastError();
        DebugBreak();
        return;
    }

    DWORD fileSizeHigh;
    const DWORD bufferSize = ::GetFileSize(hFile, &fileSizeHigh);

    DWORD numberOfBytesRead = 0;
    buffer->resize(bufferSize);
    BOOL b = ::ReadFile(hFile, &buffer->at(0), bufferSize, &numberOfBytesRead, nullptr);
    ::CloseHandle(hFile);
    b = b;
}

jsValue onMsg(jsExecState es, void* param)
{
    int argCount = jsArgCount(es);
    if (argCount < 1)
        return jsUndefined();

    jsType type = jsArgType(es, 0);
    if (JSTYPE_STRING != type)
        return jsUndefined();

    jsValue arg0 = jsArg(es, 0);
    std::string msgOutput = "eMsg:";
    std::string msg = jsToTempString(es, arg0);
    msgOutput = msgOutput + msg;
    msgOutput += "\n";
    OutputDebugStringA(msgOutput.c_str());

    return jsUndefined();
}

bool createWebWindow(Application* app)
{
    app->window = wkeCreateWebWindow(WKE_WINDOW_TYPE_TRANSPARENT, NULL, 0, 0, 640, 480);
    if (!app->window)
        return false;

    wkeOnDocumentReady(app->window, handleDocumentReady, app);


    wkeJsBindFunction("eMsg", &onMsg, nullptr, 5);
    wkeMoveToCenter(app->window);
    wkeLoadURLW(app->window, app->url.c_str());

    return true;
}

void runApp(Application* app)
{
    memset(app, 0, sizeof(Application));
    app->url = L"https://party.163.com/?from=nietop/";
    if (!createWebWindow(app)) {
        PostQuitMessage(0);
        return;
    }
}


int WINAPI wWinMain(
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in LPWSTR lpCmdLine,
    __in int nCmdShow
)
{
    if (!isOneInstance()) {
        ::MessageBoxA(NULL, "该进程已经启动", "错误", MB_OK);
        return 0;
    }

    WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance,
                    NULL,
                    LoadCursor(NULL, IDC_ARROW), NULL, NULL,
                    L"hello"};
    if (!RegisterClass(&wc))
        return FALSE;

    // Create the main window
    HWND hWnd = CreateWindow(
        L"Hello",
        L"",
        WS_POPUP | WS_BORDER,	//WS_OVERLAPPED|WS_SYSMENU,	//WS_OVERLAPPEDWINDOW,
        0, 0, 400, 300,
        NULL,
        NULL,
        hInstance,
        0);

    // 初始化窗口大小
    const int SplashWindowSizeW = 400;
    const int SplashWindowSizeH = 300;

    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    const int appPosX = (screenWidth - SplashWindowSizeW) / 2;
    const int appPosY = (screenHeight - SplashWindowSizeH) / 2;
    MoveWindow(hWnd, appPosX, appPosY, SplashWindowSizeW, SplashWindowSizeH, true);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    std::vector<char> tempPath;
    tempPath.resize(MAX_PATH);

    ::GetModuleFileNameA(nullptr, &tempPath[0], MAX_PATH);
    ::PathRemoveFileSpecA(&tempPath[0]);

    std::vector<char> mbPath = tempPath;

    ::PathAppendA(&mbPath[0], "miniblink_4975_x64.dll");
    if (!::PathFileExistsA(&mbPath[0])) {
        ::PathAppendA(&tempPath[0], "..\\..\\");
        mbPath = tempPath;
        ::PathAppendA(&mbPath[0], "miniblink_4975_x64.dll");
        if (!::PathFileExistsA(&mbPath[0])) {
            ::MessageBoxA(NULL, "请把miniblink_4975_x64.dll放exe目录下", "错误", MB_OK);
            return 0;
        }
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wideStr = converter.from_bytes(mbPath.data(), mbPath.data() + mbPath.size());
    std::vector<wchar_t> wcharVec(wideStr.begin(), wideStr.end());

    wkeSetWkeDllPath(&wideStr[0]);
    wkeInitialize();

    // Test
    runApp(&app);

    MSG msg = { 0 };
    while (1) {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

    }

    wkeFinalize();
    return 0;

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}