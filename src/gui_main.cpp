#include "gui/app_state.h"
#include "gui/shell.h"
#include "gui/theme.h"
#include "gui/worker.h"

#include "rgbpicker/runtime_backend_factory.h"
#include "rgbpicker/profiles.h"
#include "rgbpicker/settings.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <array>
#include <cstring>
#include <mutex>
#include <vector>

#include <d3d11.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <tchar.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

namespace {

using rgbpicker::gui::UiState;
using rgbpicker::gui::Worker;


ID3D11Device* g_device{nullptr};
ID3D11DeviceContext* g_context{nullptr};
IDXGISwapChain* g_swapChain{nullptr};
ID3D11RenderTargetView* g_renderTarget{nullptr};
UINT g_resizeWidth{0};
UINT g_resizeHeight{0};

void createRenderTarget()
{
    ID3D11Texture2D* backBuffer{nullptr};
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer != nullptr) {
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
        backBuffer->Release();
    }
}

void releaseRenderTarget()
{
    if (g_renderTarget != nullptr) {
        g_renderTarget->Release();
        g_renderTarget = nullptr;
    }
}

bool createDeviceD3D(HWND window)
{
    DXGI_SWAP_CHAIN_DESC swapDesc{};
    swapDesc.BufferCount = 2;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = window;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.Windowed = TRUE;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const std::array levels{D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT result{D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels.data(),
        static_cast<UINT>(levels.size()), D3D11_SDK_VERSION, &swapDesc, &g_swapChain, &g_device,
        nullptr, &g_context)};
    if (result == DXGI_ERROR_UNSUPPORTED) {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels.data(),
            static_cast<UINT>(levels.size()), D3D11_SDK_VERSION, &swapDesc, &g_swapChain, &g_device,
            nullptr, &g_context);
    }
    if (result != S_OK) {
        return false;
    }
    createRenderTarget();
    return true;
}

void cleanupDeviceD3D()
{
    releaseRenderTarget();
    if (g_swapChain != nullptr) {
        g_swapChain->Release();
        g_swapChain = nullptr;
    }
    if (g_context != nullptr) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device != nullptr) {
        g_device->Release();
        g_device = nullptr;
    }
}

void applyTitleBarTheme(HWND window)
{
    DWORD lightTheme{1};
    DWORD size{sizeof lightTheme};
    RegGetValueW(HKEY_CURRENT_USER,
                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &lightTheme, &size);
    const BOOL useDark{lightTheme == 0 ? TRUE : FALSE};
    DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof useDark);
}


constexpr UINT trayCallbackMessage{WM_APP + 1};
constexpr UINT trayIconId{1};
constexpr UINT trayShowCommand{40001};
constexpr UINT trayQuitCommand{40002};

HICON g_appIcon{nullptr};
bool g_quitting{false};

void addTrayIcon(HWND window)
{
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof icon;
    icon.hWnd = window;
    icon.uID = trayIconId;
    icon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    icon.uCallbackMessage = trayCallbackMessage;
    icon.hIcon = g_appIcon;
    wcscpy_s(icon.szTip, L"RGB Picker");
    Shell_NotifyIconW(NIM_ADD, &icon);
}

void removeTrayIcon(HWND window)
{
    NOTIFYICONDATAW icon{};
    icon.cbSize = sizeof icon;
    icon.hWnd = window;
    icon.uID = trayIconId;
    Shell_NotifyIconW(NIM_DELETE, &icon);
}

void showFromTray(HWND window)
{
    ShowWindow(window, SW_SHOW);
    ShowWindow(window, SW_RESTORE);
    SetForegroundWindow(window);
}

void showTrayMenu(HWND window)
{
    HMENU menu{CreatePopupMenu()};
    if (menu == nullptr) {
        return;
    }
    AppendMenuW(menu, MF_STRING, trayShowCommand, L"Show RGB Picker");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, trayQuitCommand, L"Quit");

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, cursor.x, cursor.y, 0, window, nullptr);
    DestroyMenu(menu);
}

std::vector<rgbpicker::DeviceColor> startupLook(rgbpicker::AppliedStore& appliedStore)
{
    std::vector<rgbpicker::DeviceColor> lit{appliedStore.load()};
    const rgbpicker::Profile* const active{rgbpicker::gui::activeProfile()};
    if (active == nullptr || rgbpicker::sameLook(lit, active->devices)) {
        return lit;
    }
    appliedStore.save(active->devices);
    return active->devices;
}

void onTrayClick(HWND window, LPARAM click)
{
    if (LOWORD(click) == WM_LBUTTONUP || LOWORD(click) == WM_LBUTTONDBLCLK) {
        showFromTray(window);
    } else if (LOWORD(click) == WM_RBUTTONUP) {
        showTrayMenu(window);
    }
}

bool onCommand(HWND window, WPARAM command)
{
    if (LOWORD(command) == trayShowCommand) {
        showFromTray(window);
        return true;
    }
    if (LOWORD(command) == trayQuitCommand) {
        g_quitting = true;
        DestroyWindow(window);
        return true;
    }
    return false;
}

void onResize(WPARAM request, LPARAM size)
{
    if (request != SIZE_MINIMIZED) {
        g_resizeWidth = LOWORD(size);
        g_resizeHeight = HIWORD(size);
    }
}

bool pumpMessages()
{
    MSG message;
    bool quitting{false};
    while (PeekMessageW(&message, nullptr, 0U, 0U, PM_REMOVE) != 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
        if (message.message == WM_QUIT) {
            quitting = true;
        }
    }
    return quitting;
}

void resizeIfRequested()
{
    if (g_resizeWidth == 0 || g_resizeHeight == 0) {
        return;
    }
    releaseRenderTarget();
    g_swapChain->ResizeBuffers(0, g_resizeWidth, g_resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
    g_resizeWidth = 0;
    g_resizeHeight = 0;
    createRenderTarget();
}

void renderFrame(UiState& state, Worker& worker)
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    rgbpicker::gui::drawUi(state, worker);

    ImGui::Render();
    if (g_renderTarget != nullptr) {
        constexpr float clear[4]{0.114f, 0.141f, 0.157f, 1.0f};
        g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
        g_context->ClearRenderTargetView(g_renderTarget, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
    g_swapChain->Present(1, 0);
}

void runFrameLoop(HWND window, UiState& state, Worker& worker)
{
    while (!pumpMessages()) {
        if (!IsWindowVisible(window)) {
            WaitMessage();
            continue;
        }
        resizeIfRequested();
        renderFrame(state, worker);
    }
}

LRESULT WINAPI windowProc(HWND window, UINT message, WPARAM wide, LPARAM low)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, wide, low) != 0) {
        return 1;
    }
    switch (message) {
    case WM_SIZE: onResize(wide, low); return 0;
    case WM_SYSCOMMAND:
        if ((wide & 0xfff0) == SC_KEYMENU) {
            return 0;
        }
        break;
    case WM_CLOSE:
        ShowWindow(window, SW_HIDE);
        return 0;
    case trayCallbackMessage: onTrayClick(window, low); return 0;
    case WM_COMMAND:
        if (onCommand(window, wide)) {
            return 0;
        }
        break;
    case WM_DESTROY:
        removeTrayIcon(window);
        PostQuitMessage(0);
        return 0;
    default: break;
    }
    return DefWindowProcW(window, message, wide, low);
}

}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, PSTR commandLine, int showCommand)
{
    ImGui_ImplWin32_EnableDpiAwareness();

    HICON appIcon{LoadIconW(instance, MAKEINTRESOURCEW(1))};
    WNDCLASSEXW windowClass{sizeof(windowClass), CS_CLASSDC, windowProc, 0,      0,
                            instance,            appIcon,    nullptr,    nullptr, nullptr,
                            L"rgb-picker",       appIcon};
    RegisterClassExW(&windowClass);
    HWND window{CreateWindowW(windowClass.lpszClassName, L"RGB Picker", WS_OVERLAPPEDWINDOW, 100,
                              100, 1180, 780, nullptr, nullptr, instance, nullptr)};
    applyTitleBarTheme(window);

    if (!createDeviceD3D(window)) {
        cleanupDeviceD3D();
        UnregisterClassW(windowClass.lpszClassName, instance);
        return 1;
    }

    g_appIcon = appIcon;
    addTrayIcon(window);

    const bool startHidden{std::strstr(commandLine, "--startminimized") != nullptr};
    if (!startHidden) {
        ShowWindow(window, showCommand);
        UpdateWindow(window);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io{ImGui::GetIO()};
    io.IniFilename = nullptr;

    const float dpiScale{ImGui_ImplWin32_GetDpiScaleForHwnd(window)};
    rgbpicker::gui::applyTheme(dpiScale);
    rgbpicker::gui::loadFonts(io, dpiScale);

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(g_device, g_context);

    const auto loginStartup{rgbpicker::makeLoginStartup("RGB Picker")};
    rgbpicker::gui::g_loginStartup = loginStartup.get();
    const auto settingsStore{rgbpicker::makeSettingsStore()};
    const auto profileStore{rgbpicker::makeProfileStore()};
    const auto appliedStore{rgbpicker::makeAppliedStore()};
    const auto layoutStore{rgbpicker::makeLayoutStore()};
    rgbpicker::gui::g_settingsStore = settingsStore.get();
    rgbpicker::gui::g_profileStore = profileStore.get();
    rgbpicker::gui::g_settings = settingsStore->load();
    rgbpicker::gui::g_profiles = profileStore->load();
    rgbpicker::gui::g_settings.runAtLogin = loginStartup->refresh();

    UiState state;
    state.lastColor = rgbpicker::gui::g_settings.lastColor;
    state.applied = startupLook(*appliedStore);
    rgbpicker::RuntimeBackendFactory factory;
    Worker worker{factory, state,
                  rgbpicker::gui::WorkerConfig{
                      .restoreColor = rgbpicker::gui::g_settings.restoreColor,
                      .brightness = rgbpicker::gui::g_settings.brightness,
                      .appliedStore = appliedStore.get(),
                      .layoutStore = layoutStore.get()}};

    runFrameLoop(window, state, worker);

    std::vector<rgbpicker::DeviceColor> look;
    {
        const std::lock_guard lock{state.mutex};
        rgbpicker::gui::g_settings.lastColor = state.lastColor;
        look = state.applied;
    }
    rgbpicker::gui::saveSettings();
    appliedStore->save(look);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    cleanupDeviceD3D();
    DestroyWindow(window);
    UnregisterClassW(windowClass.lpszClassName, instance);
    return 0;
}
