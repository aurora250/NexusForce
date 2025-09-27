#ifndef MSTL_SET_HPP__
#define MSTL_SET_HPP__
#include "rb_tree.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Key, typename Compare = less<Key>, typename Alloc = allocator<rb_tree_node<Key>>>
class set {
#ifdef MSTL_VERSION_20__	
	static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
	static_assert(is_same_v<rb_tree_node<Key>, typename Alloc::value_type>,
		"allocator type mismatch.");
	static_assert(is_object_v<Key>, "set only contains object types.");

public:
	using key_type			= Key;
	using value_type		= Key;
	using key_compare		= Compare;
	using value_compare		= Compare;
private:
	using base_type			= rb_tree<key_type, value_type, identity<value_type>, key_compare, Alloc>;
public:
	using size_type			= typename base_type::size_type;
	using difference_type	= typename base_type::difference_type;
	using pointer			= typename base_type::pointer;
	using const_pointer		= typename base_type::const_pointer;
	using reference			= typename base_type::reference;
	using const_reference	= typename base_type::const_reference;
	using iterator			= typename base_type::iterator;
	using const_iterator	= typename base_type::const_iterator;
	using reverse_iterator  = typename base_type::reverse_iterator;
	using const_reverse_iterator = typename base_type::const_reverse_iterator;
	using allocator_type	= typename base_type::allocator_type;
	using self				= set<Key, Compare, Alloc>;

private:
	base_type tree_;

	template <typename Key1, typename Compare1, typename Alloc1>
	friend bool operator ==(const set<Key1, Compare1, Alloc1>&,
		const set<Key1, Compare1, Alloc1>&) noexcept;
	template <typename Key1, typename Compare1, typename Alloc1>
	friend bool operator <(const set<Key1, Compare1, Alloc1>&,
		const set<Key1, Compare1, Alloc1>&) noexcept;

public:
	set() : tree_(Compare()) {}
	explicit set(const key_compare& comp) : tree_(comp) {}

	set(const self& x) : tree_(x.tree_) {}

	self& operator =(const self& x) = default;

	set(self&& x) noexcept(is_nothrow_move_constructible_v<base_type>)
		: tree_(_MSTL move(x.tree_)) {}

	self& operator =(self&& x) noexcept(noexcept(swap(x))) {
		tree_ = _MSTL move(x.tree_);
		return *this;
	}

	template <typename Iterator>
	set(Iterator first, Iterator last) : tree_(Compare()) {
		tree_.insert_unique(first, last);
	}
	template <typename Iterator>
	set(Iterator first, Iterator last, const key_compare& comp) : tree_(comp) {
		tree_.insert_unique(first, last);
	}

	set(std::initializer_list<value_type> l) : set(l.begin(), l.end()) {}
	set(std::initializer_list<value_type> l, const key_compare& comp) : set(l.begin(), l.end(), comp) {}

	self& operator =(std::initializer_list<value_type> l) {
		clear();
		insert(l.begin(), l.end());
		return *this;
	}
	~set() = default;

	MSTL_NODISCARD iterator begin() noexcept { return tree_.begin(); }
	MSTL_NODISCARD iterator end() noexcept { return tree_.end(); }
	MSTL_NODISCARD const_iterator cbegin() const noexcept { return tree_.cbegin(); }
	MSTL_NODISCARD const_iterator cend() const noexcept { return tree_.cend(); }
	MSTL_NODISCARD reverse_iterator rbegin() noexcept { return tree_.rbegin(); }
	MSTL_NODISCARD reverse_iterator rend() noexcept { return tree_.rend(); }
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

	void swap(self& x) noexcept(noexcept(tree_.swap(x.tree_))) { tree_.swap(x.tree_); }

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
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename Compare = less<iter_val_t<Iterator>>,
	typename Alloc = allocator<iter_val_t<Iterator>>>
set(Iterator, Iterator, Compare = Compare(), Alloc = Alloc()) -> set<iter_val_t<Iterator>, Compare, Alloc>;

template <typename Key, typename Compare = less<Key>, typename Alloc = allocator<Key>>
set(std::initializer_list<Key>, Compare = Compare(), Alloc = Alloc()) -> set<Key, Compare, Alloc>;

template <typename Iterator, typename Alloc>
set(Iterator, Iterator, Alloc) -> set<iter_val_t<Iterator>, less<iter_val_t<Iterator>>, Alloc>;

template <typename Key, typename Alloc>
set(std::initializer_list<Key>, Alloc) -> set<Key, less<Key>, Alloc>;
#endif

template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator ==(
	const set<Key, Compare, Alloc>& lh,
	const set<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ == rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator !=(
	const set<Key, Compare, Alloc>& lh,
	const set<Key, Compare, Alloc>& rh) noexcept {
	return !(lh.tree_ == rh.tree_);
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator <(
	const set<Key, Compare, Alloc>& lh,
	const set<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ < rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator >(
	const set<Key, Compare, Alloc>& lh,
	const set<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ > rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator <=(
	const set<Key, Compare, Alloc>& lh,
	const set<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ <= rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator >=(
	const set<Key, Compare, Alloc>& lh,
	const set<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ >= rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
void swap(set<Key, Compare, Alloc>& lh, set<Key, Compare, Alloc>& rh) 
noexcept(noexcept(lh.swap(rh))) {
	lh.swap(rh);
}


template <typename Key, typename Compare = less<Key>, typename Alloc = allocator<rb_tree_node<Key>>>
class multiset {
#ifdef MSTL_VERSION_20__	
	static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
	static_assert(is_same_v<rb_tree_node<Key>, typename Alloc::value_type>,
		"allocator type mismatch.");
	static_assert(is_object_v<Key>, "multiset only contains object types.");

public:
	using key_type			= Key;
	using value_type		= Key;
	using key_compare		= Compare;
	using value_compare		= Compare;
private:
	using base_type			= rb_tree<key_type, value_type, identity<value_type>, key_compare, Alloc>;
public:
	using size_type			= typename base_type::size_type;
	using difference_type	= typename base_type::difference_type;
	using pointer			= typename base_type::pointer;
	using const_pointer		= typename base_type::const_pointer;
	using reference			= typename base_type::reference;
	using const_reference	= typename base_type::const_reference;
	using iterator			= typename base_type::iterator;
	using const_iterator	= typename base_type::const_iterator;
	using reverse_iterator	= typename base_type::reverse_iterator;
	using const_reverse_iterator = typename base_type::const_reverse_iterator;
	using allocator_type	= typename base_type::allocator_type;
	using self				= multiset<Key, Compare, Alloc>;

private:
	base_type tree_;

	template <typename Key1, typename Compare1, typename Alloc1>
	friend bool operator ==(const multiset<Key1, Compare1, Alloc1>&,
		const multiset<Key1, Compare1, Alloc1>&) noexcept;
	template <typename Key1, typename Compare1, typename Alloc1>
	friend bool operator <(const multiset<Key1, Compare1, Alloc1>&,
		const multiset<Key1, Compare1, Alloc1>&) noexcept;

public:
	multiset() : tree_(Compare()) {}
	explicit multiset(const key_compare& comp) : tree_(comp) {}

	multiset(const self& x) : tree_(x.tree_) {}

	self& operator =(const self& x) = default;

	multiset(self&& x) noexcept(is_nothrow_move_constructible_v<base_type>)
		: tree_(_MSTL move(x.tree_)) {
	}

	self& operator =(self&& x) noexcept(noexcept(swap(x))) {
		tree_ = _MSTL move(x.tree_);
		return *this;
	}

	template <typename Iterator>
	multiset(Iterator first, Iterator last) : tree_(Compare()) {
		tree_.insert_equal(first, last);
	}
	template <typename Iterator>
	multiset(Iterator first, Iterator last, const key_compare& comp) : tree_(comp) {
		tree_.insert_equal(first, last);
	}

	multiset(std::initializer_list<value_type> l) : multiset(l.begin(), l.end()) {}
	multiset(std::initializer_list<value_type> l, const key_compare& comp) : multiset(l.begin(), l.end(), comp) {}

	self& operator =(std::initializer_list<value_type> l) {
		clear();
		insert(l.begin(), l.end());
		return *this;
	}
	~multiset() = default;

	MSTL_NODISCARD iterator begin() noexcept { return tree_.begin(); }
	MSTL_NODISCARD iterator end() noexcept { return tree_.end(); }
	MSTL_NODISCARD const_iterator cbegin() const noexcept { return tree_.cbegin(); }
	MSTL_NODISCARD const_iterator cend() const noexcept { return tree_.cend(); }
	MSTL_NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(tree_.begin()); }
	MSTL_NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(tree_.end()); }
	MSTL_NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(tree_.cbegin()); }
	MSTL_NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(tree_.cend()); }

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

	void swap(self& x) noexcept(noexcept(tree_.swap(x.tree_))) { tree_.swap(x.tree_); }

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
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename Compare = less<iter_val_t<Iterator>>,
	typename Alloc = allocator<iter_val_t<Iterator>>>
multiset(Iterator, Iterator, Compare = Compare(), Alloc = Alloc())
-> multiset<iter_val_t<Iterator>, Compare, Alloc>;

template <typename Key, typename Compare = less<Key>, typename Alloc = allocator<Key>>
multiset(std::initializer_list<Key>, Compare = Compare(), Alloc = Alloc()) -> multiset<Key, Compare, Alloc>;

template <typename Iterator, typename Alloc>
multiset(Iterator, Iterator, Alloc) -> multiset<iter_val_t<Iterator>, less<iter_val_t<Iterator>>, Alloc>;

template <typename Key, typename Alloc>
multiset(std::initializer_list<Key>, Alloc) -> multiset<Key, less<Key>, Alloc>;
#endif

template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator ==(
	const multiset<Key, Compare, Alloc>& lh,
	const multiset<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ == rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator !=(
	const multiset<Key, Compare, Alloc>& lh,
	const multiset<Key, Compare, Alloc>& rh) noexcept {
	return !(lh.tree_ == rh.tree_);
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator <(
	const multiset<Key, Compare, Alloc>& lh,
	const multiset<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ < rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator >(
	const multiset<Key, Compare, Alloc>& lh,
	const multiset<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ > rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator <=(
	const multiset<Key, Compare, Alloc>& lh,
	const multiset<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ <= rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
MSTL_NODISCARD bool operator >=(
	const multiset<Key, Compare, Alloc>& lh,
	const multiset<Key, Compare, Alloc>& rh) noexcept {
	return lh.tree_ >= rh.tree_;
}
template <typename Key, typename Compare, typename Alloc>
void swap(multiset<Key, Compare, Alloc>& lh, multiset<Key, Compare, Alloc>& rh)
noexcept(noexcept(lh.swap(rh))) {
	lh.swap(rh);
}

MSTL_END_NAMESPACE__
#endif // MSTL_SET_HPP__
