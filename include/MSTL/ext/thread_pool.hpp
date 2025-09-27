#ifndef MSTL_THREAD_POOL_H__
#define MSTL_THREAD_POOL_H__
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <future>
#include "MSTL/core/queue.hpp"
#include "MSTL/core/functional.hpp"
#include "MSTL/core/unordered_map.hpp"
MSTL_BEGIN_NAMESPACE__

static constexpr size_t THREAD_POOL_TASK_MAX_THRESHHOLD = INT32_MAX_SIZE;
static const size_t THREAD_POOL_THREAD_MAX_THRESHHOLD = std::thread::hardware_concurrency();
static constexpr int64_t THREAD_POOL_MAX_IDLE_SECONDS = 60;

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
    static void reset_id() noexcept;

    friend class _MSTL manual_thread;
    friend class _MSTL thread_pool;
};
MSTL_END_INNER__


#ifdef MSTL_PLATFORM_LINUX__
class MSTL_API pthread {
private:
	enum class STATE {
		Created,
		Detached,
		Joined,
		Cancelled
	};

	STATE state = STATE::Created;

	pthread_t thread = 0;
	int created = 1;

public:
	template <typename F, typename... Args>
	explicit pthread(F&& func, Args&&... args) noexcept {
		created = ::pthread_create(&thread, nullptr, func, this);
	}
	template <typename F, typename... Args>
	explicit pthread(const pthread_attr_t* attr, F&& func, Args&&... args) noexcept {
		created = ::pthread_create(&thread, attr, func, this);
	}

	~pthread() {
		if (is_created() && state == STATE::Created) {
			::pthread_detach(thread);
		}
	}

	MSTL_NODISCARD bool is_created() const noexcept {
		return created == 0;
	}
	MSTL_NODISCARD bool is_joinable() const noexcept {
		return is_created() && state != STATE::Detached;
	}

	MSTL_NODISCARD bool set_name(const char* name) const noexcept {
		return ::pthread_setname_np(thread, name) == 0;
	}

	MSTL_NODISCARD size_t get_id() const noexcept {
		return thread;
	}
	MSTL_NODISCARD STATE get_state() const noexcept {
		return state;
	}

	static pthread_t current_thread_id() noexcept {
		return ::pthread_self();
	}


	MSTL_NODISCARD bool join() {
		if (state != STATE::Created) return false;
		const bool res = ::pthread_join(thread, nullptr) == 0;
		if (res) state = STATE::Joined;
		return res;
	}

	MSTL_NODISCARD bool detach() noexcept {
		if (state != STATE::Created) return false;
		const bool res = (pthread_detach(thread) == 0);
		if (res) state = STATE::Detached;
		return res;
	}

	MSTL_NODISCARD bool cancel() {
		if (state != STATE::Created) return false;
		const bool res = (pthread_cancel(thread) == 0);
		if (res) state = STATE::Cancelled;
		return res;
	}
};
#endif


class MSTL_API manual_thread {
public:
    using id_type = uint32_t;

private:
	using thread_func = _MSTL function<void(id_type)>;

	thread_func func_;
	id_type thread_id_;

public:
	explicit manual_thread(thread_func func) noexcept;
	~manual_thread() = default;

    MSTL_NODISCARD id_type get_id() const noexcept;
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

    friend MSTL_API thread_pool& get_instance_thread_pool();

private:
    void thread_function(id_type thread_id);

    thread_pool();
    ~thread_pool();

public:
    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator =(const thread_pool&) = delete;
    thread_pool& operator =(thread_pool&&) = delete;

    bool set_mode(THREAD_POOL_MODE mode) noexcept;
    bool set_task_threshhold(size_t threshhold) noexcept;
    bool set_thread_threshhold(size_t threshhold) noexcept;

    MSTL_NODISCARD static size_t max_thread_size() noexcept;
    MSTL_NODISCARD bool running() const noexcept;
    MSTL_NODISCARD THREAD_POOL_MODE mode() const noexcept;

    bool start(size_t init_thread_size = 3);
    void stop();

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_task(Func&& func, Args&&... args);
};

MSTL_API thread_pool& get_instance_thread_pool();


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
		id_type thread_id = ptr->get_id();
		threads_map_.emplace(thread_id, _MSTL move(ptr));
		threads_map_[thread_id]->start();
		++idle_thread_size_;
	}
	return res;
}

MSTL_END_NAMESPACE__
#endif // MSTL_THREAD_POOL_H__
