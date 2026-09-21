# MinGW-w64 / MSYS2 UCRT64 (CMake not required)
# PowerShell:  .\build-msys.ps1
# UCRT64 bash: ./build-msys.sh

ifeq ($(OS),Windows_NT)
  MKDIR = powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(dir $@))' | Out-Null"
  RMDIR = powershell -NoProfile -Command "if (Test-Path '$(BUILDDIR)') { Remove-Item -Recurse -Force '$(BUILDDIR)' }"
else
  MKDIR = mkdir -p $(dir $@)
  RMDIR = rm -rf $(BUILDDIR)
endif

CXX      ?= g++
WINDRES  ?= windres
BUILDDIR ?= build-msys
TARGET   ?= $(BUILDDIR)/NetProxyManager.exe
IMGUI    := third_party/imgui

CXXFLAGS += -std=c++20 -O2 -Wall -Wextra \
	-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX \
	-D_WIN32_WINNT=0x0A00 -DNPM_VERSION=\"0.1.0\" \
	-Isrc -Iresources -I$(IMGUI) -I$(IMGUI)/backends

LDFLAGS  += -mwindows -municode -static-libgcc -static-libstdc++
LIBS     := -ld3d11 -ldxgi -ld3dcompiler -lwininet -lwinhttp -liphlpapi \
	-lws2_32 -lshell32 -lole32 -loleaut32 -luuid -ldwmapi -lcomctl32 -lcomdlg32 \
	-luser32 -lgdi32 -ladvapi32

APP_SRCS := \
	src/main.cpp \
	src/app/Application.cpp \
	src/ui/Ui.cpp \
	src/proxy/WinProxy.cpp \
	src/ping/Pinger.cpp \
	src/config/ConfigStore.cpp \
	src/common/FileDialog.cpp \
	src/tray/SystemTray.cpp \
	src/net/TrafficMonitor.cpp \

IMGUI_SRCS := \
	$(IMGUI)/imgui.cpp \
	$(IMGUI)/imgui_draw.cpp \
	$(IMGUI)/imgui_tables.cpp \
	$(IMGUI)/imgui_widgets.cpp \
	$(IMGUI)/backends/imgui_impl_win32.cpp \
	$(IMGUI)/backends/imgui_impl_dx11.cpp

SRCS := $(APP_SRCS) $(IMGUI_SRCS)
OBJS := $(patsubst %.cpp,$(BUILDDIR)/%.o,$(SRCS))
RES  := $(BUILDDIR)/app.res

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) $(RES)
	@$(MKDIR)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: %.cpp
	@$(MKDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RES): resources/app.rc resources/resource.h resources/app.ico
	@$(MKDIR)
	$(WINDRES) --include-dir=resources --output-format=coff -i $< -o $@

clean:
	$(RMDIR)
