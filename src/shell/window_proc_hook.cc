#include "window_proc_hook.h"
#include "blook/blook.h"
#include "logger.h"

#include <Windows.h>
#include <unordered_set>

namespace mb_shell {
static std::unordered_set<HWND> hooked_windows;

void window_proc_hook::install(void *hwnd) {
    if (installed.load(std::memory_order_acquire))
        uninstall();
    this->hwnd = hwnd;
    this->original_proc = (void *)GetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC);

    this->hooked_proc = (void *)blook::Function::into_function_pointer(
        [this](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
            std::optional<int> callOriginal = std::nullopt;
            try {
                for (auto &f : this->hooks) {
                    if (!callOriginal)
                        callOriginal =
                            f(hwnd, this->original_proc, msg, wp, lp);
                }
            } catch (const std::exception &e) {
                spdlog::error("Window procedure hook failed: {}", e.what());
            } catch (...) {
                spdlog::error(
                    "Window procedure hook failed with an unknown error");
            }

            while (true) {
                std::function<void()> task;
                {
                    std::lock_guard lock(this->tasks_mutex);
                    if (this->tasks.empty()) {
                        break;
                    }
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }

            return callOriginal ? *callOriginal
                                : CallWindowProcW((WNDPROC)this->original_proc,
                                                  hwnd, msg, wp, lp);
        });

    SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)this->hooked_proc);
    installed.store(true, std::memory_order_release);
}

void window_proc_hook::uninstall() {
    if (hwnd && IsWindow((HWND)hwnd) &&
        (void *)GetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC) == hooked_proc) {
        SetWindowLongPtrW((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)original_proc);
    }
    installed.store(false, std::memory_order_release);
}
window_proc_hook::~window_proc_hook() {
    if (installed.load(std::memory_order_acquire)) {
        uninstall();
    }
}
void window_proc_hook::send_null() {
    if (installed.load(std::memory_order_acquire) && hwnd &&
        IsWindow((HWND)hwnd)) {
        PostMessageW((HWND)hwnd, WM_NULL, 0, 0);
    }
}
} // namespace mb_shell
