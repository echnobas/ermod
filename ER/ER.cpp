//MIT License
//
//Copyright(c) 2021 echnobas
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this softwareand associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright noticeand this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <algorithm>
#include "Log.h"

namespace fs = std::filesystem;
using Logger::TRACE;
using Logger::FATAL;

bool inject_dll(HANDLE proc, const char *dll_path);

int main()
{
    std::cout << "Elden Ring Launcher\n";
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA("eldenring.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        return -1;
    }
    for (const auto& entry : fs::directory_iterator("mods")) {
        const std::filesystem::path& path = entry.path();
        if (path.extension() == ".dll")
        {
            const std::string full_path = fs::absolute(path).string();
            std::cout << "[+]\tAttempting to inject " << full_path << "\n";
            if (!inject_dll(pi.hProcess, full_path.c_str()))
            {
                std::cout << "Injection failed\n";
                while (1) { std::this_thread::sleep_for(std::chrono::seconds(1)); }
            }
            std::cout << "\tInjection successful\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}

bool inject_dll(HANDLE proc, const char *dll_path)
{
    LOG(TRACE) << "Injecting " << dll_path << " into process\n";
    LPVOID path_buffer = VirtualAllocEx(proc, NULL, _MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (path_buffer == NULL)
    {
        LOG(FATAL) << "Allocating memory in target process failed, aborting.\n";
        return false;
    }
    LOG(TRACE) << "[1] Allocating process memory succeeded\n";
    if (WriteProcessMemory(proc, path_buffer, dll_path, strlen(dll_path) + 1, NULL) == 0)
    {
        LOG(FATAL) << "Writing memory in target process failed, aborting.\n";
        return false;
    }
    LOG(TRACE) << "[2] Writing process memory succeeded\n";

    HMODULE h_kernel32 = GetModuleHandle(TEXT("Kernel32"));
    if (h_kernel32 == NULL)
    {
        LOG(FATAL) << "Getting module handle (Kernel32) failed, aborting.\n";
        return false;
    }
    LPVOID p_loadlibrary = GetProcAddress(h_kernel32, "LoadLibraryA");
    if (p_loadlibrary == NULL)
    {
        LOG(FATAL) << "Getting process address (LoadLibraryA) failed, aborting.\n";
        return false;
    }
    LOG(TRACE) << "[3] Getting LoadLibraryA address succeeded\n";

    HANDLE h_thread = CreateRemoteThread(proc, 0, 0, (LPTHREAD_START_ROUTINE)p_loadlibrary, path_buffer, 0, 0);
    if (h_thread == NULL)
    {
        LOG(FATAL) << "Spawning remote thread in target process failed, aborting.\n";
        VirtualFreeEx(proc, path_buffer, 0, MEM_RELEASE);
        return false;
    }
    LOG(TRACE) << "[4] Spawned remote thread succeeded\n";
    WaitForSingleObject(h_thread, INFINITE);
    CloseHandle(h_thread);
    VirtualFreeEx(proc, path_buffer, 0, MEM_RELEASE);
    return true;
}