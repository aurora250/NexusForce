#ifndef MSTL_THREAD_POOL_HPP__
#define MSTL_THREAD_POOL_HPP__
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <future>
#include "MSTL/core/thread.hpp"
#include "MSTL/core/atomic.hpp"
#include "MSTL/core/queue.hpp"
#include "MSTL/core/functional.hpp"
#include "MSTL/core/unordered_map.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr size_t THREAD_POOL_TASK_MAX_THRESHHOLD = numeric_limits<int32_t>::max();
MSTL_INLINE17 constexpr int64_t THREAD_POOL_MAX_IDLE_SECONDS = 60;

static const size_t THREAD_POOL_THREAD_MAX_THRESHHOLD = _MSTL thread::hardware_concurrency();


enum class THREAD_POOL_MODE {
	MODE_FIXED,  // static number
	MODE_CACHED  // dynamic number
};

class manual_thread;
class thread_pool;

MSTL_BEGIN_INNER__
struct MSTL_API __thread_pool_id_generator {
private:
    static uint32_t& get_id() noexcept;
    static uint32_t get_new_id() noexcept;
    static void reset_id() noexcept { get_id() = 0; }

    friend class _MSTL manual_thread;
    friend class _MSTL thread_pool;
};
MSTL_END_INNER__


class MSTL_API manual_thread {
public:
    using id_type = uint32_t;

private:
	using thread_func = _MSTL function<void(id_type)>;

	thread_func func_;
	id_type thread_id_;

public:
	explicit manual_thread(thread_func&& func) noexcept;
	~manual_thread() = default;

    MSTL_NODISCARD id_type id() const noexcept { return thread_id_; }
    void start();
};


class MSTL_API thread_pool {
public:
    using id_type = manual_thread::id_type;

private:
	using Task = _MSTL function<void()>;

	_MSTL unordered_map<id_type, _MSTL unique_ptr<manual_thread>> threads_map_;

	id_type init_thread_size_;
	size_t thread_threshhold_;

	_MSTL queue<Task> task_queue_;
	std::atomic_uint task_size_;
	std::atomic_uint idle_thread_size_;
	size_t task_threshhold_;

	std::mutex task_queue_mtx_;
	std::condition_variable not_full_;
	std::condition_variable not_empty_;
	std::condition_variable exit_cond_;

	std::atomic<THREAD_POOL_MODE> pool_mode_;
	std::atomic_bool is_running_;

private:
    void thread_function(id_type thread_id);

    thread_pool();
    ~thread_pool() { if(is_running_) stop(); }

public:
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator =(const thread_pool&) = delete;
    thread_pool& operator =(thread_pool&&) = delete;

	static thread_pool& instance() {
		static thread_pool instance;
		return instance;
	}

    bool set_mode(THREAD_POOL_MODE mode) noexcept;
    bool set_task_threshhold(size_t threshhold) noexcept;
    bool set_thread_threshhold(size_t threshhold) noexcept;

    MSTL_NODISCARD static size_t max_thread_size() noexcept { return THREAD_POOL_THREAD_MAX_THRESHHOLD; }
    MSTL_NODISCARD bool running() const noexcept { return is_running_; }
    MSTL_NODISCARD THREAD_POOL_MODE mode() const noexcept{ return pool_mode_; }

    bool start(size_t init_thread_size = 3);
    void stop();

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_task(Func&& func, Args&&... args);
};


template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
decltype(auto) thread_pool::submit_task(Func&& func, Args&&... args) {
	using Result = decltype(func(_MSTL forward<Args>(args)...));
	auto task = _MSTL make_shared<std::packaged_task<Result()>>(
		[func = _MSTL forward<Func>(func), args = _MSTL make_tuple(_MSTL forward<Args>(args)...)]() mutable {
			return _MSTL apply(func, args);
		}
	);
	std::future<Result> res = task->get_future();

	std::unique_lock<std::mutex> lock(task_queue_mtx_);
	if (!not_full_.wait_for(lock, std::chrono::seconds(1), [&]()->bool {
		return task_queue_.size() < task_threshhold_;
	})) {
		auto task_ = _MSTL make_shared<std::packaged_task<Result()>>([]() -> Result { return Result(); });
		(*task_)();
		return task_->get_future();
	}
	task_queue_.emplace([task] { (*task)(); });
	++task_size_;
	not_empty_.notify_all();
	if (pool_mode_ == THREAD_POOL_MODE::MODE_CACHED
		&& task_size_ > idle_thread_size_
		&& threads_map_.size() < thread_threshhold_) {
		auto ptr = _MSTL make_unique<manual_thread>([this](const id_type id) { thread_function(id); });
		id_type thread_id = ptr->id();
		threads_map_.emplace(thread_id, _MSTL move(ptr));
		threads_map_[thread_id]->start();
		++idle_thread_size_;
	}
	return res;
}

MSTL_END_NAMESPACE__
#endif // MSTL_THREAD_POOL_HPP__
