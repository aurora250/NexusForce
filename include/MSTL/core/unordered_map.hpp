#ifndef MSTL_HASH_MAP_HPP__
#define MSTL_HASH_MAP_HPP__
#include "hashtable.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Key, typename T, typename HashFcn = hash<Key>, typename EqualKey = equal_to<Key>,
    typename Alloc = allocator<hashtable_node<pair<const Key, T>>>>
class unordered_map {
#ifdef MSTL_VERSION_20__
    static_assert(is_hash_v<HashFcn, Key>, "unordered map requires valid hash function.");
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<hashtable_node<pair<const Key, T>>, typename Alloc::value_type>,
        "allocator type mismatch.");
    static_assert(is_object_v<Key>, "unordered map only contains object types.");

private:
    using base_type = hashtable<pair<const Key, T>, Key, HashFcn, select1st<pair<const Key, T>>, EqualKey, Alloc>;
public:
    using key_type          = typename base_type::key_type;
    using value_type        = typename base_type::value_type;
    using hasher            = typename base_type::hasher;
    using key_equal         = typename base_type::key_equal;
    using size_type         = typename base_type::size_type;
    using difference_type   = typename base_type::difference_type;
    using pointer           = typename base_type::pointer;
    using const_pointer     = typename base_type::const_pointer;
    using reference         = typename base_type::reference;
    using const_reference   = typename base_type::const_reference;

    using iterator          = typename base_type::iterator;
    using const_iterator    = typename base_type::const_iterator;

    using allocator_type    = typename base_type::allocator_type;
    using self              = unordered_map<Key, T, HashFcn, EqualKey, Alloc>;

private:
    base_type ht_{101, hasher(), key_equal()};

    template <typename Key1, typename T1, typename HashFcn1, typename EqualKey1, typename Alloc1>
    friend bool operator ==(const unordered_map<Key1, T1, HashFcn1, EqualKey1, Alloc1>&,
        const unordered_map<Key1, T1, HashFcn1, EqualKey1, Alloc1>&) noexcept;

public:
    unordered_map() = default;
    explicit unordered_map(size_type n) : ht_(n) {}

    unordered_map(size_type n, const hasher& hf) : ht_(n, hf, key_equal()) {}
    unordered_map(size_type n, const hasher& hf, const key_equal& eql) : ht_(n, hf, eql) {}

    unordered_map(const self& ht) : ht_(ht.ht_) {}
    self& operator =(const self& x) = default;

    unordered_map(self&& x) noexcept(noexcept(ht_.swap(x.ht_)))
    : ht_(_MSTL forward<base_type>(x.ht_)) {}

    self& operator =(self&& x) noexcept(noexcept(ht_.swap(x.ht_))) {
        ht_ = _MSTL move(x.ht_);
        return *this;
    }

    template <typename Iterator>
    unordered_map(Iterator first, Iterator last) : ht_(100, hasher(), key_equal()) {
        ht_.insert_unique(first, last);
    }
    template <typename Iterator>
    unordered_map(Iterator first, Iterator list, size_type n) : ht_(n, hasher(), key_equal()) {
        ht_.insert_unique(first, list);
    }
    template <typename Iterator>
    unordered_map(Iterator first, Iterator last, size_type n, const hasher& hf) : ht_(n, hf, key_equal()) {
        ht_.insert_unique(first, last);
    }
    template <typename Iterator>
    unordered_map(Iterator first, Iterator last, size_type n, const hasher& hf, const key_equal& eql) 
        : ht_(n, hf, eql) {
        ht_.insert_unique(first, last);
    }

    unordered_map(std::initializer_list<value_type> l) 
        : unordered_map(l.begin(), l.end()) {}
    unordered_map(std::initializer_list<value_type> l, size_type n) 
        : unordered_map(l.begin(), l.end(), n) {}
    unordered_map(std::initializer_list<value_type> l, size_type n, const hasher& hf)
        : unordered_map(l.begin(), l.end(), n, hf) {}
    unordered_map(std::initializer_list<value_type> l, size_type n, const hasher& hf, const key_equal& eql)
        : unordered_map(l.begin(), l.end(), n, hf, eql) {}

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
    pair<iterator, bool> emplace(Args&&... args) {
        return ht_.emplace_unique(_MSTL forward<Args>(args)...);
    }

    pair<iterator, bool> insert(const value_type& obj) {
        return ht_.insert_unique(obj);
    }
    pair<iterator, bool> insert(value_type&& obj) { 
        return ht_.insert_unique(_MSTL forward<value_type>(obj));
    }
    template <typename Iterator>
    void insert(Iterator first, Iterator last) { ht_.insert_unique(first, last); }

    size_type erase(const key_type& key) noexcept { return ht_.erase(key); }
    iterator erase(iterator it) noexcept { return ht_.erase(it); }
    iterator erase(iterator first, iterator last) noexcept { return ht_.erase(first, last); }
    const_iterator erase(const_iterator it) noexcept { return ht_.erase(it); }
    const_iterator erase(const_iterator first, const_iterator last) noexcept { return ht_.erase(first, last); }
    void clear() noexcept { ht_.clear(); }

    void swap(self& x) noexcept(noexcept(ht_.swap(x.ht_))) { ht_.swap(x.ht_); }

    MSTL_NODISCARD iterator find(const key_type& key) { return ht_.find(key); }
    MSTL_NODISCARD const_iterator find(const key_type& key) const { return ht_.find(key); }

    MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) { return ht_.equal_range(key); }
    MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return ht_.equal_range(key);
    }

    MSTL_NODISCARD T& operator[](const key_type& key) {
        auto iter = ht_.find(key);
        if (iter == ht_.end()) 
            iter = ht_.emplace_unique(key, T()).first;
        return iter->second;
    }
    MSTL_NODISCARD const T& at(const key_type& key) const {
        auto iter = ht_.find(key);
        Exception(iter != cend(), StopIterator());
        return iter->second;
    }
    MSTL_NODISCARD T& at(const key_type& key) {
        return const_cast<reference>(static_cast<const self*>(this)->at(key));
    }
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename HashFcn = hash<get_iter_key_t<Iterator>>,
    typename Compare = equal_to<get_iter_key_t<Iterator>>, typename Alloc>
unordered_map(Iterator, Iterator, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_map<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>, HashFcn, Compare, Alloc>;

template <typename Key, typename T, typename HashFcn = hash<Key>, typename Compare = equal_to<Key>,
    typename Alloc = allocator<pair<const Key, T>>>
unordered_map(std::initializer_list<pair<Key, T>>, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_map<Key, T, HashFcn, Compare, Alloc>;

template <typename Iterator, typename Alloc>
unordered_map(Iterator, Iterator, Alloc) -> unordered_map<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>,
    hash<get_iter_key_t<Iterator>>, equal_to<get_iter_key_t<Iterator>>, Alloc>;

template <typename Iterator, typename HashFcn, typename Alloc>
unordered_map(Iterator, Iterator, HashFcn, Alloc) -> unordered_map<get_iter_key_t<Iterator>,
    get_iter_val_t<Iterator>, HashFcn, equal_to<get_iter_key_t<Iterator>>, Alloc>;

template <typename Key, typename T, typename Alloc>
unordered_map(std::initializer_list<pair<Key, T>>, Alloc)
-> unordered_map<Key, T, hash<Key>, equal_to<Key>, Alloc>;

template <typename Key, typename T, typename HashFcn, typename Alloc>
unordered_map(std::initializer_list<pair<Key, T>>, HashFcn, Alloc)
-> unordered_map<Key, T, HashFcn, equal_to<Key>, Alloc>;
#endif

template <typename Key, typename T, typename HashFcn, typename EqualKey, typename Alloc>
MSTL_NODISCARD bool operator ==(const unordered_map<Key, T, HashFcn, EqualKey, Alloc>& lh,
    const unordered_map<Key, T, HashFcn, EqualKey, Alloc>& rh) noexcept {
    return lh.ht_ == rh.ht_;
}
template <typename Key, typename T, typename HashFcn, typename EqualKey, typename Alloc>
MSTL_NODISCARD bool operator !=(const unordered_map<Key, T, HashFcn, EqualKey, Alloc>& lh,
    const unordered_map<Key, T, HashFcn, EqualKey, Alloc>& rh) noexcept {
    return !(lh.ht_ == rh.ht_);
}
template <typename Key, typename T, typename HashFcn, typename EqualKey, typename Alloc>
void swap(const unordered_map<Key, T, HashFcn, EqualKey, Alloc>& lh,
    const unordered_map<Key, T, HashFcn, EqualKey, Alloc>& rh) noexcept(noexcept(lh.swap(rh))) {
    lh.swap(rh);
}


template <typename Key, typename T, typename HashFcn = hash<Key>, typename EqualKey = equal_to<Key>,
    typename Alloc = allocator<hashtable_node<pair<const Key, T>>>>
class unordered_multimap {
#ifdef MSTL_VERSION_20__
    static_assert(is_hash_v<HashFcn, Key>, "unordered multimap requires valid hash function.");
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<hashtable_node<pair<const Key, T>>, typename Alloc::value_type>,
        "allocator type mismatch.");
    static_assert(is_object_v<Key>, "unordered multimap only contains object types.");

private:
    using base_type = hashtable<pair<const Key, T>, Key, HashFcn, select1st<pair<const Key, T>>, EqualKey, Alloc>;
public:
    using key_type          = typename base_type::key_type;
    using value_type        = typename base_type::value_type;
    using hasher            = typename base_type::hasher;
    using key_equal         = typename base_type::key_equal;
    using size_type         = typename base_type::size_type;
    using difference_type   = typename base_type::difference_type;
    using pointer           = typename base_type::pointer;
    using const_pointer     = typename base_type::const_pointer;
    using reference         = typename base_type::reference;
    using const_reference   = typename base_type::const_reference;
    using iterator          = typename base_type::iterator;
    using const_iterator    = typename base_type::const_iterator;
    using allocator_type    = typename base_type::allocator_type;
    using self              = unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>;

private:
    base_type ht_{100, hasher(), key_equal()};

    template <typename Key1, typename T1, typename HashFcn1, typename EqualKey1, typename Alloc1>
    friend bool operator ==(const unordered_multimap<Key1, T1, HashFcn1, EqualKey1, Alloc1>&,
        const unordered_multimap<Key1, T1, HashFcn1, EqualKey1, Alloc1>&) noexcept;


public:
    unordered_multimap() = default;
    explicit unordered_multimap(size_type n) : ht_(n) {}

    unordered_multimap(size_type n, const hasher& hf) : ht_(n, hf, key_equal()) {}
    unordered_multimap(size_type n, const hasher& hf, const key_equal& eql) : ht_(n, hf, eql) {}

    unordered_multimap(const self& ht) : ht_(ht.ht_) {}
    self& operator =(const self& x) = default;

    unordered_multimap(self&& x) noexcept(noexcept(ht_.swap(x.ht_))) : ht_(_MSTL forward<base_type>(x.ht_)) {}
    self& operator =(self&& x) noexcept(noexcept(ht_.swap(x.ht_))) {
        ht_ = _MSTL forward<self>(x.ht_);
        return *this;
    }

    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator last) : ht_(100, hasher(), key_equal()) {
        ht_.insert_equal(first, last);
    }
    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator list, size_type n) : ht_(n, hasher(), key_equal()) {
        ht_.insert_equal(first, list);
    }
    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator last, size_type n, const hasher& hf) : ht_(n, hf, key_equal()) {
        ht_.insert_equal(first, last);
    }
    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator last, size_type n, const hasher& hf, const key_equal& eql)
        : ht_(n, hf, eql) {
        ht_.insert_equal(first, last);
    }

    unordered_multimap(std::initializer_list<value_type> l)
        : unordered_multimap(l.begin(), l.end()) {}
    unordered_multimap(std::initializer_list<value_type> l, size_type n)
        : unordered_multimap(l.begin(), l.end(), n) {}
    unordered_multimap(std::initializer_list<value_type> l, size_type n, const hasher& hf)
        : unordered_multimap(l.begin(), l.end(), n, hf) {}
    unordered_multimap(std::initializer_list<value_type> l, size_type n, const hasher& hf, const key_equal& eql)
        : unordered_multimap(l.begin(), l.end(), n, hf, eql) {}

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
        return ht_.insert_equal(_MSTL move(obj));
    }
    template <typename Iterator>
    void insert(Iterator first, Iterator last) { ht_.insert_equal(first, last); }

    size_type erase(const key_type& key) noexcept { return ht_.erase(key); }
    iterator erase(iterator it) noexcept { return ht_.erase(it); }
    iterator erase(iterator first, iterator last) noexcept { return ht_.erase(first, last); }
    const_iterator erase(const_iterator it) noexcept { return ht_.erase(it); }
    const_iterator erase(const_iterator first, const_iterator last) noexcept { return ht_.erase(first, last); }
    void clear() noexcept { ht_.clear(); }

    void swap(self& x) noexcept(noexcept(ht_.swap(x.ht_))) { ht_.swap(x.ht_); }

    MSTL_NODISCARD iterator find(const key_type& key) { return ht_.find(key); }
    MSTL_NODISCARD const_iterator find(const key_type& key) const { return ht_.find(key); }

    MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) { return ht_.equal_range(key); }
    MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return ht_.equal_range(key);
    }
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename HashFcn = hash<get_iter_key_t<Iterator>>,
    typename Compare = equal_to<get_iter_key_t<Iterator>>, typename Alloc>
unordered_multimap(Iterator, Iterator, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_multimap<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>, HashFcn, Compare, Alloc>;

template <typename Key, typename T, typename HashFcn = hash<Key>, typename Compare = equal_to<Key>,
    typename Alloc = allocator<pair<const Key, T>>>
unordered_multimap(std::initializer_list<pair<Key, T>>, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_multimap<Key, T, HashFcn, Compare, Alloc>;

template <typename Iterator, typename Alloc>
unordered_multimap(Iterator, Iterator, Alloc) -> unordered_multimap<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>,
    hash<get_iter_key_t<Iterator>>, equal_to<get_iter_key_t<Iterator>>, Alloc>;

template <typename Iterator, typename HashFcn, typename Alloc>
unordered_multimap(Iterator, Iterator, HashFcn, Alloc) -> unordered_multimap<get_iter_key_t<Iterator>,
    get_iter_val_t<Iterator>, HashFcn, equal_to<get_iter_key_t<Iterator>>, Alloc>;

template <typename Key, typename T, typename Alloc>
unordered_multimap(std::initializer_list<pair<Key, T>>, Alloc)
-> unordered_multimap<Key, T, hash<Key>, equal_to<Key>, Alloc>;

template <typename Key, typename T, typename HashFcn, typename Alloc>
unordered_multimap(std::initializer_list<pair<Key, T>>, HashFcn, Alloc)
-> unordered_multimap<Key, T, HashFcn, equal_to<Key>, Alloc>;
#endif

template <typename Key, typename T, typename HashFcn, typename EqualKey, typename Alloc>
MSTL_NODISCARD bool operator ==(const unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>& lh,
    const unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>& rh) noexcept {
    return lh.ht_ == rh.ht_;
}
template <typename Key, typename T, typename HashFcn, typename EqualKey, typename Alloc>
MSTL_NODISCARD bool operator !=(const unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>& lh,
    const unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>& rh) noexcept {
    return !(lh.ht_ == rh.ht_);
}
template <typename Key, typename T, typename HashFcn, typename EqualKey, typename Alloc>
void swap(const unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>& lh,
    const unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>& rh) noexcept(noexcept(lh.swap(rh))) {
    lh.swap(rh);
}

MSTL_END_NAMESPACE__
#endif // MSTL_HASH_MAP_HPP__
