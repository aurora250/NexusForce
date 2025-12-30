#ifndef MSTL_CORE_ASYNC_THREAD_POOL_HPP__
#define MSTL_CORE_ASYNC_THREAD_POOL_HPP__
#include "MSTL/core/container/array.hpp"
#include "MSTL/core/container/priority_queue.hpp"
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/interface/istringify.hpp"
#include "MSTL/core/async/condition_variable.hpp"
#include "MSTL/core/async/packaged_task.hpp"
#include "MSTL/core/async/timer.hpp"
#include "MSTL/core/utility/optional.hpp"
#include "MSTL/core/config/undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr size_t THREAD_POOL_TASK_MAX_THRESHHOLD = numeric_limits<int32_t>::max();
MSTL_INLINE17 constexpr int64_t THREAD_POOL_MAX_IDLE_SECONDS = 60;

static const size_t THREAD_POOL_THREAD_MAX_THRESHHOLD = _MSTL thread::hardware_concurrency();


enum class THREAD_POOL_MODE : uint8_t {
	MODE_FIXED, MODE_CACHED
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


template <size_t CAPACITY = 256>
class local_queue {
	static_assert((CAPACITY & (CAPACITY - 1)) == 0, "CAPACITY must be power of 2");
	static_assert(CAPACITY > 0, "CAPACITY must be greater than 0");

public:
	local_queue() = default;
	~local_queue() = default;

	local_queue(local_queue&& other) noexcept
	: tasks_(_MSTL move(other.tasks_))
    , head_(other.head_.load(_MSTL memory_order_relaxed))
    , tail_(other.tail_.load(_MSTL memory_order_relaxed)) {}

	local_queue& operator =(local_queue&& other) noexcept {
		if (this != &other) {
			tasks_ = _MSTL move(other.tasks_);
			head_.store(other.head_.load(_MSTL memory_order_relaxed), _MSTL memory_order_relaxed);
			tail_.store(other.tail_.load(_MSTL memory_order_relaxed), _MSTL memory_order_relaxed);
		}
		return *this;
	}

	MSTL_NODISCARD size_t capacity() const noexcept { return CAPACITY; }

	MSTL_NODISCARD size_t remain_size() const noexcept {
		const auto tail = tail_.load(_MSTL memory_order_acquire);
		const auto head = head_.load(_MSTL memory_order_acquire);
		auto [steal, local_head] = unpack(head);
		return CAPACITY - static_cast<uint64_t>(tail - steal);
	}

	MSTL_NODISCARD size_t size() const noexcept {
		const auto tail = tail_.load(_MSTL memory_order_acquire);
		const auto head = head_.load(_MSTL memory_order_acquire);
		auto [steal, local_head] = unpack(head);
		return static_cast<size_t>(tail - local_head);
	}

	MSTL_NODISCARD bool empty() const noexcept {
		return size() == 0u;
	}

	void push_back(_MSTL function<void()> task) {
		const uint32_t tail = tail_.load(_MSTL memory_order_relaxed);
		tasks_[tail & mask_] = _MSTL move(task);
		tail_.store(tail + 1, _MSTL memory_order_release);
	}

	_MSTL optional<_MSTL function<void()>> try_pop() {
		auto cur_head = head_.load(_MSTL memory_order_acquire);
		size_t index = 0;
		while (true) {
			auto [cur_steal, cur_local_head] = unpack(cur_head);
			const auto tail = tail_.load(_MSTL memory_order_acquire);
			if (cur_local_head == tail) {
				return _MSTL nullopt;
			}
			const auto next_local_head = cur_local_head + 1;
			const auto next_head = (cur_local_head == cur_steal)
				? pack(next_local_head, next_local_head)
				: pack(cur_steal, next_local_head);

			if (head_.compare_exchange_weak(cur_head, next_head,
				_MSTL memory_order_acq_rel, _MSTL memory_order_acquire)) {
				index = static_cast<size_t>(cur_local_head) & mask_;
				break;
			}
		}
		auto task = _MSTL move(tasks_[index]);
		tasks_[index] = nullptr;
		return task;
	}

	_MSTL optional<_MSTL function<void()>> be_stolen_by(local_queue& dst_queue) {
		_MSTL optional<_MSTL function<void()>> result{_MSTL nullopt};
		auto [dst_steal, dst_local_head] = unpack(dst_queue.head_.load(_MSTL memory_order_acquire));
		auto dst_tail = dst_queue.tail_.load(_MSTL memory_order_acquire);

		if (dst_tail - dst_steal > static_cast<uint32_t>(CAPACITY) / 2) {
			return result;
		}

		auto steal_num = be_stolen_by_impl(dst_queue, dst_tail);
		if (steal_num == 0) return result;

		steal_num = steal_num - 1;
		auto next_dst_tail = dst_tail + steal_num;
		auto idx = static_cast<size_t>(next_dst_tail) & mask_;
		result.emplace(_MSTL move(dst_queue.tasks_[idx]));

		if (steal_num > 0) {
			dst_queue.tail_.store(next_dst_tail, _MSTL memory_order_release);
		}
		return result;
	}

private:
	constexpr static inline size_t mask_ = CAPACITY - 1;

	MSTL_NODISCARD static uint64_t pack(const uint32_t steal, const uint32_t local_head) noexcept {
		return static_cast<uint64_t>(steal) << 32 | static_cast<uint64_t>(local_head);
	}

	MSTL_NODISCARD static pair<uint32_t, uint32_t> unpack(const uint64_t head) noexcept {
		return {static_cast<uint32_t>(head >> 32), static_cast<uint32_t>(head)};
	}

	uint32_t be_stolen_by_impl(local_queue& dst, const uint32_t dst_tail) {
		uint64_t cur_src_head = head_.load(_MSTL memory_order_acquire);
		uint64_t next_src_head = 0;
		uint32_t steal_num = 0;

		while (true) {
			auto [cur_src_steal, cur_src_local_head] = unpack(cur_src_head);
			const auto cur_src_tail = tail_.load(_MSTL memory_order_acquire);
			const auto cur_src_size = cur_src_tail - cur_src_local_head;

			if (cur_src_steal != cur_src_local_head) return 0;

			steal_num = cur_src_size / 2;
			if (steal_num == 0) return 0;

			const auto next_src_local_head = cur_src_local_head + steal_num;
			next_src_head = pack(cur_src_steal, next_src_local_head);

			if (head_.compare_exchange_weak(cur_src_head, next_src_head,
				_MSTL memory_order_acq_rel, _MSTL memory_order_acquire)) {
				break;
			}
		}

		auto [next_src_steal, next_src_local_head] = unpack(next_src_head);
		for (uint32_t i = 0; i < steal_num; i++) {
			auto src_idx = static_cast<uint32_t>(next_src_steal + i) & mask_;
			auto dst_idx = static_cast<uint32_t>(dst_tail + i) & mask_;
			dst.tasks_[dst_idx] = _MSTL move(tasks_[src_idx]);
		}

		cur_src_head = next_src_head;
		while (true) {
			auto [cur_src_steal, cur_src_local_head] = unpack(cur_src_head);
			next_src_head = pack(cur_src_local_head, cur_src_local_head);
			if (head_.compare_exchange_weak(cur_src_head, next_src_head,
				_MSTL memory_order_acq_rel, _MSTL memory_order_acquire)) {
				return steal_num;
			}
		}
	}

private:
	_MSTL array<_MSTL function<void()>, CAPACITY> tasks_{};
	_MSTL atomic<uint64_t> head_{0};
	_MSTL atomic<uint32_t> tail_{0};
};


struct worker_context {
    using id_type = manual_thread::id_type;

	worker_context() = default;
	worker_context(const worker_context&) = delete;
	worker_context& operator =(const worker_context&) = delete;

	worker_context(worker_context&& other) noexcept
	: queue(_MSTL move(other.queue))
	, id(other.id)
	, is_stealing(other.is_stealing.load(_MSTL memory_order_relaxed)) {}

	worker_context& operator =(worker_context&& other) noexcept {
		if (this != &other) {
			queue = _MSTL move(other.queue);
			id = other.id;
			is_stealing.store(other.is_stealing.load(
				_MSTL memory_order_relaxed), _MSTL memory_order_relaxed);
		}
		return *this;
	}

	local_queue<> queue{};
	id_type id{0};
	_MSTL atomic<bool> is_stealing{false};
};

MSTL_END_INNER__


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

		MSTL_NODISCARD string to_string() const;
	};

    using id_type = _INNER manual_thread::id_type;
	using periodic_token = shared_ptr<periodic_task_state>;
	using priority_type = uint32_t;

private:
	using Task = _MSTL function<void()>;

	struct priority_task {
		Task task;
		priority_type priority;

		priority_task(Task t, const priority_type p) noexcept
		: task(_MSTL move(t)), priority(p) {}

		bool operator <(const priority_task& other) const noexcept {
			return priority < other.priority;
		}
	};

	_MSTL unordered_map<id_type, _MSTL unique_ptr<_INNER manual_thread>> threads_map_;
	_MSTL unordered_map<id_type, _INNER worker_context> worker_contexts_;
	_MSTL vector<_MSTL atomic<_INNER worker_context*>> worker_contexts_ptr_;
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
	_MSTL atomic_size_t steal_worker_count_{0};

private:
    void thread_function(id_type thread_id);
	_MSTL optional<Task> try_steal_task(_INNER worker_context& ctx);

public:
    thread_pool();
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator =(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator =(thread_pool&&) = delete;

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
	decltype(auto) submit_task(priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_task(Func&& func, Args&&... args) {
		return this->submit_task(0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
	}

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_after(int64_t delay_ms, priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int> = 0>
	decltype(auto) submit_after(int64_t delay_ms, Func&& func, Args&&... args) {
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


MSTL_BEGIN_INNER__
static thread_local worker_context* t_worker_ctx = nullptr;
static thread_local _MSTL shared_ptr<task_group> t_current_task_group = nullptr;
MSTL_END_INNER__


template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
decltype(auto) thread_pool::submit_task(const priority_type priority, Func&& func, Args&&... args) {
	using Result = decltype(func(_MSTL forward<Args>(args)...));

	const auto current_group = _INNER t_current_task_group;
	if (current_group) {
		current_group->increment();
	}

	auto task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
		[func = _MSTL forward<Func>(func),
		 args = _MSTL make_tuple(_MSTL forward<Args>(args)...),
		 group = current_group]() mutable {
		 	struct context_guard {
				 _MSTL shared_ptr<_INNER task_group> group_inner;
				 _MSTL shared_ptr<_INNER task_group> prev_group_inner;

				 explicit context_guard(_MSTL shared_ptr<_INNER task_group> g) : group_inner(move(g)) {
					 prev_group_inner = _INNER t_current_task_group;
					 _INNER t_current_task_group = group_inner;
				 }

				 ~context_guard() noexcept {
					 _INNER t_current_task_group = prev_group_inner;
					 if (group_inner) group_inner->decrement();
				 }
		 	};

			context_guard guard(group);
			return _MSTL apply(func, args);
		}
	);

	_MSTL future<Result> res = task->get_future();
	Task job([task] { (*task)(); });

	if (priority > 0) {
		// 高优先级任务提交到全局队列
		_MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);
		if (!not_full_.wait_for(lock, seconds(1), [&]()->bool {
			return task_queue_.size() < task_threshhold_;
		})) {
			auto dummy_task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
				[]() -> Result { return Result(); });
			(*dummy_task)();
			return dummy_task->get_future();
		}

		task_queue_.emplace(_MSTL move(job), priority);
		++task_size_;
		++total_submitted_tasks_;
		not_empty_.notify_all();
	} else {
		// 普通优先级任务
		if (_INNER t_worker_ctx != nullptr && _INNER t_worker_ctx->queue.remain_size() > 0) {
			// 当前线程是工作线程且本地队列未满，提交到本地队列
			_INNER t_worker_ctx->queue.push_back(_MSTL move(job));
			++total_submitted_tasks_;
		} else {
			// 否则提交到全局队列
			_MSTL unique_lock<_MSTL mutex> lock(task_queue_mtx_);
			if (!not_full_.wait_for(lock, seconds(1), [&]()->bool {
				return task_queue_.size() < task_threshhold_;
			})) {
				auto dummy_task = _MSTL make_shared<_MSTL packaged_task<Result()>>(
					[]() -> Result { return Result(); });
				(*dummy_task)();
				return dummy_task->get_future();
			}

			task_queue_.emplace(priority_task(_MSTL move(job), 0));
			++task_size_;
			++total_submitted_tasks_;
			not_empty_.notify_all();
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

		threads_map_.emplace(thread_id, _MSTL move(ptr));
		threads_map_[thread_id]->start();
		++idle_thread_size_;
	}
	return res;
}

template <typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
decltype(auto) thread_pool::submit_after(const int64_t delay_ms, const priority_type priority, Func&& func, Args&&... args) {
	using ResultType = decltype(func(args...));
	auto task = _MSTL make_shared<_MSTL packaged_task<ResultType()>>(
		[func = _MSTL forward<Func>(func), tup = _MSTL make_tuple(_MSTL forward<Args>(args)...)]() mutable {
			return _MSTL apply(func, tup);
		});
	_MSTL future<ResultType> res = task->get_future();

	auto expire_time = steady_clock::now() + milliseconds(delay_ms);
	timer_.add_task(expire_time, [this, task = _MSTL move(task), priority]() mutable {
		this->submit_task(priority, [task]() {
			(*task)();
	    });
	});
	return res;
}

template<typename Func, typename... Args, enable_if_t<is_invocable_v<Func, Args...>, int>>
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
