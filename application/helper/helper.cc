/*
 * Copyright 2025 wtcat 
 */

#include <ctype.h>
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>

#include "base/file_path.h"
#include "application/helper/helper.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")


static std::wstring GetShortcutTarget(const std::wstring& shortcutPath) {
    IShellLink* psl;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); // Initialize COM
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
        IID_IShellLink, (LPVOID*)&psl);

    if (SUCCEEDED(hr)) {
        IPersistFile* ppf;

        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Load(shortcutPath.c_str(), STGM_READ);
            if (SUCCEEDED(hr)) {
                // Resolve the link if it's pointing to a moved or renamed target
                // SLR_NO_UI prevents a UI from appearing during resolution
                hr = psl->Resolve(NULL, SLR_NO_UI);
                if (SUCCEEDED(hr)) {
                    WCHAR szGotPath[MAX_PATH];
                    hr = psl->GetPath(szGotPath, MAX_PATH, NULL, SLGP_UNCPRIORITY);
                    if (SUCCEEDED(hr)) {
                        ppf->Release();
                        psl->Release();
                        CoUninitialize(); // Uninitialize COM
                        return std::wstring(szGotPath);
                    }
                }
            }
            ppf->Release();
        }
        psl->Release();
    }

    CoUninitialize(); // Uninitialize COM
    return L""; // Return empty string on failure
}

namespace app {

size_t StringToLower(const char* instr, char* outstr, size_t maxsize) {
    if (instr == nullptr || outstr == nullptr)
        return 0;

    const char* s = instr;
    while (*s != '\0' && maxsize > 1) {
        *outstr++ = tolower(*s);
        s++;
        maxsize--;
    }

    return (size_t)(s - instr);
}

size_t StringToUpper(const char* instr, char* outstr, size_t maxsize) {
    if (instr == nullptr || outstr == nullptr)
        return 0;

    const char* s = instr;
    while (*s != '\0' && maxsize > 1) {
        *outstr++ = toupper(*s);
        s++;
        maxsize--;
    }

    return (size_t)(s - instr);
}

size_t StringCopy(char* dst, const char* src, size_t dsize) {
    const char* osrc = src;
    size_t nleft = dsize;

    if (nleft != 0) {
        while (--nleft != 0) {
            if ((*dst++ = *src++) == '\0')
                break;
        }
    }
    if (nleft == 0) {
        if (dsize != 0)
            *dst = '\0';
        while (*src++ != '\0');
    }
    return src - osrc - 1;
}

bool IsSymbolFile(const std::filesystem::path &path) {
#ifndef _WIN32
    return std::filesystem::is_symlink(path);
#else
    if (path.filename().extension() == ".lnk")
        return true;
    return false;
#endif /* _WIN32 */
}

const std::filesystem::path ReadSymbolPath(const std::filesystem::path& path) {
#ifndef _WIN32
    return std::filesystem::read_symlink(path);
#else
    FilePath target(GetShortcutTarget(FilePath::FromUTF8Unsafe(path.string()).LossyDisplayName()));
    return target.AsUTF8Unsafe();
#endif /* _WIN32 */
}





} //namespace app
