#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <vector>
namespace mb_shell {
struct window_proc_hook {
    void *hwnd = nullptr;
    void *original_proc = nullptr;
    void *hooked_proc = nullptr;
    std::atomic_bool installed = false;

    std::vector<std::function<std::optional<int>(void *, void *, size_t, size_t,
                                                 size_t)>>
        hooks;
    std::queue<std::function<void()>> tasks;
    std::mutex tasks_mutex;

    void send_null();
    auto add_task(auto &&f) -> std::future<std::invoke_result_t<decltype(f)>> {
        if (!installed.load(std::memory_order_acquire))
            throw std::runtime_error("Hook not installed");
        using return_type = std::invoke_result_t<decltype(f)>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::forward<decltype(f)>(f));
        std::future<return_type> res = task->get_future();
        {
            std::lock_guard lock(tasks_mutex);
            tasks.emplace([task]() { (*task)(); });
        }
        send_null();
        return res;
    }

    void install(void *hwnd);
    void uninstall();
    ~window_proc_hook();
};
} // namespace mb_shell
