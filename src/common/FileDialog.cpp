#include "common/FileDialog.h"

#include <commdlg.h>

namespace npm {
namespace {

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, out.data(), needed, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') {
        out.pop_back();
    }
    return out;
}

}  // namespace

std::string FileDialog::OpenFile(HWND owner, const wchar_t* filter, const wchar_t* defaultExt) {
    wchar_t buffer[MAX_PATH] = {};

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = ARRAYSIZE(buffer);
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&ofn)) {
        return {};
    }
    return WideToUtf8(buffer);
}

std::string FileDialog::SaveFile(HWND owner, const wchar_t* filter, const wchar_t* defaultExt,
                                  const wchar_t* defaultFileName) {
    wchar_t buffer[MAX_PATH] = {};
    if (defaultFileName) {
        wcsncpy(buffer, defaultFileName, ARRAYSIZE(buffer) - 1);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = ARRAYSIZE(buffer);
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetSaveFileNameW(&ofn)) {
        return {};
    }
    return WideToUtf8(buffer);
}

}  // namespace npm
