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
    └── common/FileDialog.* # دیالوگ‌های Open/Save ویندوز برای Import/Export
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

## قابلیت‌های جدید

- **گروه‌بندی سرورها:** فیلد اختیاری Group هنگام افزودن سرور؛ در جدول یک ستون Group نمایش داده می‌شود و بالای لیست یک فیلتر بر اساس گروه وجود دارد.
- **Import / Export لیست سرورها:** از منوی File یا دکمه‌های Import/Export، لیست سرورها را در یک فایل متنی (همان فرمت `servers.txt`) ذخیره یا از آن بازیابی کنید. Import کل لیست فعلی را با محتوای فایل جایگزین می‌کند.
- **احراز هویت پراکسی:** فیلدهای اختیاری Username/Password هنگام افزودن سرور. هنگام Connect این مقادیر به‌عنوان اعتبارنامهٔ پیش‌فرض WinINet برای همین پراسس تنظیم می‌شوند (`INTERNET_OPTION_PROXY_USERNAME/PASSWORD`) تا در پراکسی‌هایی که Basic/NTLM auth می‌خواهند، پاپ‌آپ لاگین برای اپ‌های WinINet-محور همین سشن رد شود. **توجه:** این مقادیر در `servers.txt` به‌صورت متن ساده (بدون رمزنگاری) ذخیره می‌شوند و این اعتبارنامه سیستم‌وایید/برای همهٔ پراسس‌ها نیست.
- **ذخیرهٔ اندازه/موقعیت پنجره:** آخرین سایز و مکان پنجره در `%APPDATA%\NetProxyManager\window.txt` ذخیره و در اجرای بعدی بازیابی می‌شود.
- **آیکون اختصاصی:** یک آیکون ساده (`resources/app.ico`) برای پنجره، نوار وظیفه و System Tray اضافه شد؛ در صورت تمایل جایگزینش کنید.
- **نوار خطای بهتر:** پیام‌های خطا اکنون در یک بنر با دکمهٔ Dismiss نمایش داده می‌شوند به‌جای یک خط متن ساده.

## راهنمای پیاده‌سازی نسخه‌های بعدی
1. **اعتبار پورت پراکسی:** علاوه بر ICMP، یک TCP connect به `host:port` با timeout کوتاه.
2. **پروفایل‌ها:** HTTP / PAC URL (`INTERNET_PER_CONN_AUTOCONFIG_URL`).
3. **رمزنگاری اعتبارنامه:** فعلاً یوزر/پسورد پراکسی در `servers.txt` متن ساده ذخیره می‌شود؛ می‌توان با DPAPI (`CryptProtectData`) رمزنگاری‌اش کرد.
4. **همگام‌سازی ابری:** لیست سرور، قواعد split-bypass، آمار دقیق‌تر per-process (نیاز به APIهای دیگر غیر از WinINet).

## نکات ایمنی و محدودهٔ MVP

- برنامه تنظیمات **همین کاربر ویندوز** را عوض می‌کند، نه سیاست دامنه و نه تونل VPN.
- شمارندهٔ ترافیک، کل NIC است نه فقط ترافیک پراکسی.
- هنگام Exit اگر هنوز Connected باشید، پراکسی قبلی سیستم restore می‌شود.

## لایسنس

این پروژه تحت یک لایسنس سفارشی «Source-Available» منتشر شده (فایل `LICENSE.md`)، **نه** یک لایسنس متن‌باز استاندارد مثل MIT. خلاصه‌اش:

- استفاده، اجرا و تغییر کد برای همه و برای هر منظوری (از جمله تجاری) رایگان و آزاده.
- می‌تونید نسخه‌ی بدون‌تغییر رو برای **استفاده‌ی شخصی خودتون** نگه دارید، ولی نمی‌تونید جای دیگه‌ای (رپوی دیگه، سایت، پکیج‌ریجیستری و ...) منتشرش کنید.
- اگه کد رو تغییر دادید، تنها راه مجاز برای در دسترس‌ عموم قرار دادن نسخه‌ی تغییریافته، ارسال Pull Request به همین مخزن اصلیه؛ میزبانی یه فورک تغییریافته جای دیگه مجاز نیست.
- قبل از انتشار روی گیت‌هاب، حتماً در `LICENSE.md` جای `[COPYRIGHT HOLDER]` اسم/حساب خودتون و جای `[REPOSITORY URL]` آدرس همین مخزن رو جایگزین کنید.

توجه: این یه محدودیت *قانونیه*، نه فنی — گیت‌هاب همچنان دکمه‌ی Fork رو نشون می‌ده و کسی می‌تونه فنی فورک کنه؛ اگه بدون اجازه منتشرش کنه، داره لایسنس رو نقض می‌کنه و شما (به‌عنوان دارنده‌ی کپی‌رایت) می‌تونید درخواست حذف (مثلاً DMCA takedown در گیت‌هاب) بدید.
