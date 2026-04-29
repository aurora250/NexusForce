#ifndef NEFORCE_CORE_ASYNC_HAZARD_PTR_HPP__
#define NEFORCE_CORE_ASYNC_HAZARD_PTR_HPP__

/**
 * @file hazard_ptr.hpp
 * @brief 无锁编程中的险象指针实现
 *
 * 此文件提供了险象指针的实现，用于无锁数据结构中的内存回收。
 * 险象指针可以安全地跟踪正在被线程访问的对象，防止对象在被访问时被回收。
 */

#include "NeForce/core/algorithm/sort.hpp"
#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/utility/deleter.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup HazardPointer 险象指针
 * @brief 无锁编程中的内存回收机制
 * @{
 */

class hazard_pointer_domain;
class hazard_pointer_obj_base;

/**
 * @struct hazard_pointer_record
 * @brief 险象指针记录
 *
 * 每个线程可以持有一个险象指针记录，用于标记当前正在访问的对象。
 * 记录包含一个原子指针和一个活跃标志。
 */
struct hazard_pointer_record {
    atomic<void*> hazard_ptr{nullptr};            ///< 受保护的指针
    atomic<hazard_pointer_record*> next{nullptr}; ///< 链表中的下一个记录
    atomic<bool> active{false};                   ///< 记录是否活跃

    hazard_pointer_record() = default;

    /**
     * @brief 尝试获取记录的所有权
     * @return 是否成功获取
     */
    bool try_acquire() {
        bool expected = false;
        return active.compare_exchange_strong(expected, true, memory_order_acquire, memory_order_relaxed);
    }

    /**
     * @brief 释放记录
     */
    void release() {
        hazard_ptr.store(nullptr, memory_order_release);
        active.store(false, memory_order_release);
    }

    /**
     * @brief 保护指定的指针
     * @param ptr 要保护的指针
     */
    void protect(void* ptr) { hazard_ptr.store(ptr, memory_order_release); }

    /**
     * @brief 获取当前保护的指针
     * @return 受保护的指针
     */
    void* get_protected() const { return hazard_ptr.load(memory_order_acquire); }
};

/**
 * @class hazard_pointer_obj_base
 * @brief 险象指针对象基类
 *
 * 所有需要通过险象指针回收的对象都需要继承此类。
 * 提供多态销毁接口。
 */
class hazard_pointer_obj_base {
public:
    hazard_pointer_obj_base* next{nullptr}; ///< 链表中的下一个对象

    virtual ~hazard_pointer_obj_base() = default;
    virtual void destroy() = 0; ///< 销毁对象
};

/**
 * @class hazard_pointer_obj
 * @brief 险象指针对象模板
 * @tparam T 对象类型
 * @tparam Deleter 删除器类型
 *
 * 包装需要回收的对象，包含自定义删除器。
 */
template <typename T, typename Deleter = default_deleter<void>>
class hazard_pointer_obj final : public hazard_pointer_obj_base {
    T* ptr;          ///< 实际对象指针
    Deleter deleter; ///< 删除器

public:
    /**
     * @brief 构造函数
     * @param p 要管理的指针
     * @param d 删除器
     */
    explicit hazard_pointer_obj(T* p, Deleter d = Deleter()) :
    ptr(p),
    deleter(_NEFORCE move(d)) {}

    /**
     * @brief 销毁对象
     */
    void destroy() override { deleter(ptr); }
};

/**
 * @struct retire_list
 * @brief 线程本地退役列表
 *
 * 每个线程维护一个退役对象列表，当列表大小达到阈值时触发扫描回收。
 */
struct retire_list {
    hazard_pointer_obj_base* head{nullptr}; ///< 链表头
    size_t count{0};                        ///< 列表大小

    /**
     * @brief 添加对象到退役列表
     * @param obj 要添加的对象
     */
    void add(hazard_pointer_obj_base* obj) {
        obj->next = head;
        head = obj;
        ++count;
    }

    /**
     * @brief 清空并销毁所有对象
     */
    void clear() {
        while (head) {
            auto* next = head->next;
            head->destroy();
            delete head;
            head = next;
        }
        count = 0;
    }

    /**
     * @brief 析构函数
     */
    ~retire_list() { clear(); }
};

/**
 * @class hazard_pointer_domain
 * @brief 险象指针域
 *
 * 管理所有险象指针记录和退役对象回收。每个域独立管理一组记录和回收策略。
 */
class hazard_pointer_domain {
private:
    atomic<hazard_pointer_record*> head_{nullptr}; ///< 记录链表头

    static thread_local retire_list tl_retire_list_; ///< 线程本地退役列表

    static constexpr size_t RETIRE_THRESHOLD = 100; ///< 回收阈值

    /**
     * @brief 获取所有活跃的险象指针
     * @return 所有受保护的指针向量
     */
    vector<void*> get_hazard_pointers() {
        vector<void*> hazards;
        hazard_pointer_record* current = head_.load(memory_order_acquire);

        while (current) {
            if (current->active.load(memory_order_acquire)) {
                void* ptr = current->get_protected();
                if (ptr) {
                    hazards.push_back(ptr);
                }
            }
            current = current->next.load(memory_order_acquire);
        }

        return hazards;
    }

    /**
     * @brief 扫描并回收可安全删除的对象
     *
     * 收集所有险象指针，将当前没有被任何险象指针保护的对象销毁。
     * 使用二分查找确定对象是否受保护。
     */
    void scan_and_reclaim() {
        auto hazards = get_hazard_pointers();
        _NEFORCE sort(hazards.begin(), hazards.end());

        retire_list new_list;
        hazard_pointer_obj_base* current = tl_retire_list_.head;

        while (current) {
            auto* next = current->next;

            if (_NEFORCE binary_search(hazards.begin(), hazards.end(), current)) {
                new_list.add(current);
            } else {
                current->destroy();
                delete current;
            }

            current = next;
        }

        tl_retire_list_ = _NEFORCE move(new_list);
    }

public:
    hazard_pointer_domain() = default;

    /**
     * @brief 析构函数
     *
     * 释放所有险象指针记录。
     */
    ~hazard_pointer_domain() {
        hazard_pointer_record* current = head_.load();
        while (current) {
            auto* next = current->next.load();
            delete current;
            current = next;
        }
    }

    hazard_pointer_domain(const hazard_pointer_domain&) = delete;
    hazard_pointer_domain& operator=(const hazard_pointer_domain&) = delete;

    /**
     * @brief 获取一个可用的险象指针记录
     * @return 可用的记录指针
     *
     * 首先尝试重用已有的空闲记录，如果没有则创建新记录。
     */
    hazard_pointer_record* acquire_record() {
        hazard_pointer_record* current = head_.load(memory_order_acquire);
        while (current) {
            if (current->try_acquire()) {
                return current;
            }
            current = current->next.load(memory_order_acquire);
        }

        hazard_pointer_record* new_record = new hazard_pointer_record();
        new_record->active.store(true, memory_order_relaxed);

        hazard_pointer_record* old_head = head_.load(memory_order_relaxed);
        do {
            new_record->next.store(old_head, memory_order_relaxed);
        } while (!head_.compare_exchange_weak(old_head, new_record, memory_order_release, memory_order_relaxed));

        return new_record;
    }

    /**
     * @brief 退役一个对象
     * @tparam T 对象类型
     * @tparam Deleter 删除器类型
     * @param ptr 要退役的指针
     * @param deleter 自定义删除器
     *
     * 将对象添加到当前线程的退役列表，如果列表大小超过阈值则触发回收。
     */
    template <typename T, typename Deleter = default_deleter<T>>
    void retire(T* ptr, Deleter deleter = Deleter()) {
        if (!ptr) {
            return;
        }

        auto* obj = new hazard_pointer_obj<T, Deleter>(ptr, _NEFORCE move(deleter));
        tl_retire_list_.add(obj);

        if (tl_retire_list_.count >= RETIRE_THRESHOLD) {
            scan_and_reclaim();
        }
    }

    /**
     * @brief 手动触发回收
     */
    void reclaim() { scan_and_reclaim(); }

    /**
     * @brief 获取默认的险象指针域
     * @return 默认域的引用
     */
    static hazard_pointer_domain& default_domain() {
        static hazard_pointer_domain domain{};
        return domain;
    }
};


/**
 * @class hazard_pointer
 * @brief 险象指针句柄
 *
 * 险象指针管理类，自动获取和释放记录。
 */
class hazard_pointer {
private:
    hazard_pointer_record* record_{nullptr}; ///< 持有的记录
    hazard_pointer_domain* domain_{nullptr}; ///< 所属域

public:
    hazard_pointer() = default;

    /**
     * @brief 构造函数
     * @param domain 险象指针域
     *
     * 从指定域获取一个险象指针记录。
     */
    explicit hazard_pointer(hazard_pointer_domain& domain) :
    domain_(&domain) {
        record_ = domain_->acquire_record();
    }

    /**
     * @brief 析构函数
     *
     * 释放持有的记录。
     */
    ~hazard_pointer() {
        reset_protection();
        if (record_) {
            record_->release();
        }
    }

    hazard_pointer(hazard_pointer&& other) noexcept :
    record_(other.record_),
    domain_(other.domain_) {
        other.record_ = nullptr;
        other.domain_ = nullptr;
    }

    hazard_pointer& operator=(hazard_pointer&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }

        reset_protection();
        if (record_) {
            record_->release();
        }
        record_ = other.record_;
        domain_ = other.domain_;
        other.record_ = nullptr;
        other.domain_ = nullptr;

        return *this;
    }

    hazard_pointer(const hazard_pointer&) = delete;
    hazard_pointer& operator=(const hazard_pointer&) = delete;

    /**
     * @brief 保护一个原子指针
     * @tparam T 指针类型
     * @param src 原子指针源
     * @return 受保护的指针
     *
     * 使用ABA预防算法：读取指针 -> 保护 -> 验证未改变。
     */
    template <typename T>
    T* protect(const atomic<T*>& src) {
        if (!record_) {
            return nullptr;
        }

        T* ptr = src.load(memory_order_relaxed);
        while (true) {
            record_->protect(ptr);
            T* ptr2 = src.load(memory_order_acquire);
            if (ptr == ptr2) {
                return ptr;
            }
            ptr = ptr2;
        }
    }

    /**
     * @brief 尝试保护一个原子指针
     * @tparam T 指针类型
     * @param ptr 输出参数，受保护的指针
     * @param src 原子指针源
     * @return 是否成功保护
     */
    template <typename T>
    bool try_protect(T*& ptr, const atomic<T*>& src) {
        if (!record_) {
            return false;
        }

        ptr = src.load(memory_order_relaxed);
        record_->protect(ptr);
        T* ptr2 = src.load(memory_order_acquire);

        if (ptr == ptr2) {
            return true;
        }

        ptr = ptr2;
        return false;
    }

    /**
     * @brief 重置保护
     *
     * 清除当前保护的指针。
     */
    void reset_protection() noexcept {
        if (record_) {
            record_->protect(nullptr);
        }
    }

    /**
     * @brief 交换两个险象指针
     * @param other 要交换的对象
     */
    void swap(hazard_pointer& other) noexcept {
        _NEFORCE swap(record_, other.record_);
        _NEFORCE swap(domain_, other.domain_);
    }

    /**
     * @brief 检查是否持有有效记录
     * @return 是否有效
     */
    explicit operator bool() const noexcept { return record_ != nullptr; }
};

/**
 * @brief 创建险象指针的辅助函数
 * @param domain 险象指针域
 * @return 新创建的险象指针
 */
inline hazard_pointer make_hazard_pointer(hazard_pointer_domain& domain = hazard_pointer_domain::default_domain()) {
    return hazard_pointer(domain);
}


/**
 * @class hazard_pointer_holder
 * @brief 持有特定类型指针的险象指针包装
 * @tparam T 指针类型
 *
 * 将险象指针与一个特定类型的指针绑定，提供类型安全的访问。
 */
template <typename T>
class hazard_pointer_holder {
private:
    hazard_pointer hp_; ///< 险象指针
    T* ptr_{nullptr};   ///< 受保护的指针

public:
    hazard_pointer_holder() = default;

    /**
     * @brief 构造函数
     * @param domain 险象指针域
     */
    explicit hazard_pointer_holder(hazard_pointer_domain& domain) :
    hp_(domain) {}

    /**
     * @brief 保护原子指针
     * @param src 原子指针源
     * @return 受保护的指针
     */
    T* protect(const atomic<T*>& src) {
        ptr_ = hp_.protect(src);
        return ptr_;
    }

    /**
     * @brief 获取当前保护的指针
     * @return 指针
     */
    T* get() const noexcept { return ptr_; }

    /**
     * @brief 解引用操作符
     * @return 引用
     */
    T& operator*() const noexcept { return *ptr_; }

    /**
     * @brief 箭头操作符
     * @return 指针
     */
    T* operator->() const noexcept { return ptr_; }

    /**
     * @brief 检查是否持有有效指针
     * @return 是否有效
     */
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /**
     * @brief 重置持有的指针
     */
    void reset() noexcept {
        hp_.reset_protection();
        ptr_ = nullptr;
    }
};

/** @} */ // HazardPointer

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_HAZARD_PTR_HPP__
