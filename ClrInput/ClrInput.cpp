#include "pch.h"

#include "ClrInput.h"
#include <userenv.h>
#include <Wtsapi32.h>
#include <msclr/marshal_cppstd.h>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Userenv.lib")
using namespace System::Runtime::InteropServices;

bool ClrInput::Process::SendMessageToActiveWindow(System::String^ title, System::String^ message) {
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    // 将 System::String 转换为 LPWSTR
    System::IntPtr titlePtr = Marshal::StringToHGlobalUni(title);
    System::IntPtr messagePtr = Marshal::StringToHGlobalUni(message);
    // 获取指向 LPWSTR 的指针
    LPWSTR lpTitle = static_cast<LPWSTR>(titlePtr.ToPointer());
    LPWSTR lpMessage = static_cast<LPWSTR>(messagePtr.ToPointer());
    DWORD result;
    // 调用 WTSSendMessage 发送消息
    return WTSSendMessage(
        WTS_CURRENT_SERVER_HANDLE,
        sessionId,
        lpTitle,
        (DWORD)wcslen(lpTitle) * sizeof(DWORD),
        lpMessage,
        (DWORD)wcslen(lpMessage) * sizeof(DWORD),
        MB_OK,
        0,
        &result,
        false);
}

bool ClrInput::Process::GetSessionUserToken(PHANDLE phUserToken) {
    bool result = false;
    DWORD count = 0;
    DWORD activeSessionId = 0xFFFFFFFF;
    PWTS_SESSION_INFOW pSessionInfo;
    HANDLE hToken;
    ZeroMemory(&hToken, sizeof(hToken));
    if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &count))
    {
        if (count > 0) {
            activeSessionId = pSessionInfo[count - 1].SessionId;
        }
    }
    if (activeSessionId == 0xFFFFFFFF)
    {
        activeSessionId = WTSGetActiveConsoleSessionId();
    }
    if (WTSQueryUserToken(activeSessionId, &hToken))
    {
        LPSECURITY_ATTRIBUTES sa = new SECURITY_ATTRIBUTES();
        result = DuplicateTokenEx(hToken, 0, sa, SecurityImpersonation, TokenPrimary, phUserToken);
        CloseHandle(hToken);
    }
    return result;
}

bool ClrInput::Process::StartProcessAsCurrentUser(System::String^ appPath, System::String^ cmdLineArgs, System::String^ workDir, bool visible) {
    PROCESS_INFORMATION procInfo;
    ZeroMemory(&procInfo, sizeof(procInfo));
    HANDLE hUserToken;
    ZeroMemory(&hUserToken, sizeof(hUserToken));
    LPVOID pEnv;
    ZeroMemory(&pEnv, sizeof(pEnv));
    bool result = false;
    try {
        if (!GetSessionUserToken(&hUserToken))
        {
            DWORD err = GetLastError();
            throw gcnew System::Exception(
                "[" + err + "]" + "StartProcessAsCurrentUser: GetSessionUserToken failed."
            );
        }

        if (!CreateEnvironmentBlock(&pEnv, hUserToken, false))
        {
            DWORD err = GetLastError();
            throw gcnew System::Exception(
                "[" + err + "]" + "StartProcessAsCurrentUser: CreateEnvironmentBlock failed."
            );
        }
        STARTUPINFO  startInfo;
        ZeroMemory(&startInfo, sizeof(startInfo));
        startInfo.cb = sizeof(startInfo);
        startInfo.lpDesktop = L"winsta0\\default";

        System::IntPtr cmdLinePtr = Marshal::StringToHGlobalUni(cmdLineArgs);
        LPWSTR lpCmdLine = static_cast<LPWSTR>(cmdLinePtr.ToPointer());

        msclr::interop::marshal_context context; // 创建一个上下文用于转换
        LPCWSTR lpAppPath = context.marshal_as<LPCWSTR>(appPath);
        LPCWSTR lpworkDir = context.marshal_as<LPCWSTR>(workDir);

        DWORD dwCreationFlags =
            CREATE_UNICODE_ENVIRONMENT | (visible
            ? CREATE_NEW_CONSOLE
            : CREATE_NO_WINDOW);
        if (
            !CreateProcessAsUser(
            hUserToken,
            lpAppPath, // Application Name
            lpCmdLine, // Command Line
            nullptr,
            nullptr,
            false,
            dwCreationFlags,
            pEnv,
            lpworkDir, // Working directory
            &startInfo,
            &procInfo
            )
            )
        {
            DWORD err = GetLastError();
            throw gcnew System::Exception(
                "[" + err + "]" + "StartProcessAsCurrentUser: CreateProcessAsUser failed."
            );
        }
        System::Runtime::InteropServices::Marshal::FreeHGlobal(cmdLinePtr);
        result = true;
    }
    finally {
        CloseHandle(hUserToken);
        if (pEnv != nullptr)
        {
            DestroyEnvironmentBlock(pEnv);
        }
        CloseHandle(procInfo.hThread);
        CloseHandle(procInfo.hProcess);
    }
    return result;
}