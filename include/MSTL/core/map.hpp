#ifndef MSTL_MAP_HPP__
#define MSTL_MAP_HPP__
#include "rb_tree.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Key, typename T, typename Compare = less<Key>,
	typename Alloc = allocator<rb_tree_node<pair<const Key, T>>>>
class map : public icollector<map<Key, T, Compare, Alloc>> {
#ifdef MSTL_VERSION_20__	
	static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
	static_assert(is_same_v<rb_tree_node<pair<const Key, T>>, typename Alloc::value_type>, 
		"allocator type mismatch.");
	static_assert(is_object_v<T>, "map only contains object types.");

	using self = map<Key, T, Compare, Alloc>;
	using super = icollector<self>;
	using base_type = rb_tree<Key, pair<const Key, T>, select1st<pair<const Key, T>>, Compare, Alloc>;

public:
	using key_type		= Key;
	using data_type		= T;
	using mapped_type	= T;
	using value_type	= pair<const Key, T>;
	using key_compare	= Compare;

	struct value_compare {
	private:
		Compare comp_;
		friend class map;

	public:
		bool operator ()(const value_type& x, const value_type& y) const noexcept {
			return comp_(x.first, y.first);
		}
	};

public:
	using size_type			= typename base_type::size_type;
	using difference_type	= typename base_type::difference_type;
	using pointer			= typename base_type::pointer;
	using const_pointer		= typename base_type::const_pointer;
	using reference			= typename base_type::reference;
	using const_reference	= typename base_type::const_reference;

	using iterator					= typename base_type::iterator;
	using const_iterator			= typename base_type::const_iterator;
	using reverse_iterator			= typename base_type::reverse_iterator;
	using const_reverse_iterator	= typename base_type::const_reverse_iterator;

	using allocator_type	= typename base_type::allocator_type;

private:
	base_type tree_;

public:
	map() : tree_(Compare()) {}
	explicit map(const key_compare& comp) : tree_(comp) {}

	map(const self& x) : tree_(x.tree_) {}

	self& operator =(const self& x) = default;

	map(self&& x) noexcept(is_nothrow_move_constructible_v<base_type>) 
		: tree_(_MSTL move(x.tree_)) {}

	self& operator =(self&& x) noexcept(noexcept(swap(x))) {
		tree_ = _MSTL move(x.tree_);
		return *this;
	}

	template <typename Iterator>
	map(Iterator first, Iterator last) : tree_(Compare()) {
		tree_.insert_unique(first, last);
	}
	template <typename Iterator>
	map(Iterator first, Iterator last, const key_compare& comp) : tree_(comp) {
		tree_.insert_unique(first, last);
	}

	map(std::initializer_list<value_type> l) : map(l.begin(), l.end()) {}
	map(std::initializer_list<value_type> l, const key_compare& comp) : map(l.begin(), l.end(), comp) {}

	self& operator =(std::initializer_list<value_type> l) {
		clear();
		insert(l.begin(), l.end());
		return *this;
	}
	~map() = default;

	MSTL_NODISCARD iterator begin() noexcept { return tree_.begin(); }
    MSTL_NODISCARD iterator end() noexcept { return tree_.end(); }
    MSTL_NODISCARD const_iterator begin() const noexcept { return tree_.cbegin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return tree_.cend(); }
	MSTL_NODISCARD const_iterator cbegin() const noexcept { return tree_.cbegin(); }
	MSTL_NODISCARD const_iterator cend() const noexcept { return tree_.cend(); }
	MSTL_NODISCARD reverse_iterator rbegin() noexcept { return tree_.rbegin(); }
    MSTL_NODISCARD reverse_iterator rend() noexcept { return tree_.rend(); }
    MSTL_NODISCARD const_reverse_iterator rbegin() const noexcept { return tree_.rbegin(); }
    MSTL_NODISCARD const_reverse_iterator rend() const noexcept { return tree_.rend(); }
	MSTL_NODISCARD const_reverse_iterator crbegin() const noexcept { return tree_.crbegin(); }
	MSTL_NODISCARD const_reverse_iterator crend() const noexcept { return tree_.crend(); }

	MSTL_NODISCARD size_type size() const noexcept { return tree_.size(); }
	MSTL_NODISCARD size_type max_size() const noexcept { return tree_.max_size(); }
	MSTL_NODISCARD bool empty() const noexcept { return tree_.empty(); }

	MSTL_NODISCARD allocator_type get_allocator() const noexcept { return allocator_type(); }

	MSTL_NODISCARD key_compare key_comp() const noexcept { return tree_.key_comp(); }
	MSTL_NODISCARD value_compare value_comp() const noexcept { return value_compare(tree_.key_comp()); }

	template <typename... Args>
	pair<iterator, bool> emplace(Args&&... args) {
		return tree_.emplace_unique(_MSTL forward<Args>(args)...);
	}
	pair<iterator, bool> insert(const value_type& x) {
		return tree_.insert_unique(x);
	}
	pair<iterator, bool> insert(value_type&& x) {
		return tree_.insert_unique(_MSTL move(x));
	}

	template <typename... Args>
	iterator emplace_hint(iterator position, Args&&... args) {
		return tree_.emplace_unique_hint(position, _MSTL forward<Args>(args)...);
	}
	iterator insert(iterator position, const value_type& x) {
		return tree_.insert_unique(position, x);
	}
	iterator insert(iterator position, value_type&& x) {
		return tree_.insert_unique(position, _MSTL move(x));
	}

	template <typename Iterator>
	void insert(Iterator first, Iterator last) {
		tree_.insert_unique(first, last);
	}

	void erase(iterator position) noexcept { tree_.erase(position); }
	size_type erase(const key_type& x) noexcept { return tree_.erase(x); }
	void erase(iterator first, iterator last) noexcept { tree_.erase(first, last); }

	void clear() noexcept { tree_.clear(); }

	MSTL_NODISCARD iterator find(const key_type& x) { return tree_.find(x); }
	MSTL_NODISCARD const_iterator find(const key_type& x) const { return tree_.find(x); }
	MSTL_NODISCARD size_type count(const key_type& x) const { return tree_.count(x); }

	MSTL_NODISCARD iterator lower_bound(const key_type& x) { return tree_.lower_bound(x); }
	MSTL_NODISCARD const_iterator lower_bound(const key_type& x) const { return tree_.lower_bound(x); }
	MSTL_NODISCARD iterator upper_bound(const key_type& x) { return tree_.upper_bound(x); }
	MSTL_NODISCARD const_iterator upper_bound(const key_type& x) const { return tree_.upper_bound(x); }

	MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& x) { return tree_.equal_range(x); }
	MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& x) const {
		return tree_.equal_range(x);
	}

	MSTL_NODISCARD mapped_type& operator [](const key_type& k) {
		iterator iter = tree_.lower_bound(k);
		if (iter == end() || key_comp()(k, iter->first))
			iter = tree_.emplace_unique_hint(iter, k, T());
		return iter->second;
	}
	MSTL_NODISCARD mapped_type& operator [](key_type&& k) {
		iterator iter = tree_.lower_bound(k);
		if (iter == end() || key_comp()(k, iter->first))
			iter = tree_.emplace_unique_hint(iter, _MSTL move(k), T());
		return iter->second;
	}
	MSTL_NODISCARD const mapped_type& at(const key_type& k) const {
		const_iterator iter = lower_bound(k);
		Exception(!(iter == cend() && key_comp()(iter->first, k)),
			ValueError("the value of this key does not exists."));
		return iter->second;
	}
	MSTL_NODISCARD mapped_type& at(const key_type& k) {
		return const_cast<mapped_type&>(const_cast<const self*>(this)->at(k));
	}

	void swap(self& x) noexcept(noexcept(tree_.swap(x.tree_))) { tree_.swap(x.tree_); }

	MSTL_NODISCARD bool operator ==(const self& rh) const
    noexcept(noexcept(tree_ == rh.tree_)) {
		return tree_ == rh.tree_;
	}
	MSTL_NODISCARD bool operator !=(const self& rh) const
	noexcept(noexcept(tree_ != rh.tree_)) {
		return tree_ != rh.tree_;
	}
	MSTL_NODISCARD bool operator <(const self& rh) const
	noexcept(noexcept(tree_ < rh.tree_)) {
		return tree_ < rh.tree_;
	}
	MSTL_NODISCARD bool operator >(const self& rh) const
	noexcept(noexcept(tree_ > rh.tree_)) {
		return tree_ > rh.tree_;
	}
	MSTL_NODISCARD bool operator <=(const self& rh) const
	noexcept(noexcept(tree_ <= rh.tree_)) {
		return tree_ <= rh.tree_;
	}
	MSTL_NODISCARD bool operator >=(const self& rh) const
	noexcept(noexcept(tree_ >= rh.tree_)) {
		return tree_ >= rh.tree_;
	}

	MSTL_NODISCARD size_type to_hash() const noexcept { return tree_.to_hash(); }
	MSTL_NODISCARD string to_string() const { return tree_.to_string(); }
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename Compare, typename Alloc
	= allocator<pair<const get_iter_key_t<Iterator>, get_iter_val_t<Iterator>>>>
map(Iterator, Iterator, Compare = Compare(), Alloc = Alloc()) ->
map<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>, Compare, Alloc>;

template <typename Key, typename T, typename Compare = less<Key>, typename Alloc 
	= allocator<pair<const Key, T>>>
map(std::initializer_list<pair<Key, T>>, Compare = Compare(), Alloc = Alloc()) -> map<Key, T, Compare, Alloc>;

template <typename Iterator, typename Alloc>
map(Iterator, Iterator, Alloc) ->
map<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>, less<get_iter_key_t<Iterator>>, Alloc>;

template <typename Key, typename T, typename Alloc>
map(std::initializer_list<pair<Key, T>>, Alloc) -> map<Key, T, less<Key>, Alloc>;
#endif


template <typename Key, typename T, typename Compare = less<Key>,
	typename Alloc = allocator<rb_tree_node<pair<const Key, T>>>>
class multimap : public icollector<multimap<Key, T, Compare, Alloc>> {
#ifdef MSTL_VERSION_20__	
	static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
	static_assert(is_same_v<rb_tree_node<pair<const Key, T>>, typename Alloc::value_type>,
		"allocator type mismatch.");
	static_assert(is_object_v<T>, "multimap only contains object types.");

	using self = multimap<Key, T, Compare, Alloc>;
	using super = icollector<self>;
	using base_type			= rb_tree<Key, pair<const Key, T>, select1st<pair<const Key, T>>, Compare, Alloc>;

public:
	using key_type		= Key;
	using data_type		= T;
	using mapped_type	= T;
	using value_type	= pair<const Key, T>;
	using key_compare	= Compare;

	struct value_compare {
	private:
		Compare comp_;
		friend class multimap;

	public:
		bool operator ()(const value_type& x, const value_type& y) const noexcept {
			return comp_(x.first, y.first);
		}
	};

public:
	using size_type			= typename base_type::size_type;
	using difference_type	= typename base_type::difference_type;
	using pointer			= typename base_type::pointer;
	using const_pointer		= typename base_type::const_pointer;
	using reference			= typename base_type::reference;
	using const_reference	= typename base_type::const_reference;

	using iterator					= typename base_type::iterator;
	using const_iterator			= typename base_type::const_iterator;
	using reverse_iterator			= typename base_type::reverse_iterator;
	using const_reverse_iterator	= typename base_type::const_reverse_iterator;

	using allocator_type	= typename base_type::allocator_type;

private:
	base_type tree_;

	template <typename Key1, typename T1, typename Compare1, typename Alloc1>
	friend bool operator ==(const multimap<Key1, T1, Compare1, Alloc1>&,
		const multimap<Key1, T1, Compare1, Alloc1>&) noexcept;
	template <typename Key1, typename T1, typename Compare1, typename Alloc1>
	friend bool operator <(const multimap<Key1, T1, Compare1, Alloc1>&,
		const multimap<Key1, T1, Compare1, Alloc1>&) noexcept;

public:
	multimap() : tree_(Compare()) {}
	explicit multimap(const key_compare& comp) : tree_(comp) {}

	multimap(const self& x) : tree_(x.tree_) {}

	self& operator =(const self& x) = default;

	multimap(self&& x) noexcept(is_nothrow_move_constructible_v<base_type>)
		: tree_(_MSTL move(x.tree_)) {}

	self& operator =(self&& x) noexcept(noexcept(swap(x))) {
		tree_ = _MSTL move(x.tree_);
		return *this;
	}

	template <typename Iterator>
	multimap(Iterator first, Iterator last) : tree_(Compare()) {
		tree_.insert_equal(first, last);
	}
	template <typename Iterator>
	multimap(Iterator first, Iterator last, const key_compare& comp) : tree_(comp) {
		tree_.insert_equal(first, last);
	}

	multimap(std::initializer_list<value_type> l) : multimap(l.begin(), l.end()) {}
	multimap(std::initializer_list<value_type> l, const key_compare& comp) : multimap(l.begin(), l.end(), comp) {}

	self& operator =(std::initializer_list<value_type> l) {
		clear();
		insert(l.begin(), l.end());
		return *this;
	}
	~multimap() = default;

	MSTL_NODISCARD iterator begin() noexcept { return tree_.begin(); }
    MSTL_NODISCARD iterator end() noexcept { return tree_.end(); }
    MSTL_NODISCARD const_iterator begin() const noexcept { return tree_.cbegin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return tree_.cend(); }
	MSTL_NODISCARD const_iterator cbegin() const noexcept { return tree_.cbegin(); }
	MSTL_NODISCARD const_iterator cend() const noexcept { return tree_.cend(); }
	MSTL_NODISCARD reverse_iterator rbegin() noexcept { return tree_.rbegin(); }
    MSTL_NODISCARD reverse_iterator rend() noexcept { return tree_.rend(); }
    MSTL_NODISCARD const_reverse_iterator rbegin() const noexcept { return tree_.rbegin(); }
    MSTL_NODISCARD const_reverse_iterator rend() const noexcept { return tree_.rend(); }
	MSTL_NODISCARD const_reverse_iterator crbegin() const noexcept { return tree_.crbegin(); }
	MSTL_NODISCARD const_reverse_iterator crend() const noexcept { return tree_.crend(); }

	MSTL_NODISCARD size_type size() const noexcept { return tree_.size(); }
	MSTL_NODISCARD size_type max_size() const noexcept { return tree_.max_size(); }
	MSTL_NODISCARD bool empty() const noexcept { return tree_.empty(); }

	MSTL_NODISCARD allocator_type get_allocator() const noexcept { return allocator_type(); }

	MSTL_NODISCARD key_compare key_comp() const noexcept { return tree_.key_comp(); }
	MSTL_NODISCARD value_compare value_comp() const noexcept { return value_compare(tree_.key_comp()); }

	template <typename... Args>
	iterator emplace(Args&&... args) {
		return tree_.emplace_equal(_MSTL forward<Args>(args)...);
	}
	iterator insert(const value_type& x) {
		return tree_.insert_equal(x);
	}
	iterator insert(value_type&& x) {
		return tree_.insert_equal(_MSTL move(x));
	}

	template <typename... Args>
	iterator emplace_hint(iterator position, Args&&... args) {
		return tree_.emplace_equal_hint(position, _MSTL forward<Args>(args)...);
	}
	iterator insert(iterator position, const value_type& x) {
		return tree_.insert_equal(position, x);
	}
	iterator insert(iterator position, value_type&& x) {
		return tree_.insert_equal(position, _MSTL move(x));
	}

	template <typename Iterator>
	void insert(Iterator first, Iterator last) {
		tree_.insert_equal(first, last);
	}

	void erase(iterator position) noexcept { tree_.erase(position); }
	size_type erase(const key_type& x) noexcept { return tree_.erase(x); }
	void erase(iterator first, iterator last) noexcept { tree_.erase(first, last); }

	void clear() noexcept { tree_.clear(); }

	MSTL_NODISCARD iterator find(const key_type& x) { return tree_.find(x); }
	MSTL_NODISCARD const_iterator find(const key_type& x) const { return tree_.find(x); }
	MSTL_NODISCARD size_type count(const key_type& x) const { return tree_.count(x); }

	MSTL_NODISCARD iterator lower_bound(const key_type& x) { return tree_.lower_bound(x); }
	MSTL_NODISCARD const_iterator lower_bound(const key_type& x) const { return tree_.lower_bound(x); }
	MSTL_NODISCARD iterator upper_bound(const key_type& x) { return tree_.upper_bound(x); }
	MSTL_NODISCARD const_iterator upper_bound(const key_type& x) const { return tree_.upper_bound(x); }

	MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& x) { return tree_.equal_range(x); }
	MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& x) const {
		return tree_.equal_range(x);
	}

	void swap(self& x) noexcept(noexcept(tree_.swap(x.tree_))) { tree_.swap(x.tree_); }

	MSTL_NODISCARD bool operator ==(const self& rh) const
    noexcept(noexcept(tree_ == rh.tree_)) {
		return tree_ == rh.tree_;
	}
	MSTL_NODISCARD bool operator !=(const self& rh) const
	noexcept(noexcept(tree_ != rh.tree_)) {
		return tree_ != rh.tree_;
	}
	MSTL_NODISCARD bool operator <(const self& rh) const
	noexcept(noexcept(tree_ < rh.tree_)) {
		return tree_ < rh.tree_;
	}
	MSTL_NODISCARD bool operator >(const self& rh) const
	noexcept(noexcept(tree_ > rh.tree_)) {
		return tree_ > rh.tree_;
	}
	MSTL_NODISCARD bool operator <=(const self& rh) const
	noexcept(noexcept(tree_ <= rh.tree_)) {
		return tree_ <= rh.tree_;
	}
	MSTL_NODISCARD bool operator >=(const self& rh) const
	noexcept(noexcept(tree_ >= rh.tree_)) {
		return tree_ >= rh.tree_;
	}

	MSTL_NODISCARD size_type to_hash() const noexcept { return tree_.to_hash(); }
	MSTL_NODISCARD string to_string() const { return tree_.to_string(); }
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename Compare, typename Alloc
	= allocator<pair<const get_iter_key_t<Iterator>, get_iter_val_t<Iterator>>>>
multimap(Iterator, Iterator, Compare = Compare(), Alloc = Alloc()) ->
	multimap<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>, Compare, Alloc>;

template <typename Key, typename T, typename Compare = less<Key>, typename Alloc 
	= allocator<pair<const Key, T>>>
multimap(std::initializer_list<pair<Key, T>>, Compare = Compare(), Alloc = Alloc()) 
-> multimap<Key, T, Compare, Alloc>;

template <typename Iterator, typename Alloc>
multimap(Iterator, Iterator, Alloc) ->
multimap<get_iter_key_t<Iterator>, get_iter_val_t<Iterator>, less<get_iter_key_t<Iterator>>, Alloc>;

template <typename Key, typename T, typename Alloc>
multimap(std::initializer_list<pair<Key, T>>, Alloc) -> multimap<Key, T, less<Key>, Alloc>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_MAP_HPP__
