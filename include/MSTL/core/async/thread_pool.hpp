#ifndef MSTL_CORE_ASYNC_THREAD_POOL_HPP__
#define MSTL_CORE_ASYNC_THREAD_POOL_HPP__
#include "MSTL/core/async/packaged_task.hpp"
#include "MSTL/core/async/timer.hpp"
#include "MSTL/core/container/array.hpp"
#include "MSTL/core/container/priority_queue.hpp"
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/system/sysinfo.hpp"
#include "MSTL/core/time/datetime.hpp"
#include "MSTL/core/utility/optional.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr size_t THREAD_POOL_TASK_MAX_THRESHHOLD = numeric_traits<int32_t>::max();
MSTL_INLINE17 constexpr size_t THREAD_POOL_MAX_IDLE_SECONDS = 60;
MSTL_INLINE17 constexpr size_t THREAD_POOL_LOCAL_QUEUE_SIZE = 256;
static const size_t THREAD_POOL_THREAD_MAX_THRESHHOLD = sysinfo::instance().get_system_info().processor_numbers;


enum class THREAD_POOL_MODE : uint8_t {
	MODE_FIXED, MODE_CACHED
};

enum class STEAL_STRATEGY {
	HALF,
	FIXED_BATCH,
	SINGLE,
	ADAPTIVE
};


MSTL_BEGIN_INNER__

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

MSTL_END_INNER__


struct task_group {
	task_group() = default;
	~task_group() = default;

	_MSTL atomic<size_t> running_count{0};

	void increment() noexcept {
		running_count.fetch_add(1, _MSTL memory_order_relaxed);
	}

	void decrement() noexcept {
		if (running_count.fetch_sub(1, _MSTL memory_order_release) == 1) {
			running_count.notify_all();
		}
	}

	void wait() const noexcept {
		size_t count = running_count.load(_MSTL memory_order_acquire);
		while (count != 0) {
			running_count.wait(count);
			count = running_count.load(_MSTL memory_order_acquire);
		}
	}
};


class MSTL_API local_queue {
private:
	static STEAL_STRATEGY steal_strategy_;
	static uint32_t fixed_batch_size_;

	_MSTL array<_MSTL function<void()>, THREAD_POOL_LOCAL_QUEUE_SIZE> tasks_{};
	_MSTL atomic<uint64_t> head_{0};
	_MSTL atomic<uint32_t> tail_{0};

private:
	constexpr static size_t mask_ = THREAD_POOL_LOCAL_QUEUE_SIZE - 1;

	MSTL_NODISCARD static uint64_t pack(const uint32_t steal, const uint32_t local_head) noexcept {
		return static_cast<uint64_t>(steal) << 32 | static_cast<uint64_t>(local_head);
	}

	MSTL_NODISCARD static pair<uint32_t, uint32_t> unpack(const uint64_t head) noexcept {
		return {static_cast<uint32_t>(head >> 32), static_cast<uint32_t>(head)};
	}

	uint32_t be_stolen_by_impl(local_queue& dst, uint32_t dst_tail);

public:
	local_queue() = default;
	~local_queue() = default;
	local_queue(const local_queue&) = delete;
	local_queue& operator =(const local_queue&) = delete;
	local_queue(local_queue&& other) noexcept;
	local_queue& operator =(local_queue&& other) noexcept;

	MSTL_NODISCARD size_t capacity() const noexcept { return tasks_.size(); }
	MSTL_NODISCARD bool empty() const noexcept { return size() == 0u; }

	MSTL_NODISCARD size_t remain_size() const noexcept {
		const auto tail = tail_.load(_MSTL memory_order_acquire);
		const auto head = head_.load(_MSTL memory_order_acquire);
		const auto steal = unpack(head).first;
		const size_t used = static_cast<size_t>(tail - steal);
		const size_t remain = capacity() - used;
		return remain;
	}
	MSTL_NODISCARD size_t size() const noexcept {
		const auto tail = tail_.load(_MSTL memory_order_acquire);
		const auto head = head_.load(_MSTL memory_order_acquire);
		const auto local_head = unpack(head).second;
		return static_cast<size_t>(tail - local_head);
	}

	static void set_steal_strategy(const STEAL_STRATEGY strategy, const uint32_t batch_size = 4) {
		steal_strategy_ = strategy;
		fixed_batch_size_ = batch_size;
	}

	void push_back(_MSTL function<void()> task) {
		const uint32_t tail = tail_.load(_MSTL memory_order_relaxed);
		tasks_[tail & mask_] = _MSTL move(task);
		tail_.store(tail + 1, _MSTL memory_order_release);
	}

	_MSTL optional<_MSTL function<void()>> try_pop();

	_MSTL optional<_MSTL function<void()>> be_stolen_by(local_queue& dst_queue);
};


struct MSTL_API worker_context {
    using id_type = _INNER manual_thread::id_type;

	local_queue queue{};
	id_type id{0};
	_MSTL atomic<bool> is_stealing{false};
    size_t consecutive_idle_count = 0;

	worker_context() = default;
	worker_context(const worker_context&) = delete;
	worker_context& operator =(const worker_context&) = delete;
	worker_context(worker_context&& other) noexcept;
	worker_context& operator =(worker_context&& other) noexcept;
};


enum class TASK_STATUS {
	PENDING,
	RUNNING,
	COMPLETED,
	FAILED
};

MSTL_CONSTEXPR20 string to_string(const TASK_STATUS status) {
	switch (status) {
		case TASK_STATUS::PENDING: return "PENDING";
		case TASK_STATUS::RUNNING: return "RUNNING";
		case TASK_STATUS::COMPLETED: return "COMPLETED";
		case TASK_STATUS::FAILED: return "FAILED";
		default: MSTL_UNREACHABLE;
	}
}


struct task_info {
	using priority_type = uint32_t;

	const uint64_t id;
	atomic<TASK_STATUS> status{TASK_STATUS::PENDING};
	timestamp submit_time{timestamp::now()};
	timestamp start_time{0};
	timestamp finish_time{0};
	_INNER manual_thread::id_type worker_thread_id{0};
	string error{};
	priority_type priority;

	explicit task_info(const uint64_t task_id, const priority_type priority)
	: id(task_id), priority(priority) {}

	MSTL_NODISCARD bool is_finished() const noexcept {
		const auto s = status.load(_MSTL memory_order_acquire);
		return s == TASK_STATUS::COMPLETED || s == TASK_STATUS::FAILED;
	}

	MSTL_NODISCARD int64_t exec_time() const noexcept {
		if (start_time.value() == 0 || finish_time.value() == 0) {
			return -1;
		}
		return finish_time - start_time;
	}
};

using task_info_ptr = _MSTL shared_ptr<task_info>;


template <typename T>
struct submit_result {
	_MSTL future<T> future;
	task_info_ptr task_info;

	MSTL_NODISCARD explicit operator bool() const noexcept {
		return future.valid() && task_info;
	}
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
		size_t total_stolen;
		size_t total_completed;

		MSTL_NODISCARD string to_string() const;
	};

    using id_type = _INNER manual_thread::id_type;
	using periodic_token = shared_ptr<periodic_task_state>;
	using priority_type = task_info::priority_type;

private:
	using Task = _MSTL function<void()>;

	struct priority_task {
		Task task;
		priority_type priority;
		task_info_ptr task_info;

		priority_task(Task t, const priority_type p, task_info_ptr info) noexcept
		: task(_MSTL move(t)), priority(p), task_info(_MSTL move(info)) {}

		bool operator <(const priority_task& other) const noexcept {
			return priority < other.priority;
		}
	};

	_MSTL unordered_map<id_type, _MSTL unique_ptr<_INNER manual_thread>> threads_map_;
	_MSTL unordered_map<id_type, worker_context> worker_contexts_;
	_MSTL vector<_MSTL atomic<worker_context*>> worker_contexts_ptr_;
	_MSTL mutex worker_contexts_mtx_;

	_MSTL timer_scheduler<steady_clock> timer_{};

	id_type init_thread_size_{0};
	size_t thread_threshhold_{THREAD_POOL_THREAD_MAX_THRESHHOLD};

	_MSTL priority_queue<priority_task> task_queue_{};
	_MSTL atomic_uint task_size_{0};
	_MSTL atomic_uint idle_thread_size_{0};
	size_t task_threshhold_{THREAD_POOL_TASK_MAX_THRESHHOLD};

	_MSTL mutex task_queue_mtx_{};
	_MSTL condition_variable not_full_{};
	_MSTL condition_variable not_empty_{};
	_MSTL condition_variable exit_cond_{};

	_MSTL atomic<THREAD_POOL_MODE> pool_mode_{THREAD_POOL_MODE::MODE_FIXED};
	_MSTL atomic_bool is_running_{false};

	_MSTL atomic_size_t total_submitted_tasks_{0};
	_MSTL atomic_size_t total_completed_tasks_{0};
	_MSTL atomic_size_t total_stolen_tasks_{0};

	_MSTL atomic_size_t steal_worker_count_{0};
	_MSTL atomic_uint64_t next_task_id_{0};

private:
	uint64_t generate_task_id() {
		return next_task_id_.fetch_add(1, _MSTL memory_order_relaxed);
	}

    void thread_function(id_type thread_id);
	_MSTL optional<Task> try_steal_task(worker_context& ctx);

	pool_statistics statistics_unsafe() const;

public:
    thread_pool();
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator =(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator =(thread_pool&&) = delete;

    bool set_mode(THREAD_POOL_MODE mode) noexcept;
	bool set_steal_mode(STEAL_STRATEGY strategy, uint32_t steal_batch = 4) noexcept;
    bool set_task_threshhold(size_t threshhold) noexcept;
    bool set_thread_threshhold(size_t threshhold) noexcept;

    MSTL_NODISCARD static size_t max_thread_size() noexcept { return THREAD_POOL_THREAD_MAX_THRESHHOLD; }
    MSTL_NODISCARD bool running() const noexcept { return is_running_; }
    MSTL_NODISCARD THREAD_POOL_MODE mode() const noexcept { return pool_mode_; }
	MSTL_NODISCARD pool_statistics statistics() const;

    bool start(size_t init_thread_size = 3);
    pool_statistics stop();

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	submit_result<invoke_result_t<Func, Args...>> submit_task(priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	submit_result<invoke_result_t<Func, Args...>> submit_task(Func&& func, Args&&... args) {
		return this->submit_task(0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
	}

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	submit_result<invoke_result_t<Func, Args...>> submit_after(int64_t delay_ms, priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	submit_result<invoke_result_t<Func, Args...>> submit_after(int64_t delay_ms, Func&& func, Args&&... args) {
		return this->submit_after(delay_ms, 0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
	}

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	periodic_token submit_every(int64_t interval_ms, priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	periodic_token submit_every(int64_t interval_ms, Func&& func, Args&&... args) {
		return this->submit_every(interval_ms, 0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
	}

	static void cancel_periodic_task(const periodic_token& token) {
		if (token) token->cancelled.store(true);
	}

	template <typename... Types>
	static tuple<future_result_t<Types>...> wait(future<Types>&&... futures) {
		return _MSTL make_tuple(_MSTL get(futures)...);
	}
};

#ifdef MSTL_COMPILER_MSVC__
MSTL_ALWAYS_INLINE_INLINE MSTL_API worker_context*& get_worker_context() noexcept;
MSTL_ALWAYS_INLINE_INLINE MSTL_API _MSTL shared_ptr<task_group>& get_current_task_group() noexcept;
#else
extern thread_local worker_context* t_worker_ctx;
extern thread_local _MSTL shared_ptr<task_group> t_current_task_group;
MSTL_ALWAYS_INLINE_INLINE worker_context*& get_worker_context() noexcept { return t_worker_ctx; }
MSTL_ALWAYS_INLINE_INLINE _MSTL shared_ptr<task_group>& get_current_task_group() noexcept { return t_current_task_group; }
#endif


template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
submit_result<invoke_result_t<Func, Args...>>
thread_pool::submit_task(const priority_type priority, Func&& func, Args&&... args) {
	using Result = decltype(func(_MSTL forward<Args>(args)...));

	auto info = _MSTL make_shared<task_info>(generate_task_id(), priority);

	const auto current_group = get_current_task_group();
	if (current_group) {
		current_group->increment();
	}

	auto task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
		[func = _MSTL forward<Func>(func),
		 args = _MSTL make_tuple(_MSTL forward<Args>(args)...),
		 group = current_group,
		 info]() mutable -> Result {
		 	struct context_guard {
				task_info_ptr info;
		 		_MSTL shared_ptr<task_group> group_inner;
		 		_MSTL shared_ptr<task_group> prev_group_inner;

		 		explicit context_guard(task_info_ptr i, _MSTL shared_ptr<task_group> g)
		 		: info(move(i)), group_inner(move(g)) {
				 	info->status.store(TASK_STATUS::RUNNING, _MSTL memory_order_release);
				 	info->start_time = timestamp::now();
				 	info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;

				 	prev_group_inner = get_current_task_group();
		 			get_current_task_group() = group_inner;
		 		}

		 		~context_guard() noexcept {
		 			try {
		 				info->finish_time = timestamp::now();
						auto expected = TASK_STATUS::RUNNING;
					       info->status.compare_exchange_strong(expected,
						       TASK_STATUS::COMPLETED, _MSTL memory_order_release);

		 				get_current_task_group() = prev_group_inner;
						if (group_inner) group_inner->decrement();
		 			} catch (...) {
		 				/* ignore */
		 			}
		 		}
		 	};

			context_guard guard(info, group);
			try {
				return _MSTL apply(func, args);
			} catch (const exception& e) {
				info->status.store(TASK_STATUS::FAILED, _MSTL memory_order_release);
				info->error = e.what();
				throw;
			} catch (...) {
				info->status.store(TASK_STATUS::FAILED, _MSTL memory_order_release);
				info->error = "Unknown exception";
				throw;
			}
		}
	);

	_MSTL future<Result> res = task->get_future();
	Task job([task] { (*task)(); });

	if (priority > 0) {
		_MSTL smart_lock<_MSTL mutex> lock(task_queue_mtx_);

		if (!not_full_.wait_for(lock, seconds(1), [&]()->bool {
			return task_queue_.size() < task_threshhold_;
		})) {
			info->status.store(TASK_STATUS::FAILED, _MSTL memory_order_release);
			info->error = "Task queue is full";

			auto dummy_task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
				[]() -> Result { return Result(); });
			(*dummy_task)();
			return submit_result<Result>{dummy_task->get_future(), info};
		}

		task_queue_.emplace(_MSTL move(job), priority, info);
		++task_size_;
		++total_submitted_tasks_;
		not_empty_.notify_one();

	} else {
		auto* ctx = get_worker_context();

		if (ctx != nullptr && ctx->queue.remain_size() > 0) {
			ctx->queue.push_back(move(job));
			++total_submitted_tasks_;
		} else {
			_MSTL smart_lock<_MSTL mutex> lock(task_queue_mtx_);
			if (!not_full_.wait_for(lock, seconds(1), [&]()->bool {
				return task_queue_.size() < task_threshhold_;
			})) {
				info->status.store(TASK_STATUS::FAILED, _MSTL memory_order_release);
				info->error = "Task queue is full";

				auto dummy_task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
					[]() -> Result { return Result(); });
				(*dummy_task)();
				return submit_result<Result>{dummy_task->get_future(), info};
			}

			task_queue_.emplace(_MSTL move(job), 0, info);
			++task_size_;
			++total_submitted_tasks_;
			not_empty_.notify_one();
		}
	}

	if (pool_mode_.load() == THREAD_POOL_MODE::MODE_CACHED
		&& task_size_.load() > idle_thread_size_
		&& threads_map_.size() < thread_threshhold_) {

		auto ptr = _MSTL make_unique<_INNER manual_thread>(
			[this](const id_type id) {
				thread_function(id);
			});
		id_type thread_id = ptr->id();

	    if (thread_id >= worker_contexts_ptr_.size()) {
	        worker_contexts_ptr_.reserve(thread_id + 1);
	        for (size_t i = worker_contexts_ptr_.size() - 1; i <= thread_id; i++) {
	            atomic<worker_context*> tmp;
	            tmp.store(nullptr, memory_order_relaxed);
	            worker_contexts_ptr_.emplace_back(move(tmp));
	        }
	    }

		threads_map_.emplace(thread_id, _MSTL move(ptr));
		threads_map_[thread_id]->start();
		++idle_thread_size_;
	}

	return submit_result<Result>{_MSTL move(res), info};
}

template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
submit_result<invoke_result_t<Func, Args...>>
thread_pool::submit_after(const int64_t delay_ms, const priority_type priority, Func&& func, Args&&... args) {
	using Result = decltype(func(args...));

	auto info = _MSTL make_shared<task_info>(generate_task_id(), priority);

	auto task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
		[func = _MSTL forward<Func>(func),
		 tup = _MSTL make_tuple(_MSTL forward<Args>(args)...),
		 info]() mutable {
			struct context_guard {
				task_info_ptr info;

		 		explicit context_guard(task_info_ptr i) : info(move(i)) {
				 	info->status.store(TASK_STATUS::RUNNING, _MSTL memory_order_release);
				 	info->start_time = timestamp::now();
				 	info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;
		 		}

		 		~context_guard() noexcept {
				 	info->finish_time = timestamp::now();
				 	auto expected = TASK_STATUS::RUNNING;
					info->status.compare_exchange_strong(expected,
						TASK_STATUS::COMPLETED, _MSTL memory_order_release);
		 		}
		 	};

			context_guard guard(info);

			try {
				return _MSTL apply(func, tup);
			} catch (const _MSTL exception& e) {
				info->status.store(TASK_STATUS::FAILED, _MSTL memory_order_release);
				info->error = e.what();
				throw;
			}
		});

	_MSTL future<Result> res = task->get_future();

	auto expire_time = steady_clock::now() + milliseconds(delay_ms);
	timer_.add_task(expire_time, [this, task = _MSTL move(task), priority]() mutable {
		this->submit_task(priority, [task]() {
			(*task)();
	    });
	});

	return submit_result<Result>{_MSTL move(res), info};
}

template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
thread_pool::periodic_token thread_pool::submit_every(int64_t interval_ms, const priority_type priority, Func &&func, Args &&...args) {
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
        auto next_time = steady_clock::now() + milliseconds(interval_ms);
        timer_.add_task(next_time, [handler_ptr]() { (*handler_ptr)(); });
    };

    auto first_time = steady_clock::now() + milliseconds(interval_ms);
    timer_.add_task(first_time, [handler_ptr]() { (*handler_ptr)(); });
    return state;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_THREAD_POOL_HPP__
