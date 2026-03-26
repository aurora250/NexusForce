#ifndef NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__
#include "NeForce/core/async/packaged_task.hpp"
#include "NeForce/core/async/timer.hpp"
#include "NeForce/core/container/priority_queue.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_BEGIN_INNER__

class NEFORCE_API manual_thread {
public:
    using id_type = uint32_t;

private:
	using thread_func = function<void(id_type)>;

	thread_func func_;
	id_type thread_id_;

public:
	explicit manual_thread(thread_func&& func) noexcept;
	~manual_thread() = default;

    NEFORCE_NODISCARD id_type id() const noexcept { return thread_id_; }
    void start();
};

NEFORCE_END_INNER__


struct task_group {
	task_group() = default;
	~task_group() = default;

	atomic<size_t> running_count{0};

	void increment() noexcept {
		running_count.fetch_add(1, memory_order_relaxed);
	}

	void decrement() noexcept {
		if (running_count.fetch_sub(1, memory_order_release) == 1) {
			running_count.notify_all();
		}
	}

	void wait() const noexcept {
		size_t count = running_count.load(memory_order_acquire);
		while (count != 0) {
			running_count.wait(count);
			count = running_count.load(memory_order_acquire);
		}
	}
};


class NEFORCE_API local_queue {
public:
	enum class steal_strategy : uint8_t {
		half,
		fixed_batch,
		single,
		adaptive
	};

	static constexpr size_t queue_size = 256;

private:
	static steal_strategy steal_strategy_;
	static uint32_t fixed_batch_size_;

	array<function<void()>, queue_size> tasks_{};
	atomic<uint64_t> head_{0};
	atomic<uint32_t> tail_{0};

private:
	constexpr static size_t mask_ = queue_size - 1;

	NEFORCE_NODISCARD static uint64_t pack(const uint32_t steal, const uint32_t local_head) noexcept {
		return static_cast<uint64_t>(steal) << 32 | static_cast<uint64_t>(local_head);
	}

	NEFORCE_NODISCARD static pair<uint32_t, uint32_t> unpack(const uint64_t head) noexcept {
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

	NEFORCE_NODISCARD size_t capacity() const noexcept { return tasks_.size(); }
	NEFORCE_NODISCARD bool empty() const noexcept { return size() == 0u; }

	NEFORCE_NODISCARD size_t remain_size() const noexcept {
		const auto tail = tail_.load(memory_order_acquire);
		const auto head = head_.load(memory_order_acquire);
		const auto steal = unpack(head).first;
		const size_t used = static_cast<size_t>(tail - steal);
		const size_t remain = capacity() - used;
		return remain;
	}
	NEFORCE_NODISCARD size_t size() const noexcept {
		const auto tail = tail_.load(memory_order_acquire);
		const auto head = head_.load(memory_order_acquire);
		const auto local_head = unpack(head).second;
		return static_cast<size_t>(tail - local_head);
	}

	static void set_steal_strategy(const steal_strategy strategy, const uint32_t batch_size = 4) {
		steal_strategy_ = strategy;
		fixed_batch_size_ = batch_size;
	}

	void push_back(function<void()> task) {
		const uint32_t tail = tail_.load(memory_order_relaxed);
		tasks_[tail & mask_] = move(task);
		tail_.store(tail + 1, memory_order_release);
	}

	optional<function<void()>> try_pop();

	optional<function<void()>> be_stolen_by(local_queue& dst_queue);
};


struct NEFORCE_API worker_context {
    using id_type = inner::manual_thread::id_type;

	local_queue queue{};
	id_type id{0};
	atomic<bool> is_stealing{false};
    size_t consecutive_idle_count = 0;

	worker_context() = default;
	worker_context(const worker_context&) = delete;
	worker_context& operator =(const worker_context&) = delete;
	worker_context(worker_context&& other) noexcept;
	worker_context& operator =(worker_context&& other) noexcept;
};


struct task_info {
	enum class status {
		pending,
		running,
		completed,
		failed
	};

	enum class priority_type : uint32_t {};

	const uint64_t id;
	atomic<status> status{status::pending};
	timestamp submit_time{timestamp::now()};
	timestamp start_time{0};
	timestamp finish_time{0};
	inner::manual_thread::id_type worker_thread_id{0};
	string error{};
	priority_type priority;

	explicit task_info(const uint64_t task_id, const priority_type priority)
	: id(task_id), priority(priority) {}

	NEFORCE_NODISCARD bool is_finished() const noexcept {
		const auto s = status.load(memory_order_acquire);
		return s == status::completed || s == status::failed;
	}

	NEFORCE_NODISCARD int64_t exec_time() const noexcept {
		if (start_time.value() == 0 || finish_time.value() == 0) {
			return -1;
		}
		return finish_time - start_time;
	}
};


template <typename T>
struct submit_result {
	_NEFORCE future<T> future;
	shared_ptr<_NEFORCE task_info> task_info;

	NEFORCE_NODISCARD explicit operator bool() const noexcept {
		return future.valid() && task_info;
	}
};


class NEFORCE_API thread_pool {
public:
	enum class pool_mode : uint8_t {
		fixed,
		cached
	};

	struct periodic_task_state {
		atomic<bool> cancelled{false};
	};

	struct NEFORCE_API pool_statistics : istringify<pool_statistics> {
		size_t total_threads;
		size_t idle_threads;
		size_t busy_threads;
		size_t queue_size;
		size_t total_submitted;
		size_t total_stolen;
		size_t total_completed;

		NEFORCE_NODISCARD string to_string() const;
	};

	using steal_strategy = local_queue::steal_strategy;
    using id_type = inner::manual_thread::id_type;
	using periodic_token = shared_ptr<periodic_task_state>;
	using priority_type = task_info::priority_type;

	static constexpr size_t task_max_threshhold = numeric_traits<int32_t>::max();
	static constexpr size_t max_idle_seconds = 60;
	static const size_t max_threshhold;

private:
	using task_type = function<void()>;

	struct priority_task {
		task_type task;
		priority_type priority;
		shared_ptr<task_info> info;

		priority_task(task_type t, const priority_type p, shared_ptr<task_info> info) noexcept
		: task(move(t)), priority(p), info(_NEFORCE move(info)) {}

		bool operator <(const priority_task& other) const noexcept {
			return priority < other.priority;
		}
	};

	unordered_map<id_type, unique_ptr<inner::manual_thread>> threads_map_;
	unordered_map<id_type, worker_context> worker_contexts_;
	vector<atomic<worker_context*>> worker_contexts_ptr_;
	mutex worker_contexts_mtx_;

	timer_scheduler<steady_clock> timer_{};

	id_type init_thread_size_{0};
	size_t thread_threshhold_{max_threshhold};

	priority_queue<priority_task> task_queue_{};
	atomic<uint32_t> task_size_{0};
	atomic<uint32_t> idle_thread_size_{0};
	size_t task_threshhold_{task_max_threshhold};

	mutex task_queue_mtx_{};
	condition_variable not_full_{};
	condition_variable not_empty_{};
	condition_variable exit_cond_{};

	atomic<pool_mode> pool_mode_{pool_mode::fixed};
	atomic<bool> is_running_{false};

	atomic<size_t> total_submitted_tasks_{0};
	atomic<size_t> total_completed_tasks_{0};
	atomic<size_t> total_stolen_tasks_{0};

	atomic<size_t> steal_worker_count_{0};
	atomic<uint64_t> next_task_id_{0};

private:
	uint64_t generate_task_id() {
		return next_task_id_.fetch_add(1, memory_order_relaxed);
	}

    void thread_function(id_type thread_id);
	optional<task_type> try_steal_task(worker_context& ctx);

	pool_statistics statistics_unsafe() const;

public:
    thread_pool();
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator =(const thread_pool&) = delete;

    thread_pool(thread_pool&&) = default;
    thread_pool& operator =(thread_pool&&) = default;

    bool set_mode(pool_mode mode) noexcept;
	bool set_steal_mode(steal_strategy strategy, uint32_t steal_batch = 4) noexcept;
    bool set_task_threshhold(size_t threshhold) noexcept;
    bool set_thread_threshhold(size_t threshhold) noexcept;

    NEFORCE_NODISCARD static size_t max_thread_size() noexcept { return max_threshhold; }
    NEFORCE_NODISCARD bool running() const noexcept { return is_running_; }
    NEFORCE_NODISCARD pool_mode mode() const noexcept { return pool_mode_; }
	NEFORCE_NODISCARD pool_statistics statistics() const;

    bool start(size_t init_thread_size = 3);
    pool_statistics stop();

	template <typename Func, typename... Args>
	submit_result<invoke_result_t<Func, Args...>> submit_task(priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args>
	submit_result<invoke_result_t<Func, Args...>> submit_task(Func&& func, Args&&... args) {
		return this->submit_task(priority_type{0}, _NEFORCE forward<Func>(func), _NEFORCE forward<Args>(args)...);
	}

	template <typename Func, typename... Args>
	submit_result<invoke_result_t<Func, Args...>> submit_after(int64_t delay_ms, priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args>
	submit_result<invoke_result_t<Func, Args...>> submit_after(int64_t delay_ms, Func&& func, Args&&... args) {
		return this->submit_after(delay_ms, priority_type{0}, _NEFORCE forward<Func>(func), _NEFORCE forward<Args>(args)...);
	}

	template <typename Func, typename... Args>
	periodic_token submit_every(int64_t interval_ms, priority_type priority, Func&& func, Args&&... args);

	template <typename Func, typename... Args>
	periodic_token submit_every(int64_t interval_ms, Func&& func, Args&&... args) {
		return this->submit_every(interval_ms, priority_type{0}, _NEFORCE forward<Func>(func), _NEFORCE forward<Args>(args)...);
	}

	static void cancel_periodic_task(const periodic_token& token) {
		if (token) token->cancelled.store(true);
	}

	template <typename... Types>
	static tuple<future_result_t<Types>...> wait(future<Types>&&... futures) {
		return _NEFORCE make_tuple(_NEFORCE get(futures)...);
	}
};


NEFORCE_API worker_context*& get_worker_context() noexcept;

NEFORCE_API shared_ptr<task_group>& get_current_task_group() noexcept;


template <typename Func, typename... Args>
submit_result<invoke_result_t<Func, Args...>>
thread_pool::submit_task(const priority_type priority, Func&& func, Args&&... args) {
    static_assert(is_invocable_v<Func, Args...>, "Func must be invocable with Args");

	using Result = invoke_result_t<Func, Args...>;

	auto info = make_shared<task_info>(generate_task_id(), priority);

	const auto current_group = get_current_task_group();
	if (current_group) {
		current_group->increment();
	}

	auto task = _NEFORCE make_shared<packaged_task<Result()>>(
		[func = _NEFORCE forward<Func>(func),
		 args = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...),
		 group = current_group,
		 info]() mutable -> Result {
		 	struct context_guard {
				shared_ptr<task_info> info;
		 		shared_ptr<task_group> group_inner;
		 		shared_ptr<task_group> prev_group_inner;

		 		explicit context_guard(shared_ptr<task_info> i, shared_ptr<task_group> g)
		 		: info(move(i)), group_inner(move(g)) {
				 	info->status.store(task_info::status::running, memory_order_release);
				 	info->start_time = timestamp::now();
				 	info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;

				 	prev_group_inner = get_current_task_group();
		 			get_current_task_group() = group_inner;
		 		}

		 		~context_guard() noexcept {
		 			try {
		 				info->finish_time = timestamp::now();
						auto expected = task_info::status::running;
					       info->status.compare_exchange_strong(expected,
						       task_info::status::completed, memory_order_release);

		 				get_current_task_group() = prev_group_inner;
						if (group_inner) group_inner->decrement();
		 			} catch (...) {
		 				/* ignore */
		 			}
		 		}
		 	};

			context_guard guard(info, group);
			try {
				return _NEFORCE apply(func, args);
			} catch (const exception& e) {
				info->status.store(task_info::status::failed, memory_order_release);
				info->error = e.what();
				throw;
			} catch (...) {
				info->status.store(task_info::status::failed, memory_order_release);
				info->error = "Unknown exception";
				throw;
			}
		}
	);

	future<Result> res = task->get_future();
	task_type job([task] { (*task)(); });

	if (static_cast<uint32_t>(priority) > 0) {
		smart_lock<mutex> lock(task_queue_mtx_);

		if (!not_full_.wait_for(lock, seconds(1), [&]()->bool {
			return task_queue_.size() < task_threshhold_;
		})) {
			info->status.store(task_info::status::failed, memory_order_release);
			info->error = "Task queue is full";

			auto dummy_task = _NEFORCE make_shared<packaged_task<Result()>>(
				[]() -> Result { return Result(); });
			(*dummy_task)();
			return submit_result<Result>{dummy_task->get_future(), info};
		}

		task_queue_.emplace(move(job), priority, info);
		++task_size_;
		++total_submitted_tasks_;
		not_empty_.notify_one();

	} else {
		auto* ctx = get_worker_context();

		if (ctx != nullptr && ctx->queue.remain_size() > 0) {
			ctx->queue.push_back(move(job));
			++total_submitted_tasks_;
		} else {
			smart_lock<mutex> lock(task_queue_mtx_);
			if (!not_full_.wait_for(lock, seconds(1), [&]()->bool {
				return task_queue_.size() < task_threshhold_;
			})) {
				info->status.store(task_info::status::failed, memory_order_release);
				info->error = "Task queue is full";

				auto dummy_task = _NEFORCE make_shared<packaged_task<Result()>>(
					[]() -> Result { return Result(); });
				(*dummy_task)();
				return submit_result<Result>{dummy_task->get_future(), info};
			}

			task_queue_.emplace(move(job), priority_type{0}, info);
			++task_size_;
			++total_submitted_tasks_;
			not_empty_.notify_one();
		}
	}

	if (pool_mode_.load() == pool_mode::cached
		&& task_size_.load() > idle_thread_size_) {

		inner::manual_thread* t_ptr = nullptr;
		id_type thread_id = 0;

		{
			smart_lock<mutex> lock(task_queue_mtx_);
			if (threads_map_.size() < thread_threshhold_) {
				auto ptr = _NEFORCE make_unique<inner::manual_thread>(
					[this](const id_type id) {
						thread_function(id);
					});

				thread_id = ptr->id();
				t_ptr = ptr.get();
				threads_map_.emplace(thread_id, move(ptr));
			}
		}

		if (t_ptr != nullptr) {
			{
				lock<mutex> ctx_lock(worker_contexts_mtx_);
				if (thread_id >= worker_contexts_ptr_.size()) {
					worker_contexts_ptr_.reserve(thread_id + 1);
					for (size_t i = worker_contexts_ptr_.size(); i <= thread_id; i++) {
						atomic<worker_context*> tmp;
						tmp.store(nullptr, memory_order_relaxed);
						worker_contexts_ptr_.emplace_back(move(tmp));
					}
				}
			}

			t_ptr->start();
		}
	}

	return submit_result<Result>{move(res), move(info)};
}

template <typename Func, typename... Args>
submit_result<invoke_result_t<Func, Args...>>
thread_pool::submit_after(const int64_t delay_ms, const priority_type priority, Func&& func, Args&&... args) {
    static_assert(is_invocable_v<Func, Args...>, "Func must be invocable with Args");

    using Result = invoke_result_t<Func, Args...>;

	auto info = make_shared<task_info>(generate_task_id(), priority);

	auto task = _NEFORCE make_shared<packaged_task<Result()>>(
		[func = _NEFORCE forward<Func>(func),
		 tup = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...),
		 info]() mutable {
			struct context_guard {
				shared_ptr<task_info> info;

		 		explicit context_guard(shared_ptr<task_info> i) : info(move(i)) {
				 	info->status.store(task_info::status::running, memory_order_release);
				 	info->start_time = timestamp::now();
				 	info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;
		 		}

		 		~context_guard() noexcept {
				 	info->finish_time = timestamp::now();
				 	auto expected = task_info::status::running;
					info->status.compare_exchange_strong(expected,
						task_info::status::completed, memory_order_release);
		 		}
		 	};

			context_guard guard(info);

			try {
				return _NEFORCE apply(func, tup);
			} catch (const exception& e) {
				info->status.store(task_info::status::failed, memory_order_release);
				info->error = e.what();
				throw;
			}
		});

	future<Result> res = task->get_future();

	auto expire_time = steady_clock::now() + milliseconds(delay_ms);
	timer_.add_task(expire_time, [this, task = _NEFORCE move(task), priority]() mutable {
		this->submit_task(priority, [task]() {
			(*task)();
	    });
	});

	return submit_result<Result>{_NEFORCE move(res), info};
}

template <typename Func, typename... Args>
thread_pool::periodic_token thread_pool::submit_every(int64_t interval_ms, const priority_type priority, Func &&func, Args &&...args) {
    auto state = make_shared<periodic_task_state>();
	auto task = _NEFORCE make_shared<function<void()>>(
	    [func = _NEFORCE forward<Func>(func), tup = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...)]() mutable {
			_NEFORCE apply(func, tup);
	    }
	);
    auto handler_ptr = _NEFORCE make_shared<task_type>();
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

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__
