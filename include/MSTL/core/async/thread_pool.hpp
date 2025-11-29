#ifndef MSTL_CORE_ASYNC_THREAD_POOL_HPP__
#define MSTL_CORE_ASYNC_THREAD_POOL_HPP__
#include "../container/priority_queue.hpp"
#include "../container/unordered_map.hpp"
#include "condition_variable.hpp"
#include "packaged_task.hpp"
#include "timer.hpp"
#include "../config/undef_cmacro.hpp"
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
	struct periodic_task_state {
		atomic_bool cancelled{false};
	};

	struct MSTL_API pool_statistics : istringify<pool_statistics> {
		size_t total_threads;
		size_t idle_threads;
		size_t busy_threads;
		size_t queue_size;
		size_t total_submitted;
		size_t total_completed;

		string to_string() const;
	};

    using id_type = manual_thread::id_type;
	using periodic_token = shared_ptr<periodic_task_state>;

private:
	using Task = _MSTL function<void()>;

	struct priority_task {
		Task task;
		unsigned int priority;

		priority_task(Task t, unsigned int p)
		: task(_MSTL move(t)), priority(p) {}

		bool operator <(const priority_task& other) const {
			return priority < other.priority;
		}
	};

	_MSTL unordered_map<id_type, _MSTL unique_ptr<manual_thread>> threads_map_;
	_MSTL timer_scheduler<_MSTL_CHRONO steady_clock> timer_;

	id_type init_thread_size_;
	size_t thread_threshhold_;

	_MSTL priority_queue<priority_task> task_queue_;
	_MSTL atomic_uint task_size_;
	_MSTL atomic_uint idle_thread_size_;
	size_t task_threshhold_;

	_MSTL mutex task_queue_mtx_;
	_MSTL condition_variable not_full_;
	_MSTL condition_variable not_empty_;
	_MSTL condition_variable exit_cond_;

	_MSTL atomic<THREAD_POOL_MODE> pool_mode_;
	_MSTL atomic_bool is_running_;

	_MSTL atomic_size_t total_submitted_tasks_;
	_MSTL atomic_size_t total_completed_tasks_;

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
    MSTL_NODISCARD THREAD_POOL_MODE mode() const noexcept { return pool_mode_; }
	MSTL_NODISCARD pool_statistics statistics() const;

    bool start(size_t init_thread_size = 3);
    void stop();

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_task(unsigned int priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_task(Func&& func, Args&&... args) {
		return this->submit_task(0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
	}

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_after(int64_t delay_ms, unsigned int priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_after(int64_t delay_ms, Func&& func, Args&&... args) {
		return this->submit_after(delay_ms, 0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
	}

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	periodic_token submit_every(int64_t interval_ms, unsigned int priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	periodic_token submit_every(int64_t interval_ms, Func&& func, Args&&... args) {
		return this->submit_every(interval_ms, 0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
	}

	static void cancel_periodic_task(const periodic_token& token) {
		if (token) token->cancelled.store(true);
	}
};


template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
decltype(auto) thread_pool::submit_task(unsigned int priority, Func&& func, Args&&... args) {
	using Result = decltype(func(_MSTL forward<Args>(args)...));
	auto task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
		[func = _MSTL forward<Func>(func), args = _MSTL make_tuple(_MSTL forward<Args>(args)...)]() mutable {
			return _MSTL apply(func, args);
		}
	);
	_MSTL future<Result> res = task->get_future();

	_MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);
	if (!not_full_.wait_for(lock, _MSTL_CHRONO seconds(1), [&]()->bool {
		return task_queue_.size() < task_threshhold_;
	})) {
		auto task_ = _MSTL make_shared<_MSTL packaged_task<Result()>>([]() -> Result { return Result(); });
		(*task_)();
		return task_->get_future(); // 无意义future
	}

	task_queue_.emplace(priority_task([task] { (*task)(); }, priority));
	++task_size_;
	++total_submitted_tasks_;
	not_empty_.notify_all();

	if (pool_mode_.load() == THREAD_POOL_MODE::MODE_CACHED
		&& task_size_.load() > idle_thread_size_
		&& threads_map_.size() < thread_threshhold_) {
		auto ptr = _MSTL make_unique<manual_thread>([this](const id_type id) { thread_function(id); });
		id_type thread_id = ptr->id();
		threads_map_.emplace(thread_id, _MSTL move(ptr));
		threads_map_[thread_id]->start();
		++idle_thread_size_;
	}
	return res;
}

template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
decltype(auto) thread_pool::submit_after(const int64_t delay_ms, unsigned int priority, Func&& func, Args&&... args) {
	using ResultType = decltype(func(args...));
	auto task = _MSTL make_shared<_MSTL packaged_task<ResultType()>>(
		[func = _MSTL forward<Func>(func), tup = _MSTL make_tuple(_MSTL forward<Args>(args)...)]() mutable {
			return _MSTL apply(func, tup);
		});
	_MSTL future<ResultType> res = task->get_future();

	auto expire_time = _MSTL_CHRONO steady_clock::now() + _MSTL_CHRONO milliseconds(delay_ms);
	timer_.add_task(expire_time, [this, task = _MSTL move(task), priority]() mutable {
		this->submit_task(priority, [task]() {
			(*task)();
	    });
	});
	return res;
}

template<typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
thread_pool::periodic_token thread_pool::submit_every(int64_t interval_ms, unsigned int priority, Func &&func, Args &&...args) {
    auto state = _MSTL make_shared<periodic_task_state>();
	auto task = _MSTL make_shared<_MSTL function<void()>>(
	    [func = _MSTL forward<Func>(func), tup = _MSTL make_tuple(_MSTL forward<Args>(args)...)]() mutable {
			_MSTL apply(func, tup);
	    }
	);
    auto handler_ptr = _MSTL make_shared<Task>();
    *handler_ptr = [this, state, task, interval_ms, priority, handler_ptr]() {
        if (state->cancelled.load()) return;

    	this->submit_task(priority, [task]() {
			(*task)();
	    });

        if (state->cancelled.load()) return;
        auto next_time = _MSTL_CHRONO steady_clock::now() + _MSTL_CHRONO milliseconds(interval_ms);
        timer_.add_task(next_time, [handler_ptr]() { (*handler_ptr)(); });
    };

    auto first_time = _MSTL_CHRONO steady_clock::now() + _MSTL_CHRONO milliseconds(interval_ms);
    timer_.add_task(first_time, [handler_ptr]() { (*handler_ptr)(); });
    return state;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_THREAD_POOL_HPP__
