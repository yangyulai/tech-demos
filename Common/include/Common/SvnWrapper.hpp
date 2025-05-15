#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <format>
#include <windows.h>

class SVNController {
public:
    enum class Operation {
        Checkout,
        Update,
        Commit,
        Add,
        Delete,
        Status,
        Log,
        Info,
        Revert,
        Cleanup
    };

    struct Result {
        bool success;
        int exitCode;
        std::wstring output;
        std::wstring error;
    };

    explicit SVNController(std::wstring_view svnPath = L"svn");

    Result executeCommand(Operation op,
        const std::vector<std::wstring>& args,
        const std::wstring& workingDir = L"") const
    {
        std::wstring command = buildCommand(op, args);
        return executeSystemCommand(command, workingDir);
    }

    Result checkout(const std::wstring& url,
        const std::wstring& targetPath,
        const std::wstring& revision = L"") const
    {
        std::vector<std::wstring> args = { L"checkout", url, targetPath };
        if (!revision.empty()) {
            args.emplace_back(L"-r");
            args.push_back(revision);
        }
        return executeCommand(Operation::Checkout, args, targetPath);
    }

    Result update(const std::wstring& path = L".",
        const std::wstring& revision = L"") {
        std::vector<std::wstring> args = { L"update", path };
        if (!revision.empty()) {
            args.emplace_back(L"-r");
            args.push_back(revision);
        }
        return executeCommand(Operation::Update, args, path);
    }

    Result commit(const std::wstring& message,
        const std::wstring& path = L".") {
        std::vector<std::wstring> args = { L"commit", L"-m", quoteArg(message), path };
        return executeCommand(Operation::Commit, args, path);
    }

private:
    std::wstring svnExecutable;
    std::wstring logBuffer;

    std::wstring buildCommand(Operation op,
        const std::vector<std::wstring>& args) const
    {
        std::wostringstream cmd;
        cmd << svnExecutable << L" ";

        switch (op) {
        case Operation::Checkout: cmd << L"checkout"; break;
        case Operation::Update:   cmd << L"update"; break;
        case Operation::Commit:   cmd << L"commit"; break;
        case Operation::Add:
	        break;
        case Operation::Delete:
	        break;
        case Operation::Status:
	        break;
        case Operation::Log:
	        break;
        case Operation::Info:
	        break;
        case Operation::Revert:
	        break;
        case Operation::Cleanup:
	        break;
        }

        for (const auto& arg : args) {
            cmd << L" " << quoteArg(arg);
        }

        return cmd.str();
    }

    static Result executeSystemCommand(const std::wstring& command,
                                       const std::wstring& workingDir) {
        Result result;
        std::wstring fullCommand = command;

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        HANDLE hOutputRead, hErrorRead;
        HANDLE hOutputWrite, hErrorWrite;
        CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0);
        CreatePipe(&hErrorRead, &hErrorWrite, &sa, 0);

        STARTUPINFO si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hOutputWrite;
        si.hStdError = hErrorWrite;

        PROCESS_INFORMATION pi;
        CreateProcess(NULL, const_cast<LPWSTR>(fullCommand.c_str()),
            NULL, NULL, TRUE, 0, NULL,
            workingDir.empty() ? NULL : workingDir.c_str(),
            &si, &pi);

        CloseHandle(hOutputWrite);
        CloseHandle(hErrorWrite);

        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, reinterpret_cast<LPDWORD>(&result.exitCode));
        result.success = (result.exitCode == 0);
        return result;
    }

    static std::wstring quoteArg(const std::wstring& arg) {
        if (arg.find_first_of(L" \t\n\"") == std::wstring::npos) {
            return arg;
        }
        return L"\"" + arg + L"\"";
    }

    void log(const std::wstring& message) {
        logBuffer += L"[" + getTimestamp() + L"] " + message + L"\n";
    }

    static std::wstring getTimestamp() {
        auto now = std::chrono::system_clock::now();
        return std::format(L"{:%Y-%m-%d %H:%M:%S}", now);
    }
};