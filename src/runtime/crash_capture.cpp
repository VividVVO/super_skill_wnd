#include "runtime/crash_capture.h"

#include "core/Common.h"
#include "runtime/feature_switches.h"

#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <exception>
#include <intrin.h>
#include <signal.h>
#include <stdlib.h>

namespace ssw
{
namespace runtime
{
namespace
{
    typedef BOOL(WINAPI* MiniDumpWriteDumpFn)(
        HANDLE,
        DWORD,
        HANDLE,
        MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION);

    static const DWORD kMsVcThreadNameException = 0x406D1388;
    static const DWORD kStatusInvalidCrtParameter = 0xC0000417;
    static const DWORD kCrashCodePureCall = 0xE0001001;
    static const DWORD kCrashCodeTerminate = 0xE0001002;
    static const DWORD kCrashCodeSigAbort = 0xE0001003;
    static const UINT kAbortBehaviorMask = _WRITE_ABORT_MSG | _CALL_REPORTFAULT;

    PVOID g_crashCaptureVectoredHandle = nullptr;
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousUnhandledExceptionFilter = nullptr;
    LONG g_crashCaptureBootstrapInitialized = 0;
    LONG g_crashCaptureInfrastructureInstalled = 0;
    LONG g_crashCaptureCrtHandlersInstalled = 0;
    LONG g_crashCaptureEnabled = 0;
    LONG g_crashCaptureWriting = 0;
    LONG g_crashCaptureCandidateLogBudget = 8;
    LONG g_crashCaptureCandidateArtifactBudget = 16;
    LONG g_savedErrorModeValid = 0;
    UINT g_savedErrorMode = 0;
    LONG g_savedAbortBehaviorValid = 0;
    UINT g_savedAbortBehavior = 0;
    wchar_t g_crashOutputDirectory[MAX_PATH] = {};
    HMODULE g_dbgHelpModule = nullptr;
    MiniDumpWriteDumpFn g_miniDumpWriteDump = nullptr;
    wchar_t g_dbgHelpModulePath[MAX_PATH] = {};
    _invalid_parameter_handler g_previousInvalidParameterHandler = nullptr;
    _purecall_handler g_previousPureCallHandler = nullptr;
    std::terminate_handler g_previousTerminateHandler = nullptr;
    typedef void(__cdecl* SignalHandlerFn)(int);
    SignalHandlerFn g_previousSigAbortHandler = SIG_DFL;

    struct DumpWriteDiagnostics
    {
        bool attempted = false;
        bool wroteDump = false;
        DWORD resolveDbgHelpError = 0;
        DWORD createFileError = 0;
        DWORD primaryWriteError = 0;
        DWORD fallbackWriteError = 0;
        unsigned long long fileSize = 0;
    };

    void InstallCrashCaptureCoreInfrastructure();
    void InstallCrashCaptureCrtHandlers();
    LONG WINAPI CrashCaptureUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers);
    LONG CALLBACK CrashCaptureVectoredHandler(EXCEPTION_POINTERS* exceptionPointers);
    void WriteCrashArtifacts(EXCEPTION_POINTERS* exceptionPointers, const char* captureSource, bool attemptDump);
    void CaptureSyntheticCrash(
        DWORD exceptionCode,
        const void* exceptionAddress,
        ULONG parameterCount,
        const ULONG_PTR* parameters,
        const char* captureSource);
    [[noreturn]] void CrashCaptureTerminateProcess(UINT exitCode);

    bool CopyWideText(const wchar_t* source, wchar_t* destination, size_t destinationCount)
    {
        if (!destination || destinationCount == 0)
            return false;

        destination[0] = L'\0';
        if (!source || !source[0])
            return false;

        return wcscpy_s(destination, destinationCount, source) == 0;
    }

    void TrimTrailingSlashInPlace(wchar_t* path)
    {
        if (!path)
            return;

        size_t length = wcslen(path);
        while (length > 0 && (path[length - 1] == L'\\' || path[length - 1] == L'/'))
        {
            path[length - 1] = L'\0';
            --length;
        }
    }

    bool GetParentDirectoryInPlace(wchar_t* path)
    {
        if (!path || !path[0])
            return false;

        TrimTrailingSlashInPlace(path);
        const size_t length = wcslen(path);
        if (length == 0)
            return false;

        for (size_t i = length; i > 0; --i)
        {
            if (path[i - 1] == L'\\' || path[i - 1] == L'/')
            {
                if (i == 3 && path[1] == L':')
                {
                    path[i] = L'\0';
                    return true;
                }

                path[i - 1] = L'\0';
                return i > 1;
            }
        }

        return false;
    }

    bool DirectoryExistsWide(const wchar_t* path)
    {
        if (!path || !path[0])
            return false;

        const DWORD attributes = ::GetFileAttributesW(path);
        return attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    bool CombinePathWide(const wchar_t* left, const wchar_t* right, wchar_t* outPath, size_t outPathCount)
    {
        if (!outPath || outPathCount == 0)
            return false;

        outPath[0] = L'\0';
        if (!left || !left[0])
            return false;
        if (!right || !right[0])
            return CopyWideText(left, outPath, outPathCount);

        wchar_t base[MAX_PATH] = {};
        if (!CopyWideText(left, base, MAX_PATH))
            return false;

        TrimTrailingSlashInPlace(base);
        return swprintf_s(outPath, outPathCount, L"%s\\%s", base, right) > 0;
    }

    bool EnsureDirectoryExistsRecursive(const wchar_t* path)
    {
        if (!path || !path[0])
            return false;

        if (DirectoryExistsWide(path))
            return true;

        wchar_t parent[MAX_PATH] = {};
        if (CopyWideText(path, parent, MAX_PATH) && GetParentDirectoryInPlace(parent) && parent[0])
            EnsureDirectoryExistsRecursive(parent);

        if (::CreateDirectoryW(path, nullptr))
            return true;

        const DWORD error = ::GetLastError();
        return error == ERROR_ALREADY_EXISTS && DirectoryExistsWide(path);
    }

    bool GetSelfModuleDirectory(wchar_t* outDirectory, size_t outDirectoryCount)
    {
        if (!outDirectory || outDirectoryCount == 0)
            return false;

        outDirectory[0] = L'\0';

        HMODULE module = nullptr;
        if (!::GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&GetSelfModuleDirectory),
                &module))
        {
            return false;
        }

        wchar_t modulePath[MAX_PATH] = {};
        const DWORD length = ::GetModuleFileNameW(module, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return false;

        if (!CopyWideText(modulePath, outDirectory, outDirectoryCount))
            return false;

        return GetParentDirectoryInPlace(outDirectory);
    }

    void WideToUtf8Buffer(const wchar_t* source, char* destination, int destinationCount)
    {
        if (!destination || destinationCount <= 0)
            return;

        destination[0] = '\0';
        if (!source || !source[0])
            return;

        const int written = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            source,
            -1,
            destination,
            destinationCount,
            nullptr,
            nullptr);
        if (written <= 0)
            destination[0] = '\0';
    }

    bool ResolveCrashOutputDirectory(wchar_t* outDirectory, size_t outDirectoryCount)
    {
        if (!outDirectory || outDirectoryCount == 0)
            return false;

        outDirectory[0] = L'\0';

        wchar_t moduleDirectory[MAX_PATH] = {};
        wchar_t candidate[MAX_PATH] = {};
        if (GetSelfModuleDirectory(moduleDirectory, MAX_PATH) &&
            CombinePathWide(moduleDirectory, L"CrashDumps", candidate, MAX_PATH) &&
            EnsureDirectoryExistsRecursive(candidate))
        {
            return CopyWideText(candidate, outDirectory, outDirectoryCount);
        }

        wchar_t tempDirectory[MAX_PATH] = {};
        const DWORD tempLength = ::GetTempPathW(MAX_PATH, tempDirectory);
        if (tempLength > 0 && tempLength < MAX_PATH)
        {
            TrimTrailingSlashInPlace(tempDirectory);
            if (CombinePathWide(tempDirectory, L"SuperSkillWnd\\CrashDumps", candidate, MAX_PATH) &&
                EnsureDirectoryExistsRecursive(candidate))
            {
                return CopyWideText(candidate, outDirectory, outDirectoryCount);
            }
        }

        return false;
    }

    bool ResolveDbgHelpModule()
    {
        if (g_dbgHelpModule && g_miniDumpWriteDump)
            return true;

        wchar_t systemDirectory[MAX_PATH] = {};
        wchar_t dbgHelpPath[MAX_PATH] = {};
        HMODULE dbgHelp = nullptr;

        const UINT systemLength = ::GetSystemDirectoryW(systemDirectory, MAX_PATH);
        if (systemLength > 0 &&
            systemLength < MAX_PATH &&
            CombinePathWide(systemDirectory, L"dbghelp.dll", dbgHelpPath, MAX_PATH))
        {
            dbgHelp = ::LoadLibraryW(dbgHelpPath);
        }

        if (!dbgHelp)
            dbgHelp = ::LoadLibraryW(L"dbghelp.dll");

        if (!dbgHelp)
            return false;

        MiniDumpWriteDumpFn writeDump = reinterpret_cast<MiniDumpWriteDumpFn>(
            ::GetProcAddress(dbgHelp, "MiniDumpWriteDump"));
        if (!writeDump)
        {
            ::FreeLibrary(dbgHelp);
            return false;
        }

        g_dbgHelpModule = dbgHelp;
        g_miniDumpWriteDump = writeDump;
        g_dbgHelpModulePath[0] = L'\0';
        ::GetModuleFileNameW(g_dbgHelpModule, g_dbgHelpModulePath, MAX_PATH);
        return true;
    }

    [[noreturn]] void CrashCaptureTerminateProcess(UINT exitCode)
    {
        ::TerminateProcess(::GetCurrentProcess(), exitCode);
        ::ExitProcess(exitCode);
    }

    void InstallCrashCaptureCoreInfrastructure()
    {
        if (InterlockedCompareExchange(&g_crashCaptureInfrastructureInstalled, 1, 0) != 0)
            return;

        if (InterlockedCompareExchange(&g_savedErrorModeValid, 1, 0) == 0)
            g_savedErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        else
            ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

        g_previousUnhandledExceptionFilter = ::SetUnhandledExceptionFilter(&CrashCaptureUnhandledExceptionFilter);
        g_crashCaptureVectoredHandle = ::AddVectoredExceptionHandler(1, &CrashCaptureVectoredHandler);
    }

    void __cdecl CrashCaptureInvalidParameterHandler(
        const wchar_t* expression,
        const wchar_t* function,
        const wchar_t* file,
        unsigned int line,
        uintptr_t reserved)
    {
        ULONG_PTR parameters[5] = {};
        parameters[0] = reinterpret_cast<ULONG_PTR>(expression);
        parameters[1] = reinterpret_cast<ULONG_PTR>(function);
        parameters[2] = reinterpret_cast<ULONG_PTR>(file);
        parameters[3] = static_cast<ULONG_PTR>(line);
        parameters[4] = static_cast<ULONG_PTR>(reserved);
        CaptureSyntheticCrash(
            kStatusInvalidCrtParameter,
            _ReturnAddress(),
            5,
            parameters,
            "invalid_parameter");

        if (g_previousInvalidParameterHandler &&
            g_previousInvalidParameterHandler != &CrashCaptureInvalidParameterHandler)
        {
            __try
            {
                g_previousInvalidParameterHandler(expression, function, file, line, reserved);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        CrashCaptureTerminateProcess(kStatusInvalidCrtParameter);
    }

    void __cdecl CrashCapturePureCallHandler()
    {
        CaptureSyntheticCrash(kCrashCodePureCall, _ReturnAddress(), 0, nullptr, "purecall");

        if (g_previousPureCallHandler &&
            g_previousPureCallHandler != &CrashCapturePureCallHandler)
        {
            __try
            {
                g_previousPureCallHandler();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        CrashCaptureTerminateProcess(kCrashCodePureCall);
    }

    void __cdecl CrashCaptureTerminateHandler()
    {
        CaptureSyntheticCrash(kCrashCodeTerminate, _ReturnAddress(), 0, nullptr, "terminate");

        if (g_previousTerminateHandler &&
            g_previousTerminateHandler != &CrashCaptureTerminateHandler)
        {
            __try
            {
                g_previousTerminateHandler();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        CrashCaptureTerminateProcess(kCrashCodeTerminate);
    }

    void __cdecl CrashCaptureAbortSignalHandler(int signalValue)
    {
        ULONG_PTR parameters[1] = {};
        parameters[0] = static_cast<ULONG_PTR>(signalValue);
        CaptureSyntheticCrash(kCrashCodeSigAbort, _ReturnAddress(), 1, parameters, "sigabrt");

        if (g_previousSigAbortHandler &&
            g_previousSigAbortHandler != SIG_DFL &&
            g_previousSigAbortHandler != SIG_IGN &&
            g_previousSigAbortHandler != &CrashCaptureAbortSignalHandler)
        {
            __try
            {
                g_previousSigAbortHandler(signalValue);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        CrashCaptureTerminateProcess(kCrashCodeSigAbort);
    }

    void InstallCrashCaptureCrtHandlers()
    {
        if (InterlockedCompareExchange(&g_crashCaptureCrtHandlersInstalled, 1, 0) != 0)
            return;

        if (InterlockedCompareExchange(&g_savedAbortBehaviorValid, 1, 0) == 0)
            g_savedAbortBehavior = _set_abort_behavior(0, kAbortBehaviorMask);
        else
            _set_abort_behavior(0, kAbortBehaviorMask);

        g_previousInvalidParameterHandler = _set_invalid_parameter_handler(&CrashCaptureInvalidParameterHandler);
        g_previousPureCallHandler = _set_purecall_handler(&CrashCapturePureCallHandler);
        g_previousTerminateHandler = std::set_terminate(&CrashCaptureTerminateHandler);
        g_previousSigAbortHandler = signal(SIGABRT, &CrashCaptureAbortSignalHandler);
    }

    bool ShouldLogVectoredCandidate(DWORD exceptionCode)
    {
        switch (exceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        case EXCEPTION_DATATYPE_MISALIGNMENT:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_ILLEGAL_INSTRUCTION:
        case EXCEPTION_IN_PAGE_ERROR:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        case EXCEPTION_PRIV_INSTRUCTION:
        case EXCEPTION_STACK_OVERFLOW:
        case 0xC0000374: // STATUS_HEAP_CORRUPTION
        case 0xC0000409: // STATUS_STACK_BUFFER_OVERRUN / fail-fast
        case 0xC0000602: // STATUS_FAIL_FAST_EXCEPTION
            return true;
        default:
            return false;
        }
    }

    bool ShouldCaptureImmediatelyInVectoredHandler(DWORD exceptionCode)
    {
        switch (exceptionCode)
        {
        case 0xC0000374: // STATUS_HEAP_CORRUPTION
        case 0xC0000409: // STATUS_STACK_BUFFER_OVERRUN / fail-fast
        case 0xC0000602: // STATUS_FAIL_FAST_EXCEPTION
            return true;
        default:
            return false;
        }
    }

    void WriteRawText(HANDLE file, const char* text)
    {
        if (!text || text[0] == '\0' || file == INVALID_HANDLE_VALUE)
            return;

        DWORD written = 0;
        ::WriteFile(file, text, (DWORD)strlen(text), &written, nullptr);
    }

    void WriteTextFormat(HANDLE file, const char* format, ...)
    {
        if (file == INVALID_HANDLE_VALUE || !format || !format[0])
            return;

        char buffer[1024] = {};
        va_list args;
        va_start(args, format);
        _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
        va_end(args);
        WriteRawText(file, buffer);
    }

    void WriteUtf8Bom(HANDLE file)
    {
        static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        DWORD written = 0;
        ::WriteFile(file, bom, sizeof(bom), &written, nullptr);
    }

    void WriteAddressRegionInfo(HANDLE file, const char* label, const void* address)
    {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (!address || ::VirtualQuery(address, &mbi, sizeof(mbi)) == 0)
        {
            WriteTextFormat(file, "%s region: unavailable address=0x%08X\r\n",
                label ? label : "Address",
                (unsigned int)(uintptr_t)address);
            return;
        }

        WriteTextFormat(file,
            "%s region: address=0x%08X base=0x%08X allocBase=0x%08X size=0x%08X protect=0x%08X state=0x%08X type=0x%08X\r\n",
            label ? label : "Address",
            (unsigned int)(uintptr_t)address,
            (unsigned int)(uintptr_t)mbi.BaseAddress,
            (unsigned int)(uintptr_t)mbi.AllocationBase,
            (unsigned int)mbi.RegionSize,
            (unsigned int)mbi.Protect,
            (unsigned int)mbi.State,
            (unsigned int)mbi.Type);
    }

    void WriteAddressModuleInfo(HANDLE file, const char* label, const void* address)
    {
        if (!address)
        {
            WriteTextFormat(file, "%s module: address=0x00000000\r\n", label ? label : "Address");
            return;
        }

        HMODULE module = nullptr;
        if (!::GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(address),
                &module))
        {
            WriteTextFormat(file, "%s module: unresolved address=0x%08X\r\n",
                label ? label : "Address",
                (unsigned int)(uintptr_t)address);
            return;
        }

        wchar_t modulePath[MAX_PATH] = {};
        char modulePathUtf8[MAX_PATH * 3] = {};
        ::GetModuleFileNameW(module, modulePath, MAX_PATH);
        WideToUtf8Buffer(modulePath, modulePathUtf8, (int)sizeof(modulePathUtf8));

        WriteTextFormat(file, "%s module: base=0x%08X address=0x%08X path=%s\r\n",
            label ? label : "Address",
            (unsigned int)(uintptr_t)module,
            (unsigned int)(uintptr_t)address,
            modulePathUtf8[0] ? modulePathUtf8 : "(unavailable)");
    }

    void WriteHexBytes(HANDLE file, const char* label, const void* address, size_t byteCount)
    {
        WriteTextFormat(file, "%s bytes @0x%08X:", label ? label : "Bytes", (unsigned int)(uintptr_t)address);

        if (!address || byteCount == 0 || SafeIsBadReadPtr(address, byteCount))
        {
            WriteRawText(file, " unreadable\r\n");
            return;
        }

        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(address);
        for (size_t i = 0; i < byteCount; ++i)
            WriteTextFormat(file, " %02X", (unsigned int)bytes[i]);
        WriteRawText(file, "\r\n");
    }

    void WriteStackDwords(HANDLE file, const CONTEXT* context)
    {
#if defined(_M_IX86)
        if (!context)
        {
            WriteRawText(file, "[Stack]\r\ncontext unavailable\r\n");
            return;
        }

        WriteTextFormat(file, "[Stack]\r\nESP=0x%08X\r\n", (unsigned int)context->Esp);
        const DWORD* stack = reinterpret_cast<const DWORD*>(context->Esp);
        if (!stack || SafeIsBadReadPtr(stack, sizeof(DWORD) * 32))
        {
            WriteRawText(file, "stack unreadable\r\n");
            return;
        }

        for (size_t i = 0; i < 32; ++i)
        {
            WriteTextFormat(file, "+%02X 0x%08X : 0x%08X\r\n",
                (unsigned int)(i * sizeof(DWORD)),
                (unsigned int)(uintptr_t)(stack + i),
                (unsigned int)stack[i]);
        }
#else
        UNREFERENCED_PARAMETER(file);
        UNREFERENCED_PARAMETER(context);
#endif
    }

    void WriteModuleSnapshot(HANDLE file)
    {
        HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ::GetCurrentProcessId());
        if (snapshot == INVALID_HANDLE_VALUE)
        {
            WriteRawText(file, "[Modules]\r\nsnapshot failed\r\n");
            return;
        }

        MODULEENTRY32W entry = {};
        entry.dwSize = sizeof(entry);
        if (!::Module32FirstW(snapshot, &entry))
        {
            ::CloseHandle(snapshot);
            WriteRawText(file, "[Modules]\r\nmodule enumeration failed\r\n");
            return;
        }

        WriteRawText(file, "[Modules]\r\n");
        do
        {
            char moduleNameUtf8[MAX_PATH * 3] = {};
            char modulePathUtf8[MAX_PATH * 3] = {};
            WideToUtf8Buffer(entry.szModule, moduleNameUtf8, (int)sizeof(moduleNameUtf8));
            WideToUtf8Buffer(entry.szExePath, modulePathUtf8, (int)sizeof(modulePathUtf8));
            WriteTextFormat(file,
                "%s base=0x%08X size=0x%08X path=%s\r\n",
                moduleNameUtf8[0] ? moduleNameUtf8 : "(unknown)",
                (unsigned int)(uintptr_t)entry.modBaseAddr,
                (unsigned int)entry.modBaseSize,
                modulePathUtf8[0] ? modulePathUtf8 : "(unavailable)");
        } while (::Module32NextW(snapshot, &entry));

        ::CloseHandle(snapshot);
    }

    bool BuildCrashArtifactPaths(
        DWORD exceptionCode,
        DWORD threadId,
        wchar_t* outDumpPath,
        size_t outDumpPathCount,
        wchar_t* outTextPath,
        size_t outTextPathCount)
    {
        if ((!outDumpPath || outDumpPathCount == 0) &&
            (!outTextPath || outTextPathCount == 0))
        {
            return false;
        }

        if (!g_crashOutputDirectory[0] && !ResolveCrashOutputDirectory(g_crashOutputDirectory, MAX_PATH))
            return false;

        SYSTEMTIME localTime = {};
        ::GetLocalTime(&localTime);

        wchar_t baseName[160] = {};
        swprintf_s(baseName,
            L"SuperSkillCrash_%04u%02u%02u_%02u%02u%02u_%03u_pid%lu_tid%lu_code%08X",
            (unsigned int)localTime.wYear,
            (unsigned int)localTime.wMonth,
            (unsigned int)localTime.wDay,
            (unsigned int)localTime.wHour,
            (unsigned int)localTime.wMinute,
            (unsigned int)localTime.wSecond,
            (unsigned int)localTime.wMilliseconds,
            (unsigned long)::GetCurrentProcessId(),
            (unsigned long)threadId,
            (unsigned int)exceptionCode);

        if (outDumpPath && outDumpPathCount > 0)
            swprintf_s(outDumpPath, outDumpPathCount, L"%s\\%s.dmp", g_crashOutputDirectory, baseName);
        if (outTextPath && outTextPathCount > 0)
            swprintf_s(outTextPath, outTextPathCount, L"%s\\%s.txt", g_crashOutputDirectory, baseName);
        return true;
    }

    bool TryWriteMiniDumpFile(const wchar_t* dumpPath, EXCEPTION_POINTERS* exceptionPointers, DumpWriteDiagnostics* diagnostics)
    {
        if (diagnostics)
            *diagnostics = DumpWriteDiagnostics{};

        if (!dumpPath || !dumpPath[0] || !exceptionPointers)
            return false;

        if (diagnostics)
            diagnostics->attempted = true;

        if (!g_miniDumpWriteDump && !ResolveDbgHelpModule())
        {
            if (diagnostics)
                diagnostics->resolveDbgHelpError = ::GetLastError();
            return false;
        }

        HANDLE dumpFile = ::CreateFileW(
            dumpPath,
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (dumpFile == INVALID_HANDLE_VALUE)
        {
            if (diagnostics)
                diagnostics->createFileError = ::GetLastError();
            return false;
        }

        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = {};
        exceptionInfo.ThreadId = ::GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPointers;
        exceptionInfo.ClientPointers = FALSE;

        const MINIDUMP_TYPE primaryType = static_cast<MINIDUMP_TYPE>(
            MiniDumpNormal |
            MiniDumpWithDataSegs |
            MiniDumpWithHandleData |
            MiniDumpWithIndirectlyReferencedMemory |
            MiniDumpScanMemory |
            MiniDumpWithThreadInfo |
            MiniDumpWithUnloadedModules);

        BOOL ok = g_miniDumpWriteDump(
            ::GetCurrentProcess(),
            ::GetCurrentProcessId(),
            dumpFile,
            primaryType,
            &exceptionInfo,
            nullptr,
            nullptr);

        if (!ok)
        {
            if (diagnostics)
                diagnostics->primaryWriteError = ::GetLastError();

            ::SetFilePointer(dumpFile, 0, nullptr, FILE_BEGIN);
            ::SetEndOfFile(dumpFile);

            ok = g_miniDumpWriteDump(
                ::GetCurrentProcess(),
                ::GetCurrentProcessId(),
                dumpFile,
                MiniDumpNormal,
                &exceptionInfo,
                nullptr,
                nullptr);
            if (!ok && diagnostics)
                diagnostics->fallbackWriteError = ::GetLastError();
        }

        ::FlushFileBuffers(dumpFile);
        LARGE_INTEGER fileSize = {};
        unsigned long long finalFileSize = 0;
        if (::GetFileSizeEx(dumpFile, &fileSize) && fileSize.QuadPart > 0)
            finalFileSize = static_cast<unsigned long long>(fileSize.QuadPart);
        if (diagnostics)
            diagnostics->fileSize = finalFileSize;
        ::CloseHandle(dumpFile);

        const bool wroteDump = ok == TRUE && finalFileSize > 0;
        if (!wroteDump)
            ::DeleteFileW(dumpPath);

        if (diagnostics)
            diagnostics->wroteDump = wroteDump;
        return wroteDump;
    }

    void WriteCrashTextFile(
        const wchar_t* textPath,
        const wchar_t* dumpPath,
        const DumpWriteDiagnostics& diagnostics,
        const char* captureSource,
        EXCEPTION_POINTERS* exceptionPointers)
    {
        if (!textPath || !textPath[0])
            return;

        HANDLE file = ::CreateFileW(
            textPath,
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return;

        WriteUtf8Bom(file);

        SYSTEMTIME localTime = {};
        ::GetLocalTime(&localTime);

        char dumpPathUtf8[MAX_PATH * 3] = {};
        char outputDirUtf8[MAX_PATH * 3] = {};
        char dbgHelpPathUtf8[MAX_PATH * 3] = {};
        WideToUtf8Buffer(dumpPath, dumpPathUtf8, (int)sizeof(dumpPathUtf8));
        WideToUtf8Buffer(g_crashOutputDirectory, outputDirUtf8, (int)sizeof(outputDirUtf8));
        WideToUtf8Buffer(g_dbgHelpModulePath, dbgHelpPathUtf8, (int)sizeof(dbgHelpPathUtf8));

        WriteRawText(file, "SuperSkillWnd Crash Capture\r\n");
        WriteTextFormat(file,
            "Time=%04u-%02u-%02u %02u:%02u:%02u\r\n",
            (unsigned int)localTime.wYear,
            (unsigned int)localTime.wMonth,
            (unsigned int)localTime.wDay,
            (unsigned int)localTime.wHour,
            (unsigned int)localTime.wMinute,
            (unsigned int)localTime.wSecond);
        WriteTextFormat(file, "ProcessId=%lu ThreadId=%lu\r\n",
            (unsigned long)::GetCurrentProcessId(),
            (unsigned long)::GetCurrentThreadId());
        WriteTextFormat(file, "CaptureSource=%s\r\n",
            (captureSource && captureSource[0]) ? captureSource : "(unknown)");
        WriteTextFormat(file, "CaptureEnabled=%d Installed=%d OutputDir=%s\r\n",
            InterlockedCompareExchange(&g_crashCaptureEnabled, 0, 0) != 0 ? 1 : 0,
            InterlockedCompareExchange(&g_crashCaptureInfrastructureInstalled, 0, 0) != 0 ? 1 : 0,
            outputDirUtf8[0] ? outputDirUtf8 : "(unavailable)");
        WriteTextFormat(file, "DumpAttempted=%d DumpWritten=%d DumpSize=%llu DumpPath=%s\r\n",
            diagnostics.attempted ? 1 : 0,
            diagnostics.wroteDump ? 1 : 0,
            diagnostics.fileSize,
            dumpPathUtf8[0] ? dumpPathUtf8 : "(unavailable)");
        WriteTextFormat(file,
            "DbgHelp=%s ResolveErr=0x%08X CreateErr=0x%08X PrimaryErr=0x%08X FallbackErr=0x%08X\r\n",
            dbgHelpPathUtf8[0] ? dbgHelpPathUtf8 : "(unavailable)",
            (unsigned int)diagnostics.resolveDbgHelpError,
            (unsigned int)diagnostics.createFileError,
            (unsigned int)diagnostics.primaryWriteError,
            (unsigned int)diagnostics.fallbackWriteError);
        WriteTextFormat(file, "RuntimeLog=%s\r\n",
            GetRuntimeLogPathA()[0] ? GetRuntimeLogPathA() : "(disabled)");

        if (!exceptionPointers || !exceptionPointers->ExceptionRecord || !exceptionPointers->ContextRecord)
        {
            WriteRawText(file, "ExceptionPointers unavailable\r\n");
            ::CloseHandle(file);
            return;
        }

        const EXCEPTION_RECORD* record = exceptionPointers->ExceptionRecord;
        const CONTEXT* context = exceptionPointers->ContextRecord;

        WriteRawText(file, "\r\n[Exception]\r\n");
        WriteTextFormat(file,
            "Code=0x%08X Flags=0x%08X Address=0x%08X Parameters=%u\r\n",
            (unsigned int)record->ExceptionCode,
            (unsigned int)record->ExceptionFlags,
            (unsigned int)(uintptr_t)record->ExceptionAddress,
            (unsigned int)record->NumberParameters);

        for (DWORD i = 0; i < record->NumberParameters && i < EXCEPTION_MAXIMUM_PARAMETERS; ++i)
        {
            WriteTextFormat(file,
                "Param[%u]=0x%08X\r\n",
                (unsigned int)i,
                (unsigned int)record->ExceptionInformation[i]);
        }

        if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
            record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
        {
            const unsigned int accessType =
                record->NumberParameters > 0 ? (unsigned int)record->ExceptionInformation[0] : 0xFFFFFFFFU;
            const unsigned int accessAddress =
                record->NumberParameters > 1 ? (unsigned int)record->ExceptionInformation[1] : 0;
            WriteTextFormat(file,
                "AccessViolation type=%u target=0x%08X\r\n",
                accessType,
                accessAddress);
        }

        WriteAddressModuleInfo(file, "ExceptionAddress", record->ExceptionAddress);
        WriteAddressRegionInfo(file, "ExceptionAddress", record->ExceptionAddress);

#if defined(_M_IX86)
        WriteRawText(file, "\r\n[Registers]\r\n");
        WriteTextFormat(file, "EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X\r\n",
            (unsigned int)context->Eax,
            (unsigned int)context->Ebx,
            (unsigned int)context->Ecx,
            (unsigned int)context->Edx);
        WriteTextFormat(file, "ESI=0x%08X EDI=0x%08X EBP=0x%08X ESP=0x%08X\r\n",
            (unsigned int)context->Esi,
            (unsigned int)context->Edi,
            (unsigned int)context->Ebp,
            (unsigned int)context->Esp);
        WriteTextFormat(file, "EIP=0x%08X EFLAGS=0x%08X\r\n",
            (unsigned int)context->Eip,
            (unsigned int)context->EFlags);

        WriteRawText(file, "\r\n[DisasmBytes]\r\n");
        WriteHexBytes(file, "EIP", reinterpret_cast<const void*>(context->Eip), 16);
        WriteAddressRegionInfo(file, "ESP", reinterpret_cast<const void*>(context->Esp));
        WriteAddressModuleInfo(file, "ESP", reinterpret_cast<const void*>(context->Esp));
        WriteRawText(file, "\r\n");
        WriteStackDwords(file, context);
#endif

        WriteRawText(file, "\r\n");
        WriteModuleSnapshot(file);
        ::CloseHandle(file);
    }

    void WriteCrashArtifacts(EXCEPTION_POINTERS* exceptionPointers, const char* captureSource, bool attemptDump)
    {
        if (!exceptionPointers || !exceptionPointers->ExceptionRecord || !exceptionPointers->ContextRecord)
            return;

        if (InterlockedCompareExchange(&g_crashCaptureWriting, 1, 0) != 0)
            return;

        __try
        {
            wchar_t dumpPath[MAX_PATH] = {};
            wchar_t textPath[MAX_PATH] = {};
            const bool pathReady = BuildCrashArtifactPaths(
                    exceptionPointers->ExceptionRecord->ExceptionCode,
                    ::GetCurrentThreadId(),
                    attemptDump ? dumpPath : nullptr,
                    attemptDump ? MAX_PATH : 0,
                    textPath,
                    MAX_PATH);
            if (!pathReady)
            {
                WriteLog("[CrashCapture] failed to resolve crash artifact paths");
            }
            else
            {
                DumpWriteDiagnostics diagnostics = {};
                bool dumpWritten = false;
                if (attemptDump)
                    dumpWritten = TryWriteMiniDumpFile(dumpPath, exceptionPointers, &diagnostics);
                WriteCrashTextFile(textPath, dumpPath, diagnostics, captureSource, exceptionPointers);

                char textPathUtf8[MAX_PATH * 3] = {};
                WideToUtf8Buffer(textPath, textPathUtf8, (int)sizeof(textPathUtf8));
                WriteLogFmt("[CrashCapture] wrote dump=%d attempted=%d size=%llu source=%s text=%s",
                    dumpWritten ? 1 : 0,
                    diagnostics.attempted ? 1 : 0,
                    diagnostics.fileSize,
                    (captureSource && captureSource[0]) ? captureSource : "unknown",
                    textPathUtf8[0] ? textPathUtf8 : "(unavailable)");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            WriteLog("[CrashCapture] exception while writing crash artifacts");
        }

        InterlockedExchange(&g_crashCaptureWriting, 0);
    }

    void CaptureSyntheticCrash(
        DWORD exceptionCode,
        const void* exceptionAddress,
        ULONG parameterCount,
        const ULONG_PTR* parameters,
        const char* captureSource)
    {
        if (InterlockedCompareExchange(&g_crashCaptureEnabled, 0, 0) == 0)
            return;

        CONTEXT context = {};
        ::RtlCaptureContext(&context);

        EXCEPTION_RECORD record = {};
        record.ExceptionCode = exceptionCode;
        record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        record.ExceptionAddress = const_cast<void*>(
            exceptionAddress ? exceptionAddress : reinterpret_cast<const void*>(_ReturnAddress()));

        const ULONG safeCount =
            (parameterCount <= EXCEPTION_MAXIMUM_PARAMETERS) ? parameterCount : EXCEPTION_MAXIMUM_PARAMETERS;
        record.NumberParameters = safeCount;
        for (ULONG i = 0; i < safeCount; ++i)
            record.ExceptionInformation[i] = parameters ? parameters[i] : 0;

        EXCEPTION_POINTERS pointers = {};
        pointers.ExceptionRecord = &record;
        pointers.ContextRecord = &context;

        WriteCrashArtifacts(&pointers, captureSource, true);
    }

    LONG WINAPI CrashCaptureUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
    {
        if (InterlockedCompareExchange(&g_crashCaptureEnabled, 0, 0) != 0)
            WriteCrashArtifacts(exceptionPointers, "unhandled", true);

        if (g_previousUnhandledExceptionFilter &&
            g_previousUnhandledExceptionFilter != &CrashCaptureUnhandledExceptionFilter)
        {
            __try
            {
                return g_previousUnhandledExceptionFilter(exceptionPointers);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    LONG CALLBACK CrashCaptureVectoredHandler(EXCEPTION_POINTERS* exceptionPointers)
    {
        if (InterlockedCompareExchange(&g_crashCaptureEnabled, 0, 0) == 0 ||
            !exceptionPointers ||
            !exceptionPointers->ExceptionRecord)
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        const DWORD exceptionCode = exceptionPointers->ExceptionRecord->ExceptionCode;
        if (exceptionCode == EXCEPTION_BREAKPOINT ||
            exceptionCode == EXCEPTION_SINGLE_STEP ||
            exceptionCode == kMsVcThreadNameException ||
            !ShouldLogVectoredCandidate(exceptionCode))
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }

        LONG remainingBudget = InterlockedDecrement(&g_crashCaptureCandidateLogBudget);
        if (remainingBudget >= 0)
        {
            WriteLogFmt("[CrashCapture] fatal candidate code=0x%08X addr=0x%08X tid=%lu",
                (unsigned int)exceptionCode,
                (unsigned int)(uintptr_t)exceptionPointers->ExceptionRecord->ExceptionAddress,
                (unsigned long)::GetCurrentThreadId());
        }

        if (ShouldCaptureImmediatelyInVectoredHandler(exceptionCode))
        {
            WriteCrashArtifacts(exceptionPointers, "vectored", true);
        }
        else
        {
            LONG remainingArtifactBudget = InterlockedDecrement(&g_crashCaptureCandidateArtifactBudget);
            if (remainingArtifactBudget >= 0)
                WriteCrashArtifacts(exceptionPointers, "vectored-firstchance", false);
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void EnsureCrashCaptureInfrastructureInstalled()
    {
        InstallCrashCaptureCoreInfrastructure();
        InstallCrashCaptureCrtHandlers();
        ResolveCrashOutputDirectory(g_crashOutputDirectory, MAX_PATH);
        ResolveDbgHelpModule();

        char outputDirUtf8[MAX_PATH * 3] = {};
        WideToUtf8Buffer(g_crashOutputDirectory, outputDirUtf8, (int)sizeof(outputDirUtf8));
        WriteLogFmt("[CrashCapture] infrastructure installed vectored=%d outputDir=%s",
            g_crashCaptureVectoredHandle ? 1 : 0,
            outputDirUtf8[0] ? outputDirUtf8 : "(unavailable)");
    }

    void RefreshCrashCaptureFromFeatureSwitch()
    {
        const bool enabled = IsFeatureEnabled(FeatureSwitchId::DiagnosticCrashCapture);
        InterlockedExchange(&g_crashCaptureEnabled, enabled ? 1 : 0);

        if (enabled)
            EnsureCrashCaptureInfrastructureInstalled();

        WriteLogFmt("[CrashCapture] feature=%d installed=%d vectored=%d",
            enabled ? 1 : 0,
            InterlockedCompareExchange(&g_crashCaptureInfrastructureInstalled, 0, 0) != 0 ? 1 : 0,
            g_crashCaptureVectoredHandle ? 1 : 0);
    }
} // namespace

void BootstrapCrashCaptureRuntime()
{
    InterlockedExchange(&g_crashCaptureEnabled, 1);
    InstallCrashCaptureCoreInfrastructure();
    InstallCrashCaptureCrtHandlers();
}

void InitializeCrashCaptureRuntime()
{
    if (InterlockedCompareExchange(&g_crashCaptureBootstrapInitialized, 1, 0) == 0)
        SetFeatureSwitchReloadCallback(&RefreshCrashCaptureFromFeatureSwitch);

    InstallCrashCaptureCrtHandlers();
    RefreshCrashCaptureFromFeatureSwitch();
}

void ShutdownCrashCaptureRuntime()
{
    InterlockedExchange(&g_crashCaptureEnabled, 0);

    if (g_crashCaptureVectoredHandle)
    {
        ::RemoveVectoredExceptionHandler(g_crashCaptureVectoredHandle);
        g_crashCaptureVectoredHandle = nullptr;
    }

    ::SetUnhandledExceptionFilter(g_previousUnhandledExceptionFilter);
    g_previousUnhandledExceptionFilter = nullptr;

    if (InterlockedCompareExchange(&g_savedErrorModeValid, 0, 0) != 0)
        ::SetErrorMode(g_savedErrorMode);

    if (InterlockedExchange(&g_crashCaptureCrtHandlersInstalled, 0) != 0)
    {
        _set_invalid_parameter_handler(g_previousInvalidParameterHandler);
        g_previousInvalidParameterHandler = nullptr;

        _set_purecall_handler(g_previousPureCallHandler);
        g_previousPureCallHandler = nullptr;

        std::set_terminate(g_previousTerminateHandler);
        g_previousTerminateHandler = nullptr;

        signal(SIGABRT, g_previousSigAbortHandler);
        g_previousSigAbortHandler = SIG_DFL;
    }

    if (InterlockedCompareExchange(&g_savedAbortBehaviorValid, 0, 0) != 0)
        _set_abort_behavior(g_savedAbortBehavior, kAbortBehaviorMask);

    if (g_dbgHelpModule)
    {
        ::FreeLibrary(g_dbgHelpModule);
        g_dbgHelpModule = nullptr;
    }
    g_miniDumpWriteDump = nullptr;
    g_dbgHelpModulePath[0] = L'\0';
    InterlockedExchange(&g_crashCaptureInfrastructureInstalled, 0);
}

} // namespace runtime
} // namespace ssw
