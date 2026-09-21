#pragma once

#include <windows.h>

#include <string>

namespace npm {

// Thin wrapper around the classic Win32 GetOpenFileName/GetSaveFileName
// common dialogs. Returns an empty string if the user cancels.
class FileDialog {
public:
    static std::string OpenFile(HWND owner, const wchar_t* filter, const wchar_t* defaultExt);
    static std::string SaveFile(HWND owner, const wchar_t* filter, const wchar_t* defaultExt,
                                 const wchar_t* defaultFileName);
};

}  // namespace npm
