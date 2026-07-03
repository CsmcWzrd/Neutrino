#include "Neutrino/Neutrino.hpp"
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>

namespace neutrino {

Neu_Application* Neu_Application::current_ = nullptr;

Neu_Application::Neu_Application()
{
    current_ = this;
}

Neu_Application::~Neu_Application()
{
    if (display_) {
        XCloseDisplay(display_);
    }
    current_ = nullptr;
}

bool Neu_Application::detectWayland()
{
    const char* forceX11 = std::getenv("NEUTRINO_USE_X11");
    if (forceX11 && std::strcmp(forceX11, "1") == 0) {
        return false;
    }

    const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
    if (!waylandDisplay || !*waylandDisplay) {
        return false;
    }

    void* library = dlopen("libwayland-client.so.0", RTLD_LAZY);
    if (!library) {
        library = dlopen("libwayland-client.so", RTLD_LAZY);
    }
    if (!library) {
        return false;
    }

    using ConnectFn = void* (*)(const char*);
    using DisconnectFn = void (*)(void*);
    auto connect = reinterpret_cast<ConnectFn>(dlsym(library, "wl_display_connect"));
    auto disconnect = reinterpret_cast<DisconnectFn>(dlsym(library, "wl_display_disconnect"));
    bool available = false;
    if (connect && disconnect) {
        void* wl = connect(nullptr);
        if (wl) {
            available = true;
            disconnect(wl);
        }
    }
    dlclose(library);
    return available;
}

bool Neu_Application::detectXRender()
{
    if (!display_) {
        return false;
    }

    void* library = dlopen("libXrender.so.1", RTLD_LAZY);
    if (!library) {
        library = dlopen("libXrender.so", RTLD_LAZY);
    }
    if (!library) {
        return false;
    }

    using QueryFn = int (*)(Display*, int*, int*);
    auto query = reinterpret_cast<QueryFn>(dlsym(library, "XRenderQueryExtension"));
    int eventBase = 0;
    int errorBase = 0;
    const bool available = query && query(display_, &eventBase, &errorBase);
    dlclose(library);
    return available;
}

bool Neu_Application::open()
{
    waylandAvailable_ = detectWayland();
    const char* forceX11 = std::getenv("NEUTRINO_USE_X11");
    const bool useX11 = forceX11 && std::strcmp(forceX11, "1") == 0;
    if (!useX11 && waylandAvailable_) {
        // Stage2 uses the existing renderer through an XWayland bridge when present.
        // The application still chooses the Wayland session path by default and only
        // forces plain X11 when NEUTRINO_USE_X11=1 is set.
        backend_ = Neu_Backend::Wayland;
        backendName_ = "Wayland/XWayland";
    } else {
        backend_ = Neu_Backend::X11;
        backendName_ = "X11";
    }

    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        return false;
    }

    screen_ = DefaultScreen(display_);
    xrenderAvailable_ = detectXRender();

    const char* vmMode = std::getenv("NEUTRINO_VM_MODE");
    const char* fastMode = std::getenv("NEUTRINO_FAST_RENDER");
    if ((vmMode && std::strcmp(vmMode, "0") != 0) || (fastMode && std::strcmp(fastMode, "0") != 0)) {
        Neu_UseVirtualMachineFriendlyDefaults(true);
    }

    return true;
}

Neu_Application* Neu_Application::current()
{
    return current_;
}

const char* Neu_SelectedBackendName()
{
    auto* app = Neu_Application::current();
    return app ? app->backendName().c_str() : "unopened";
}

bool Neu_IsWaylandBackendSelected()
{
    auto* app = Neu_Application::current();
    return app && app->usingWayland();
}

void Neu_Application::quit()
{
    running_ = false;
    if (display_) {
        XFlush(display_);
    }
}

void Neu_Application::registerWindow(Neu_Window* window)
{
    windows_.push_back(window);
}

void Neu_Application::unregisterWindow(Neu_Window* window)
{
    windows_.erase(std::remove(windows_.begin(), windows_.end(), window), windows_.end());
}

void Neu_Application::run()
{
    running_ = true;

    while (running_ && !windows_.empty()) {
        XEvent event;
        XNextEvent(display_, &event);

        for (auto* window : windows_) {
            if (event.xany.window == window->xid()) {
                window->handleXEvent(event);
                break;
            }
        }
    }
}

} // namespace neutrino
