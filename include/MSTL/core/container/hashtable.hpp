#ifndef MSTL_CORE_CONTAINER_HASHTABLE_HPP__
#define MSTL_CORE_CONTAINER_HASHTABLE_HPP__
#include "../algorithm/sort.hpp"
#include "vector.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Value, typename Key, typename HashFcn, 
    typename ExtractKey, typename EqualKey, typename Alloc>
class hashtable;
template <bool IsConst, typename HashTable>
struct hashtable_iterator;

template <typename T>
struct hashtable_node {
private:
    hashtable_node* next_ = nullptr;
    T data_{};

    template <typename, typename, typename, typename, typename, typename> friend class hashtable;
    template <bool, typename> friend struct hashtable_iterator;

public:
    hashtable_node() = default;
};

template <bool IsConst, typename HashTable>
struct hashtable_iterator {
private:
    using container_type    = HashTable;
    using iterator          = hashtable_iterator<false, container_type>;
    using const_iterator    = hashtable_iterator<true, container_type>;

public:
    using iterator_category = forward_iterator_tag;
    using value_type        = typename container_type::value_type;
    using reference         = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer           = conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;
    using difference_type   = typename container_type::difference_type;
    using size_type         = typename container_type::size_type;

private:
    using node_type         = hashtable_node<value_type>;

    node_type* cur_ = nullptr;
    const container_type* ht_ = nullptr;
    size_type bucket_ = 0;

    template <typename, typename, typename, typename, typename, typename> friend class hashtable;
    template <bool, typename> friend struct hashtable_iterator;

public:
    hashtable_iterator() noexcept = default;

    hashtable_iterator(node_type* n, const HashTable* ht, const size_type bucket)
    : cur_(n), ht_(ht), bucket_(bucket) {}

    hashtable_iterator(const iterator& it)
    : cur_(it.cur_), ht_(it.ht_), bucket_(it.bucket_) {}

    hashtable_iterator& operator =(const iterator& it) {
        if(_MSTL addressof(it) == this) return *this;
        cur_ = it.cur_;
        ht_ = it.ht_;
        bucket_ = it.bucket_;
        return *this;
    }

    hashtable_iterator(iterator&& it) noexcept
    : cur_(it.cur_), ht_(it.ht_), bucket_(it.bucket_) {
        it.cur_ = nullptr;
        it.ht_ = nullptr;
        it.bucket_ = -1;
    }

    hashtable_iterator& operator =(iterator&& it) noexcept {
        if(_MSTL addressof(it) == this) return *this;
        cur_ = it.cur_;
        ht_ = it.ht_;
        bucket_ = it.bucket_;
        it.cur_ = nullptr;
        it.ht_ = nullptr;
        it.bucket_ = -1;
        return *this;
    }

    hashtable_iterator(const const_iterator& it)
    : cur_(it.cur_), ht_(it.ht_), bucket_(it.bucket_) {}

    hashtable_iterator& operator =(const const_iterator& it) {
        if(_MSTL addressof(it) == this) return *this;
        cur_ = it.cur_;
        ht_ = it.ht_;
        bucket_ = it.bucket_;
        return *this;
    }

    hashtable_iterator(const_iterator&& it)
    : cur_(it.cur_), ht_(it.ht_), bucket_(it.bucket_) {
        it.cur_ = nullptr;
        it.ht_ = nullptr;
        it.bucket_ = -1;
    }

    hashtable_iterator& operator =(const_iterator&& it) {
        if(_MSTL addressof(it) == this) return *this;
        cur_ = it.cur_;
        ht_ = it.ht_;
        bucket_ = it.bucket_;
        it.cur_ = nullptr;
        it.ht_ = nullptr;
        it.bucket_ = -1;
        return *this;
    }

    ~hashtable_iterator() = default;

    MSTL_NODISCARD reference operator *() const noexcept {
        MSTL_DEBUG_VERIFY(cur_ && ht_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(hashtable_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        MSTL_DEBUG_VERIFY(bucket_ < ht_->buckets_.size() && 0 <= bucket_,
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(hashtable_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return cur_->data_;
    }
    MSTL_NODISCARD pointer operator ->() const noexcept {
        MSTL_DEBUG_VERIFY(cur_ && ht_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(hashtable_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        MSTL_DEBUG_VERIFY(bucket_ < ht_->buckets_.size() && 0 <= bucket_,
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(hashtable_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return &operator*();
    }

    hashtable_iterator& operator ++() {
        MSTL_DEBUG_VERIFY(cur_ && ht_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(hashtable_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        MSTL_DEBUG_VERIFY(bucket_ < ht_->buckets_.size() && !(bucket_ + 1 == ht_->buckets_.size() && cur_->next_ != nullptr),
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(hashtable_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        cur_ = cur_->next_;
        if (cur_ == nullptr) {
            while (cur_ == nullptr && ++bucket_ < ht_->buckets_.size()) {
                cur_ = ht_->buckets_[bucket_];
            }
        }
        return *this;
    }
    hashtable_iterator operator ++(int) {
        iterator tmp = *this;
        ++*this;
        return tmp;
    }

    MSTL_NODISCARD bool operator ==(const hashtable_iterator& x) const noexcept {
		MSTL_DEBUG_VERIFY(ht_ == x.ht_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(hashtable_iterator));
        return cur_ == x.cur_;
    }
    MSTL_NODISCARD bool operator !=(const hashtable_iterator& x) const noexcept {
        return !(*this == x);
    }

    MSTL_NODISCARD pointer base() const noexcept {
        return cur_;
    }
};


MSTL_BEGIN_CONSTANTS__
#ifdef MSTL_DATA_BUS_WIDTH_64__
MSTL_INLINE17 constexpr size_t HASH_PRIME_LIST[] = {
    101,                    173,                        263,                        397,
    599,                    907,                        1361,                       2053,
    3083,                   4637,                       6959,                       10453,
    15683,                  23531,                      35311,                      52967,
    79451,                  119179,                     178781,                     268189,
    402299,                 603457,                     905189,                     1357787,
    2036687,                3055043,                    4582577,                    6873871,
    10310819,               15466229,                   23199347,                   34799021,
    52198537,               78297827,                   117446801,                  176170229,
    264255353,              396383041,                  594574583,                  891861923,
    1337792887,             2006689337,                 3010034021u,                4515051137ull,
    6772576709ull,          10158865069ull,             15238297621ull,             22857446471ull,
    34286169707ull,         51429254599ull,             77143881917ull,             115715822899ull,
    173573734363ull,        260360601547ull,            390540902329ull,            585811353559ull,
    878717030339ull,        1318075545511ull,           1977113318311ull,           2965669977497ull,
    4448504966249ull,       6672757449409ull,           10009136174239ull,          15013704261371ull,
    22520556392057ull,      33780834588157ull,          50671251882247ull,          76006877823377ull,
    114010316735089ull,     171015475102649ull,         256523212653977ull,         384784818980971ull,
    577177228471507ull,     865765842707309ull,         1298648764060979ull,        1947973146091477ull,
    2921959719137273ull,    4382939578705967ull,        6574409368058969ull,        9861614052088471ull,
    14792421078132871ull,   22188631617199337ull,       33282947425799017ull,       49924421138698549ull,
    74886631708047827ull,   112329947562071807ull,      168494921343107851ull,      252742382014661767ull,
    379113573021992729ull,  568670359532989111ull,      853005539299483657ull,      1279508308949225477ull,
    1919262463423838231ull, 2878893695135757317ull,     4318340542703636011ull,     6477510814055453699ull
};
#else
MSTL_INLINE17 constexpr size_t HASH_PRIME_LIST[] = {
    53,         97,           193,         389,       769,
    1543,       3079,         6151,        12289,     24593,
    49157,      98317,        196613,      393241,    786433,
    1572869,    3145739,      6291469,     12582917,  25165843,
    50331653,   100663319,    201326611,   402653189, 805306457,
    1610612741
};
#endif

MSTL_INLINE17 constexpr size_t HASH_PRIMER_COUNT = extent_v<decltype(HASH_PRIME_LIST)>;

MSTL_END_CONSTANTS__


MSTL_NODISCARD MSTL_CONSTEXPR20 size_t hashtable_next_prime(const size_t n) {
    const size_t* first = _CONSTANTS HASH_PRIME_LIST;
    const size_t* last = _CONSTANTS HASH_PRIME_LIST + _CONSTANTS HASH_PRIMER_COUNT;
    const size_t* pos = _MSTL lower_bound(first, last, n);
    return pos == last ? *(last - 1) : *pos;
}


template <typename Value, typename Key, typename HashFcn,
    typename ExtractKey, typename EqualKey, typename Alloc>
class hashtable : public icollector<hashtable<Value, Key, HashFcn, ExtractKey, EqualKey, Alloc>> {
    using node_type = hashtable_node<Value>;

public:
    using key_type          = Key;
    using hasher            = HashFcn;
    using key_equal         = EqualKey;

    MSTL_BUILD_TYPE_ALIAS(Value)

    using iterator          = hashtable_iterator<false, hashtable>;
    using const_iterator    = hashtable_iterator<true, hashtable>;

    using allocator_type    = Alloc;

private:
    vector<node_type*> buckets_{};
    size_type size_ = 0;
    hasher hasher_{};
    key_equal equals_{};
    ExtractKey extracter_{};
    compressed_pair<allocator_type, float> pair_{ _MSTL_TAG default_construct_tag{}, 1.0f };

    template <bool, typename>
    friend struct hashtable_iterator;

private:
    MSTL_NODISCARD static size_type next_size(const size_type n) noexcept {
        return hashtable_next_prime(n);
    }

    void initialize_buckets(const size_type n) {
        const size_type n_buckets = next_size(n);
        buckets_.assign(n_buckets, nullptr);
    }

    size_type bkt_num_key(const key_type& key, const size_t n) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        if (n == 0) return 0;
        return hasher_(key) % n;
    }

    size_type bkt_num(const value_type& obj, const size_t n) const
    noexcept(is_nothrow_hashable_v<key_type>) {
        return this->bkt_num_key(extracter_(obj), n);
    }

    template <typename... Args>
    node_type* new_node(Args&&... args) {
        node_type* n = pair_.get_base().allocate();
        n->next_ = nullptr;
        try {
            _MSTL construct(&n->data_, _MSTL forward<Args>(args)...);
        }
        catch (...) {
            this->delete_node(n);
            throw_exception(memory_exception("hashtable construct node failed."));
        }
        return n;
    }

    void delete_node(node_type* n) noexcept {
        _MSTL destroy(&n->data_);
        pair_.get_base().deallocate(n);
    }

    void copy_from(const hashtable& ht) {
        buckets_.clear();
        buckets_.reserve(ht.buckets_.size());
        buckets_.insert(buckets_.end(), ht.buckets_.size(), nullptr);
        try {
            for (size_type i = 0; i < ht.buckets_.size(); ++i) {
                if (const node_type* cur = ht.buckets_[i]) {
                    node_type* copy = this->new_node(cur->data_);
                    buckets_[i] = copy;
                    for (node_type* next = cur->next_; next != nullptr; cur = next, next = cur->next_) {
                        copy->next_ = this->new_node(next->data_);
                        copy = copy->next_;
                    }
                }
            }
            size_ = ht.size_;
        }
        catch (...) {
            clear();
            throw;
        }
    }

    pair<iterator, bool> insert_unique_noresize(node_type* x) {
        const size_type n = bkt_num(x->data_, buckets_.size());

        node_type** buckets_p = &buckets_[n];
        while (*buckets_p != nullptr) {
            if (equals_(extracter_((*buckets_p)->data_), extracter_(x->data_))) {
                x->next_ = (*buckets_p)->next_;
                delete_node(*buckets_p);
                *buckets_p = x;
                return {{x, this, n}, false};
            }
            buckets_p = &(*buckets_p)->next_;
        }
        x->next_ = nullptr;
        *buckets_p = x;
        ++size_;
        return {iterator{x, this, n}, true};
    }

    iterator insert_equal_noresize(node_type* x) {
        const size_type n = bkt_num(x->data_, buckets_.size());
        node_type* first = buckets_[n];

        node_type* prev = nullptr;
        node_type* cur = first;
        while (cur != nullptr && equals_(extracter_(cur->data_), extracter_(x->data_))) {
            prev = cur;
            cur = cur->next_;
        }

        if (prev != nullptr) {
            prev->next_ = x;
            x->next_ = cur;
        } else {
            x->next_ = first;
            buckets_[n] = x;
        }

        ++size_;
        return {x, this, n};
    }

    template <typename Iterator,
        enable_if_t<is_iter_v<Iterator> && !is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void insert_unique_aux(Iterator first, Iterator last) {
        for (; first != last; ++first)
            this->insert_unique(*first);
    }

    template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void insert_unique_aux(Iterator first, Iterator last) {
        size_type n = _MSTL distance(first, last);
        rehash(size_ + n);
        for (; n > 0; --n, ++first) {
            node_type* tmp = this->new_node(*first);
            this->insert_unique_noresize(tmp);
        }
    }

    template <typename Iterator,
        enable_if_t<is_ranges_input_iter_v<Iterator> && !is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void insert_equal_aux(Iterator first, Iterator last) {
        for (; first != last; ++first)
            this->insert_equal(*first);
    }

    template <typename Iterator, enable_if_t<
        is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void insert_equal_aux(Iterator first, Iterator last) {
        size_type n = _MSTL distance(first, last);
        rehash(size_ + n);
        for (; n > 0; --n, ++first) {
            node_type* tmp = this->new_node(*first);
            this->insert_equal_noresize(tmp);
        }
    }

    size_type erase_bucket_to_node(size_type bucket, node_type* last) noexcept {
        size_type count = 0;
        node_type* curr = buckets_[bucket];
        while (curr != nullptr && curr != last) {
            node_type* next = curr->next_;
            delete_node(curr);
            curr = next;
            --size_;
            ++count;
        }
        buckets_[bucket] = last;
        return count;
    }

    size_type erase_bucket_range(size_type bucket,
        node_type* first, node_type* last) noexcept {
        size_type count = 0;
        if (first == nullptr) return 0;

        if (buckets_[bucket] == first) {
            count += erase_bucket_to_node(bucket, last);
        } else {
            node_type* prev = buckets_[bucket];
            while (prev != nullptr && prev->next_ != first) {
                prev = prev->next_;
            }
            if (prev == nullptr) return 0;

            node_type* curr = first;
            while (curr != nullptr && curr != last) {
                node_type* next = curr->next_;
                prev->next_ = next;
                delete_node(curr);
                curr = next;
                ++count;
            }
        }
        return count;
    }

    size_type erase_bucket_completely(size_type bucket) noexcept {
        size_type count = 0;
        node_type* curr = buckets_[bucket];
        while (curr != nullptr) {
            node_type* next = curr->next_;
            delete_node(curr);
            curr = next;
            ++count;
        }
        buckets_[bucket] = nullptr;
        return count;
    }

    bool equal_small(const hashtable& rhs) const {
        for (const_iterator it1 = begin(); it1 != end(); ++it1) {
            const key_type& key = extracter_(*it1);
            const size_t count_this = _MSTL count_if(begin(), end(), [&](const value_type& val) {
                return equals_(extracter_(val), key);
            });
            const size_t count_rh = _MSTL count_if(rhs.begin(), rhs.end(), [&](const value_type& val) {
                return rhs.equals_(rhs.extracter_(val), key);
            });
            if (count_this != count_rh) return false;
        }
        return true;
    }

    bool equal_large(const hashtable& rhs) const {
        vector<value_type> elements_this, elements_rh;
        elements_this.reserve(size_);
        elements_rh.reserve(size_);

        for (const_iterator it = begin(); it != end(); ++it) {
            elements_this.push_back(*it);
        }
        for (const_iterator it = rhs.begin(); it != rhs.end(); ++it) {
            elements_rh.push_back(*it);
        }

        _MSTL sort(elements_this.begin(), elements_this.end());
        _MSTL sort(elements_rh.begin(), elements_rh.end());

        return elements_this == elements_rh;
    }

public:
    explicit hashtable(const size_type n)
    : hasher_(HashFcn()), equals_(EqualKey()) {
        initialize_buckets(n);
    }

    hashtable(const size_type n, const HashFcn& hf)
    : hasher_(hf), equals_(EqualKey()) {
        initialize_buckets(n);
    }
    hashtable(const size_type n, const HashFcn& hf, const EqualKey& eql)
    : hasher_(hf), equals_(eql) {
        initialize_buckets(n);
    }
    hashtable(const size_type n, const HashFcn& hf, const EqualKey& eql, const ExtractKey& ext)
    : hasher_(hf), equals_(eql), extracter_(ext) {
        initialize_buckets(n);
    }

    hashtable(const hashtable& ht)
    : hasher_(ht.hasher_), equals_(ht.equals_), extracter_(ht.extracter_), pair_(ht.pair_) {
        this->copy_from(ht);
    }
    hashtable& operator =(const hashtable& ht) {
        if (_MSTL addressof(ht) == this) return *this;
        clear();
        hasher_ = ht.hasher_;
        equals_ = ht.equals_;
        extracter_ = ht.extracter_;
        this->copy_from(ht);
        return *this;
    }

    hashtable(hashtable&& ht) noexcept(noexcept(this->swap(ht))) {
        this->swap(ht);
    }
    hashtable& operator =(hashtable&& ht) noexcept(noexcept(this->swap(ht))) {
        if (_MSTL addressof(ht) == this) return *this;
        clear();
        this->swap(ht);
        return *this;
    }

    ~hashtable() { clear(); }

    MSTL_NODISCARD iterator begin() noexcept {
        for (size_type n = 0; n < buckets_.size(); ++n) {
            if (buckets_[n] != nullptr)
                return iterator(buckets_[n], this, n);
        }
        return end();
    }
    MSTL_NODISCARD iterator end() noexcept { return iterator(nullptr, this, -1); }

    MSTL_NODISCARD const_iterator begin() const noexcept { return cbegin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return cend(); }

    MSTL_NODISCARD const_iterator cbegin() const noexcept {
        for (size_type n = 0; n < buckets_.size(); ++n) {
            if (buckets_[n] != nullptr)
                return const_iterator(buckets_[n], this, n);
        }
        return cend();
    }
    MSTL_NODISCARD const_iterator cend() const noexcept {
        return const_iterator(nullptr, this, -1);
    }

    MSTL_NODISCARD size_type size() const noexcept { return size_; }
    MSTL_NODISCARD size_type max_size() noexcept { return static_cast<size_type>(-1); }
    MSTL_NODISCARD bool empty() const noexcept { return size_ == 0; }
    MSTL_NODISCARD size_type bucket_count() const noexcept { return buckets_.size(); }
    MSTL_NODISCARD static size_type max_bucket_count() noexcept {
        return _CONSTANTS HASH_PRIME_LIST[_CONSTANTS HASH_PRIMER_COUNT - 1];
    }
    MSTL_NODISCARD size_type bucket(const key_type& key) const noexcept(is_nothrow_hashable_v<key_type>) {
        return bkt_num_key(key);
    }
    MSTL_NODISCARD size_type bucket_size(size_type bucket) const noexcept {
        size_type result = 0;
        for (node_type* cur = buckets_[bucket]; cur != nullptr; cur = cur->next_)
            result++;
        return result;
    }

    MSTL_NODISCARD allocator_type get_allocator() const noexcept { return allocator_type(); }

    MSTL_NODISCARD hasher hash_func() const noexcept(is_nothrow_copy_constructible_v<hasher>) {
        return hasher_;
    }
    MSTL_NODISCARD key_equal key_eql() const noexcept(is_nothrow_copy_constructible_v<key_equal>) {
        return equals_;
    }
    MSTL_NODISCARD float load_factor() const noexcept {
        return bucket_count() == 0 ? 0.0f : static_cast<float>(size()) / static_cast<float>(bucket_count());
    }
    MSTL_NODISCARD float max_load_factor() const noexcept {
        return pair_.value;
    }
    void max_load_factor(float new_max) noexcept {
        MSTL_DEBUG_VERIFY(new_max > 0, "hashtable load factor invalid.");
        pair_.value = new_max;
    }

    void rehash(const size_type new_size) {
        const size_type min_buckets_for_size = static_cast<size_type>(
            _MSTL ceil(static_cast<double>(size_) / max_load_factor())
        );
        const size_type target = _MSTL max(new_size, min_buckets_for_size);
        const size_type old_size = buckets_.size();
        if (target <= old_size) return;

        const size_type n = next_size(target);
        if (n < target) {
            throw_exception(value_exception("hashtable size exceeds max count"));
        }

        vector<node_type*> new_buckets(n, nullptr);

        for (size_type bucket = 0; bucket < old_size; ++bucket) {
            node_type* cur = buckets_[bucket];
            while (cur != nullptr) {
                node_type* next = cur->next_;
                const size_type new_bucket = bkt_num(cur->data_, n);

                cur->next_ = new_buckets[new_bucket];
                new_buckets[new_bucket] = cur;

                cur = next;
            }
            buckets_[bucket] = nullptr;
        }

        buckets_.swap(new_buckets);
    }

    void reserve(const size_type count) {
        if (count <= size_) return;

        const float needed = static_cast<float>(count) / max_load_factor();
        if (needed > static_cast<float>(max_bucket_count())) {
            throw_exception(value_exception("hashtable size exceeds max count"));
        }
        rehash(static_cast<size_type>(needed));
    }

    template <typename... Args>
    pair<iterator, bool> emplace_unique(Args&&... args) {
        if (size_ + 1 > static_cast<size_type>(buckets_.size() * max_load_factor())) {
            rehash(size_ + 1);
        }
        node_type* node = (new_node)(_MSTL forward<Args>(args)...);
        return (insert_unique_noresize)(node);
    }
    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        if (size_ + 1 > static_cast<size_type>(buckets_.size() * max_load_factor())) {
            rehash(size_ + 1);
        }
        node_type* node = (new_node)(_MSTL forward<Args>(args)...);
        return (insert_equal_noresize)(node);
    }

    pair<iterator, bool> insert_unique(const value_type& x) {
        return (emplace_unique)(x);
    }
    pair<iterator, bool> insert_unique(value_type&& x) {
        return (emplace_unique)(_MSTL move(x));
    }
    iterator insert_equal(const value_type& x) {
        return (emplace_equal)(x);
    }
    iterator insert_equal(value_type&& x) {
        return (emplace_equal)(_MSTL move(x));
    }

    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert_unique(Iterator first, Iterator last) {
        this->insert_unique_aux(first, last);
    }

    void insert_unique(std::initializer_list<value_type> l) {
        this->insert_unique(l.begin(), l.end());
    }

    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert_equal(Iterator first, Iterator last) {
        this->insert_equal_aux(first, last);
    }

    void insert_equal(std::initializer_list<value_type> l) {
        this->insert_equal(l.begin(), l.end());
    }

    size_type erase(const key_type& key) noexcept(is_nothrow_hashable_v<key_type>) {
        const size_type n = this->bkt_num_key(key, buckets_.size());
        node_type* first = buckets_[n];
        size_type erased = 0;
        if (first != nullptr) {
            node_type* cur = first;
            node_type* next = cur->next_;
            while (next != nullptr) {
                if (equals_(extracter_(next->data_), key)) {
                    cur->next_ = next->next_;
                    delete_node(next);
                    next = cur->next_;
                    ++erased;
                    --size_;
                }
                else {
                    cur = next;
                    next = cur->next_;
                }
            }
            if (equals_(extracter_(first->data_), key)) {
                buckets_[n] = first->next_;
                this->delete_node(first);
                ++erased;
                --size_;
            }
        }
        return erased;
    }
    iterator erase(const iterator& it) noexcept(is_nothrow_hashable_v<key_type>) {
        if (it.cur_ == nullptr || it.ht_ != this) {
            return end();
        }

        const size_type n = it.bucket_;
        node_type* const p = it.cur_;
        node_type* next_node = p->next_;

        node_type* prev = nullptr;
        node_type* curr = buckets_[n];
        while (curr != nullptr && curr != p) {
            prev = curr;
            curr = curr->next_;
        }

        if (curr == nullptr) return end();

        if (prev == nullptr) {
            buckets_[n] = next_node;
        } else {
            prev->next_ = next_node;
        }

        this->delete_node(p);
        --size_;

        if (next_node != nullptr) {
            return iterator(next_node, this, n);
        }

        for (size_type bucket = n + 1; bucket < buckets_.size(); ++bucket) {
            if (buckets_[bucket] != nullptr) {
                return iterator(buckets_[bucket], this, bucket);
            }
        }
        return end();
    }

    iterator erase(iterator first, iterator last) noexcept(is_nothrow_hashable_v<key_type>) {
        if (first == last) {
            return last;
        }
        if (first.ht_ != this || (last.ht_ != this && last != end())) {
            return end();
        }
        size_type count_erased = 0;

        if (first.bucket_ == last.bucket_) {
            count_erased = erase_bucket_range(first.bucket_, first.cur_, last.cur_);
        } else {
            count_erased += erase_bucket_range(first.bucket_, first.cur_, nullptr);
            for (size_type bucket = first.bucket_ + 1; bucket < last.bucket_; ++bucket) {
                count_erased += erase_bucket_completely(bucket);
            }
            if (last.bucket_ < buckets_.size()) {
                count_erased += erase_bucket_range(last.bucket_, buckets_[last.bucket_], last.cur_);
            }
        }
        size_ -= count_erased;
        return last;
    }

    const_iterator erase(const const_iterator& it) noexcept(is_nothrow_hashable_v<key_type>) {
        return this->erase(iterator(it));
    }

    const_iterator erase(const_iterator first, const_iterator last) noexcept(is_nothrow_hashable_v<key_type>) {
        return this->erase(iterator(first), iterator(last));
    }

    void clear() noexcept {
        for (size_type i = 0; i < buckets_.size(); ++i) {
            node_type* cur = buckets_[i];
            while (cur != nullptr) {
                node_type* next = cur->next_;
                this->delete_node(cur);
                cur = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    MSTL_NODISCARD iterator find(const key_type& key) noexcept(is_nothrow_hashable_v<key_type>) {
        if (buckets_.empty()) return end();

        size_type n = this->bkt_num_key(key, buckets_.size());
        for (node_type* first = buckets_[n]; first != nullptr; first = first->next_) {
            if (equals_(extracter_(first->data_), key)) {
                return iterator(first, this, n);
            }
        }
        return end();
    }

    MSTL_NODISCARD const_iterator find(const key_type& key) const noexcept(is_nothrow_hashable_v<key_type>) {
        if (buckets_.empty()) return cend();

        size_type n = this->bkt_num_key(key, buckets_.size());
        for (node_type* first = buckets_[n]; first != nullptr; first = first->next_) {
            if (equals_(extracter_(first->data_), key)) {
                return const_iterator(first, this, n);
            }
        }
        return cend();
    }

    MSTL_NODISCARD size_type count(const key_type& key) const noexcept(is_nothrow_hashable_v<key_type>) {
        if (buckets_.empty()) return 0;
        const size_type n = this->bkt_num_key(key, buckets_.size());
        size_type result = 0;
        for (const node_type* cur = buckets_[n]; cur != nullptr; cur = cur->next_) {
            if (equals_(extracter_(cur->data_), key)) ++result;
        }
        return result;
    }
    MSTL_NODISCARD bool contains(const key_type& key) const noexcept(is_nothrow_hashable_v<key_type>) {
        return find(key) != cend();
    }

    MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) {
        if (buckets_.empty()) return {end(), end()};

        const size_type n = this->bkt_num_key(key, buckets_.size());
        node_type* first_match = nullptr;
        node_type* last_match = nullptr;
        node_type* prev = nullptr;

        for (node_type* curr = buckets_[n]; curr != nullptr; prev = curr, curr = curr->next_) {
            if (equals_(extracter_(curr->data_), key)) {
                if (first_match == nullptr) {
                    first_match = curr;
                }
                last_match = curr;
            } else if (first_match != nullptr) {
                break;
            }
        }

        if (first_match == nullptr) {
            return {end(), end()};
        }

        node_type* range_end = (last_match != nullptr) ? last_match->next_ : nullptr;
        return {
            iterator(first_match, this, n),
            iterator(range_end, this, n)
        };
    }

    MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        if (buckets_.empty()) return {cend(), cend()};

        const size_type n = this->bkt_num_key(key, buckets_.size());
        const node_type* first_match = nullptr;
        const node_type* last_match = nullptr;
        const node_type* prev = nullptr;

        for (const node_type* curr = buckets_[n]; curr != nullptr; prev = curr, curr = curr->next_) {
            if (equals_(extracter_(curr->data_), key)) {
                if (first_match == nullptr) {
                    first_match = curr;
                }
                last_match = curr;
            } else if (first_match != nullptr) {
                break;
            }
        }

        if (first_match == nullptr) {
            return {cend(), cend()};
        }

        const node_type* range_end = (last_match != nullptr) ? last_match->next_ : nullptr;
        return {
            const_iterator(first_match, this, n),
            const_iterator(range_end, this, n)
        };
    }

    void swap(hashtable& ht) noexcept(is_nothrow_swappable_v<HashFcn> && is_nothrow_swappable_v<EqualKey>) {
        if (_MSTL addressof(ht) == this) return;
        _MSTL swap(hasher_, ht.hasher_);
        _MSTL swap(equals_, ht.equals_);
        _MSTL swap(extracter_, ht.extracter_);
        buckets_.swap(ht.buckets_);
        _MSTL swap(size_, ht.size_);
        pair_.swap(ht.pair_);
    }

    MSTL_NODISCARD bool operator ==(const hashtable& rhs) const {
        if (size_ != rhs.size_) return false;
        if (size_ == 0) return true;
        if (this == &rhs) return true;

        if (size_ < 100) return equal_small(rhs);
        return equal_large(rhs);
    }

    MSTL_NODISCARD bool operator <(const hashtable& rhs) const
    noexcept(noexcept(_MSTL lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend()))) {
        return _MSTL lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend());
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_HASHTABLE_HPP__
