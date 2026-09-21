# Net Proxy Manager

یک اپلیکیشن دسکتاپ ویندوز برای مدیریت پراکسی‌های HTTP/HTTPS، تست تأخیر سرورها (ICMP ping) و فعال/غیرفعال‌کردن پراکسی سیستم با استفاده از WinINet. رابط کاربری با Dear ImGui و DirectX 11 پیاده‌سازی شده و برنامه به حالت Tray می‌رود.

> این برنامه پراکسی سیستم ویندوز را روی سطح HTTP/HTTPS اعمال می‌کند. برای SOCKS5 نیاز به API‌های متفاوت‌تری است.

## ویژگی‌ها

- مدیریت لیست پراکسی‌ها
- Ping و بررسی تأخیر هر سرور
- فعال/غیرفعال‌کردن پراکسی سیستم
- ذخیرهٔ لیست در %APPDATA%\NetProxyManager
- Import/Export از فایل متنی
- گروه‌بندی سرورها
- Tray Mode و ذخیرهٔ اندازه/موقعیت پنجره

## ساختار پروژه

```text
System & File Utility/
├── CMakeLists.txt
├── Makefile
├── LICENSE.md
├── README.md
├── resources/
│   ├── app.ico
│   ├── app.rc
│   └── resource.h
├── src/
│   ├── main.cpp
│   ├── app/
│   ├── common/
│   ├── config/
│   ├── net/
│   ├── ping/
│   ├── proxy/
│   ├── tray/
│   └── ui/
├── third_party/
│   └── imgui/
└── build/
```

## پیش‌نیازها

- ویندوز 10/11
- CMake 3.20+
- Visual Studio 2022 یا MinGW-w64
- Git برای دانلود ImGui

## کامپایل

```powershell
cd "C:\Users\atlas\Desktop\System & File Utility"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

یا:

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw
```

## اجرا

1. `NetProxyManager.exe` را اجرا کنید.
2. سرورهای خود را اضافه کنید.
3. روی **Connect** کلیک کنید.
4. برای قطع اتصال، **Disconnect** را بزنید.

## مجوز

برای جزئیات کامل مجوز، به [LICENSE.md](./LICENSE.md) مراجعه کنید.
