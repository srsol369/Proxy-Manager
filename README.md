# Net Proxy Manager

برنامهٔ دسکتاپ ویندوز برای مدیریت لیست پراکسی HTTP، تست تأخیر (ICMP ping)، و اعمال/برداشتن پراکسی سیستم از طریق **WinINet** (`InternetSetOption` / Internet Options). رابط کاربری با **Dear ImGui + DirectX 11** است و برنامه به System Tray مینیمایز می‌شود.

> پراکسی سیستم ویندوز از نوع **HTTP/HTTPS** است. SOCKS5 را نمی‌توان با همین API به‌عنوان پراکسی سراسری WinINet اعمال کرد.

## ساختار فایل‌ها

```
System & File Utility/
├── CMakeLists.txt
├── README.md
├── resources/
│   ├── app.rc              # منوی راست‌کلیک Tray
│   └── resource.h
└── src/
    ├── main.cpp            # Win32 + DX11 + حلقه ImGui + Tray
    ├── common/Types.h
    ├── app/Application.*   # وضعیت اتصال، لیست سرور، اکشن‌ها
    ├── ui/Ui.*             # رابط ImGui
    ├── proxy/WinProxy.*    # WinINet / Internet Settings
    ├── ping/Pinger.*       # ICMP
    ├── config/ConfigStore.*# ذخیره در %APPDATA%\NetProxyManager
    ├── tray/SystemTray.*
    ├── net/TrafficMonitor.*# تخمین throughput کارت شبکه
```

## پیش‌نیازها

- ویندوز 10/11
- CMake 3.20+
- Git (برای دانلود ImGui در اولین configure)
- یکی از:
  - **Visual Studio 2022** با workload «Desktop development with C++»
  - یا **MinGW-w64** (GCC) به‌همراه Windows SDK

## کامپایل با Visual Studio 2022

در PowerShell:

```powershell
cd "C:\Users\atlas\Desktop\System & File Utility"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

خروجی: `build\Release\NetProxyManager.exe`

باز کردن در IDE:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
start build\NetProxyManager.sln
```

Startup Project را روی `NetProxyManager` بگذارید.

## کامپایل با GCC (MinGW-w64)

Ninja یا MinGW Makefiles:

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw
```

باید `g++`، `cmake` و `windres` در PATH باشند. لینک DirectX (`d3d11`, `dxgi`) از Windows SDK انجام می‌شود.

## اجرای گام‌به‌گام بعد از بیلد

1. `NetProxyManager.exe` را اجرا کنید.
2. سرور HTTP (آدرس و پورت) را اضافه و Save کنید. فایل در  
   `%APPDATA%\NetProxyManager\servers.txt` ذخیره می‌شود.
3. **Ping all** تأخیر ICMP را می‌سنجد و لیست را از سریع به کند مرتب می‌کند. اگر فایروال ICMP را ببندد، Delay خالی می‌ماند حتی اگر پورت پراکسی باز باشد.
4. سرور را انتخاب کنید و **Connect** بزنید. تنظیمات Internet Options ویندوز عوض می‌شود.
5. **Disconnect** تنظیم قبلی سیستم را برمی‌گرداند.
6. بستن پنجره یا Minimize برنامه را به کنار ساعت می‌فرستد. خروج واقعی: منوی File → Exit یا راست‌کلیک Tray → Exit.

## راهنمای پیاده‌سازی نسخه‌های بعدی

1. **اعتبار پورت پراکسی:** علاوه بر ICMP، یک TCP connect به `host:port` با timeout کوتاه.
2. **پروفایل‌ها:** HTTP / PAC URL (`INTERNET_PER_CONN_AUTOCONFIG_URL`).
3. **اعتبارنامه:** WinINet پراکسی با یوزر/پسورد را جداگانه می‌خواهد؛ در MVP ذخیره نشده.
4. **آیکون اختصاصی:** یک `app.ico` بسازید و در `resources/app.rc` با `IDI_APPICON` وصل کنید.

## نکات ایمنی و محدودهٔ MVP

- برنامه تنظیمات **همین کاربر ویندوز** را عوض می‌کند، نه سیاست دامنه و نه تونل VPN.
- شمارندهٔ ترافیک، کل NIC است نه فقط ترافیک پراکسی.
- هنگام Exit اگر هنوز Connected باشید، پراکسی قبلی سیستم restore می‌شود.
