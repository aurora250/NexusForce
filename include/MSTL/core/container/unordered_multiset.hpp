#ifndef MSTL_CORE_CONTAINER_UNORDERED_MULTISET_HPP__
#define MSTL_CORE_CONTAINER_UNORDERED_MULTISET_HPP__
#include "hashtable.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Value, typename HashFcn = hash<Value>, typename EqualKey = equal_to<Value>,
    typename Alloc = allocator<hashtable_node<Value>>>
class unordered_multiset : icollector<unordered_multiset<Value, HashFcn, EqualKey, Alloc>> {
#ifdef MSTL_STANDARD_20__
    static_assert(is_hash_v<HashFcn, Value>, "unordered multiset requires valid hash function.");
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<hashtable_node<Value>, typename Alloc::value_type>,
        "allocator type mismatch.");
    static_assert(is_object_v<Value>, "unordered multiset only contains object types.");

    using self = unordered_multiset<Value, HashFcn, EqualKey, Alloc>;
    using super = icollector<self>;
    using base_type = hashtable<Value, Value, HashFcn, identity<Value>, EqualKey, Alloc>;

public:
    using key_type          = typename base_type::key_type;
    using value_type        = typename base_type::value_type;
    using hasher            = typename base_type::hasher;
    using key_equal         = typename base_type::key_equal;

    using size_type         = typename base_type::size_type;
    using difference_type   = typename base_type::difference_type;
    using pointer           = typename base_type::const_pointer;
    using const_pointer     = typename base_type::const_pointer;
    using reference         = typename base_type::const_reference;
    using const_reference   = typename base_type::const_reference;

    using iterator          = typename base_type::const_iterator;
    using const_iterator    = typename base_type::const_iterator;

    using allocator_type    = typename base_type::allocator_type;

private:
    base_type ht_{100};

public:
    unordered_multiset() : ht_(100, hasher(), key_equal()) {}
    explicit unordered_multiset(size_type n) : ht_(n) {}

    unordered_multiset(size_type n, const hasher& hf) : ht_(n, hf, key_equal()) {}
    unordered_multiset(size_type n, const hasher& hf, const key_equal& eql) : ht_(n, hf, eql) {}

    unordered_multiset(const self& ht) : ht_(ht.ht_) {}
    self& operator =(const self& x) = default;

    unordered_multiset(self&& x) noexcept(noexcept(ht_.swap(x.ht_))) : ht_(_MSTL forward<base_type>(x.ht_)) {}
    self& operator =(self&& x) noexcept(noexcept(ht_.swap(x.ht_))) {
        ht_ = _MSTL forward<self>(x.ht_);
        return *this;
    }

    template <typename Iterator>
    unordered_multiset(Iterator first, Iterator last) : ht_(100, hasher(), key_equal()) {
        ht_.insert_equal(first, last);
    }
    template <typename Iterator>
    unordered_multiset(Iterator first, Iterator list, size_type n) : ht_(n, hasher(), key_equal()) {
        ht_.insert_equal(first, list);
    }
    template <typename Iterator>
    unordered_multiset(Iterator first, Iterator last, size_type n, const hasher& hf) : ht_(n, hf, key_equal()) {
        ht_.insert_equal(first, last);
    }
    template <typename Iterator>
    unordered_multiset(Iterator first, Iterator last, size_type n, const hasher& hf, const key_equal& eql)
        : ht_(n, hf, eql) {
        ht_.insert_equal(first, last);
    }

    unordered_multiset(std::initializer_list<value_type> l)
        : unordered_multiset(l.begin(), l.end()) {}
    unordered_multiset(std::initializer_list<value_type> l, size_type n)
        : unordered_multiset(l.begin(), l.end(), n) {}
    unordered_multiset(std::initializer_list<value_type> l, size_type n, const hasher& hf)
        : unordered_multiset(l.begin(), l.end(), n, hf) {}
    unordered_multiset(std::initializer_list<value_type> l, size_type n, const hasher& hf, const key_equal& eql)
        : unordered_multiset(l.begin(), l.end(), n, hf, eql) {}

    MSTL_NODISCARD iterator begin() noexcept { return ht_.begin(); }
    MSTL_NODISCARD iterator end() noexcept { return ht_.end(); }
    MSTL_NODISCARD const_iterator begin() const noexcept { return ht_.begin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return ht_.end(); }
    MSTL_NODISCARD const_iterator cbegin() const noexcept { return ht_.cbegin(); }
    MSTL_NODISCARD const_iterator cend() const noexcept { return ht_.cend(); }

    MSTL_NODISCARD size_type size() const noexcept { return ht_.size(); }
    MSTL_NODISCARD size_type max_size() const noexcept { return ht_.max_size(); }
    MSTL_NODISCARD bool empty() const noexcept { return ht_.empty(); }

    MSTL_NODISCARD size_type count(const key_type& key) const noexcept(noexcept(ht_.count(key))) {
        return ht_.count(key);
    }
    MSTL_NODISCARD size_type bucket_count() const noexcept { return ht_.bucket_count(); }
    MSTL_NODISCARD size_type max_bucket_count() const noexcept { return ht_.max_bucket_count(); }
    MSTL_NODISCARD size_type bucket_size(size_type n) const noexcept { return ht_.bucket_size(n); }

    MSTL_NODISCARD allocator_type get_allocator() const noexcept { return allocator_type(); }

    MSTL_NODISCARD hasher hash_funct() const noexcept(noexcept(ht_.hash_func())) { return ht_.hash_func(); }
    MSTL_NODISCARD key_equal key_eq() const noexcept(noexcept(ht_.key_eql())) { return ht_.key_eql(); }

    MSTL_NODISCARD float load_factor() const noexcept { return ht_.load_factor(); }
    MSTL_NODISCARD float max_load_factor() const noexcept { return ht_.max_load_factor(); }
    void max_load_factor(float new_max) noexcept { ht_.max_load_factor(new_max); }

    void rehash(size_type new_size) { ht_.rehash(new_size); }
    void reserve(size_type max_count) { ht_.reserve(max_count); }

    template <typename... Args>
    iterator emplace(Args&&... args) {
        return ht_.emplace_equal(_MSTL forward<Args>(args)...);
    }

    iterator insert(const value_type& obj) {
        return ht_.insert_equal(obj);
    }
    iterator insert(value_type&& obj) {
        return ht_.insert_equal(_MSTL forward<value_type>(obj));
    }
    template <typename Iterator>
    void insert(Iterator first, Iterator last) { ht_.insert_equal(first, last); }

    size_type erase(const key_type& key) noexcept { return ht_.erase(key); }
    void erase(iterator it) noexcept { ht_.erase(it); }
    void erase(iterator first, iterator last) noexcept { ht_.erase(first, last); }
    void clear() noexcept { ht_.clear(); }

    void swap(self& x) noexcept(noexcept(ht_.swap(x.ht_))) { ht_.swap(x.ht_); }

    MSTL_NODISCARD iterator find(const key_type& key) { return ht_.find(key); }
    MSTL_NODISCARD const_iterator find(const key_type& key) const { return ht_.find(key); }

    MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) { return ht_.equal_range(key); }
    MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return ht_.equal_range(key);
    }

    MSTL_NODISCARD bool operator ==(const self& rh) const
    noexcept(noexcept(ht_ == rh.ht_)) {
        return ht_ == rh.ht_;
    }
    MSTL_NODISCARD bool operator <(const self& rh) const
    noexcept(noexcept(ht_ < rh.ht_)) {
        return ht_ < rh.ht_;
    }
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename HashFcn = hash<iter_value_t<Iterator>>, typename Compare
    = equal_to<iter_value_t<Iterator>>, typename Alloc = allocator<iter_value_t<Iterator>>>
unordered_multiset(Iterator, Iterator, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_multiset<iter_value_t<Iterator>, HashFcn, Compare, Alloc>;

template <typename Value, typename HashFcn = hash<Value>, typename Compare = equal_to<Value>,
    typename Alloc = allocator<Value>>
unordered_multiset(std::initializer_list<Value>, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_multiset<Value, HashFcn, Compare, Alloc>;

template <typename Iterator, typename Alloc>
unordered_multiset(Iterator, Iterator, Alloc)
-> unordered_multiset<iter_value_t<Iterator>, hash<iter_value_t<Iterator>>, equal_to<iter_value_t<Iterator>>, Alloc>;

template <typename Iterator, typename HashFcn, typename Alloc>
unordered_multiset(Iterator, Iterator, HashFcn, Alloc)
-> unordered_multiset<iter_value_t<Iterator>, HashFcn, equal_to<iter_value_t<Iterator>>, Alloc>;

template <typename Value, typename Alloc>
unordered_multiset(std::initializer_list<Value>, Alloc)
-> unordered_multiset<Value, hash<Value>, equal_to<Value>, Alloc>;

template <typename Value, typename HashFcn, typename Alloc>
unordered_multiset(std::initializer_list<Value>, HashFcn, Alloc)
-> unordered_multiset<Value, HashFcn, equal_to<Value>, Alloc>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_UNORDERED_MULTISET_HPP__
