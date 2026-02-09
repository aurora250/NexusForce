#ifndef MSTL_CORE_CONTAINER_VECTOR_HPP__
#define MSTL_CORE_CONTAINER_VECTOR_HPP__
#include "MSTL/core/algorithm/compare.hpp"
#include "MSTL/core/interface/icollector.hpp"
#include "MSTL/core/interface/iiterator.hpp"
#include "MSTL/core/memory/standard_allocator.hpp"
#include "MSTL/core/memory/uninitialized.hpp"
#include "MSTL/core/utility/compressed_pair.hpp"
MSTL_BEGIN_NAMESPACE__

template <bool IsConst, typename Vector>
struct vector_iterator : iiterator<vector_iterator<IsConst, Vector>> {
public:
	using container_type	= Vector;
	using value_type		= typename container_type::value_type;
	using size_type			= typename container_type::size_type;
	using difference_type	= typename container_type::difference_type;
    using iterator_category = contiguous_iterator_tag;
	using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
	using pointer	= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;

private:
    pointer current_ = nullptr;
    const container_type* container_ = nullptr;

public:
	MSTL_CONSTEXPR20 vector_iterator() noexcept = default;
    MSTL_CONSTEXPR20 ~vector_iterator() = default;

	MSTL_CONSTEXPR20 vector_iterator(const vector_iterator&) noexcept = default;
	MSTL_CONSTEXPR20 vector_iterator& operator =(const vector_iterator&) noexcept = default;
	MSTL_CONSTEXPR20 vector_iterator(vector_iterator&&) noexcept = default;
	MSTL_CONSTEXPR20 vector_iterator& operator =(vector_iterator&&) noexcept = default;

    MSTL_CONSTEXPR20 vector_iterator(pointer ptr, const container_type* vec) noexcept
	: current_(ptr), container_(vec) {}

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference dereference() const noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        MSTL_DEBUG_VERIFY(
        	container_->start_ <= current_ && current_ < container_->finish_,
            "Attempting to dereference out of boundary");
        return *current_;
    }

    MSTL_CONSTEXPR20 void increment() noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        MSTL_DEBUG_VERIFY(current_ < container_->finish_, "Attempting to increment out of boundary");
        ++current_;
    }

    MSTL_CONSTEXPR20 void decrement() noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to decrement a null pointer");
        MSTL_DEBUG_VERIFY(container_->start_ < current_, "Attempting to decrement out of boundary");
        --current_;
    }

    MSTL_CONSTEXPR20 void advance(difference_type off) noexcept {
    	MSTL_DEBUG_VERIFY((current_ && container_) || off == 0, "Attempting to advance a null pointer");
    	MSTL_DEBUG_VERIFY(
    		(off < 0 ? off >= container_->start_ - current_ : off <= container_->finish_ - current_),
			"Attempting to advance out of boundary");
        current_ += off;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 difference_type distance_to(const vector_iterator& other) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == other.container_, "Attempting to distance to a different container");
        return static_cast<difference_type>(current_ - other.current_);
    }

	MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](difference_type off) noexcept {
		return *(*this + off);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool equal(const vector_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool less_than(const vector_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to less than a different container");
        return current_ < rhs.current_;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer base() const noexcept {
        return current_;
    }

	MSTL_NODISCARD MSTL_CONSTEXPR20 const container_type* container() const noexcept {
    	return container_;
    }
};


template <typename T, typename Alloc = allocator<T>>
class vector : public icollector<vector<T, Alloc>> {
	static_assert(is_object_v<T>, "vector only contains object types.");
	static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
	static_assert(is_same_v<T, typename Alloc::value_type>, "allocator type mismatch.");

public:
	using pointer			= T*;
	using reference			= T&;
	using const_pointer		= const T*;
	using const_reference	= const T&;
	using value_type		= T;
	using size_type			= size_t;
	using difference_type	= ptrdiff_t;
	using iterator					= vector_iterator<false, vector<T, Alloc>>;
	using const_iterator			= vector_iterator<true, vector<T, Alloc>>;
	using reverse_iterator          = _MSTL reverse_iterator<iterator>;
	using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;
	using allocator_type			= Alloc;

private:
	pointer start_ = nullptr;
	pointer finish_ = nullptr;
	compressed_pair<allocator_type, pointer> pair_{ default_construct_tag{}, nullptr };

	template <bool, typename> friend struct vector_iterator;

private:
	MSTL_CONSTEXPR20 void fill_initialize(size_type n, const T& x) {
		start_ = pair_.get_base().allocate(n);
		_MSTL uninitialized_fill_n(start_, n, x);
		finish_ = start_ + n;
		pair_.value = finish_;
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 pointer allocate_and_copy(size_type n, Iterator first, Iterator last) {
		MSTL_DEBUG_VERIFY(n < max_size(), "vector allocate out of allocate bounds.");
		pointer result = pair_.get_base().allocate(n);
		pointer finish = result;
		try {
			finish = _MSTL uninitialized_copy(first, last, result);
		} catch (...) {
			_MSTL destroy(result, finish);
			pair_.get_base().deallocate(result, n);
			throw;
		}
		return result;
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 pointer allocate_and_move(size_type n, Iterator first, Iterator last) {
		pointer result = pair_.get_base().allocate(n);
		pointer finish = result;
		try {
			finish = _MSTL uninitialized_move(first, last, result);
		} catch (...) {
			_MSTL destroy(result, finish);
			pair_.get_base().deallocate(result, n);
			throw;
		}
		return result;
	}

	template <typename Iterator, enable_if_t<
		is_iter_v<Iterator> && !is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void range_initialize(Iterator first, Iterator last) {
		for (; first != last; ++first) {
			vector::push_back(*first);
		}
	}

	template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void range_initialize(Iterator first, Iterator last) {
		size_type n = _MSTL distance(first, last);
		start_ = vector::allocate_and_copy(n, first, last);
		finish_ = start_ + n;
		pair_.value = finish_;
	}

	MSTL_CONSTEXPR20 void deallocate() {
		if (start_) pair_.get_base().deallocate(start_, pair_.value - start_);
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 void range_insert(iterator position, Iterator first, Iterator last) {
		if (first == last) return;
		const size_t n = _MSTL distance(first, last);
		if (static_cast<size_t>(pair_.value - finish_) >= n) {
			const auto elems_after = static_cast<size_t>(vector::end() - position);
			iterator old_finish = vector::end();
			if (elems_after > n) {
				_MSTL uninitialized_copy(finish_ - n, finish_, finish_);
				finish_ += n;
				_MSTL copy_backward(position, old_finish - n, old_finish);
				_MSTL copy(first, last, position);
			}
			else {
				Iterator mid = first;
				_MSTL advance(mid, elems_after);
				_MSTL uninitialized_copy(mid, last, finish_);
				finish_ += (n - elems_after);
				_MSTL uninitialized_move(position, old_finish, finish_);
				finish_ += elems_after;
				_MSTL copy(first, mid, position);
			}
		}
		else {
			const size_type old_size = vector::size();
			const size_type len = old_size + _MSTL max(old_size, n);
			pointer new_start = pair_.get_base().allocate(len);
			pointer new_finish = new_start;
			new_finish = _MSTL uninitialized_copy(vector::begin(), position, new_start);
			new_finish = _MSTL uninitialized_copy(first, last, new_finish);
			new_finish = _MSTL uninitialized_copy(position, vector::end(), new_finish);
			_MSTL destroy(start_, finish_);
			vector::deallocate();
			start_ = new_start;
			finish_ = new_finish;
			pair_.value = new_start + len;
		}
	}

	template <typename Iterator, enable_if_t<!is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void assign_aux(Iterator first, Iterator last) {
		pointer cur = start_;
		for (; first != last && cur != finish_; ++first, ++cur)
			*cur = *first;
		if (first == last)
			vector::erase(cur, finish_);
		else
			vector::insert(finish_, first, last);
	}

	template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void assign_aux(Iterator first, Iterator last) {
		const size_t n = _MSTL distance(first, last);
		if (n > vector::capacity()) {
			vector::clear();
			vector::range_insert(vector::begin(), first, last);
		}
		else if (n > size()) {
			Iterator mid = first;
			_MSTL advance(mid, vector::size());
			_MSTL copy(first, mid, vector::begin());
			finish_ = _MSTL uninitialized_copy(mid, last, finish_);
		}
		else {
			_MSTL copy(first, last, vector::begin());
			vector::erase(vector::begin() + n, vector::end());
		}
	}

public:
	MSTL_CONSTEXPR20 vector() {
		constexpr size_type init_cap = 1;
		pointer result = pair_.get_base().allocate(init_cap);
		finish_ = start_ = result;
		pair_.value = finish_ + init_cap;
	}

	MSTL_CONSTEXPR20 explicit vector(const size_type n) {
		start_ = pair_.get_base().allocate(n);
		finish_ = start_;
		try {
			for (size_type i = 0; i < n; ++i) {
				_MSTL construct(finish_);
				++finish_;
			}
		} catch (...) {
			_MSTL destroy(start_, finish_);
			pair_.get_base().deallocate(start_, n);
			throw;
		}
		pair_.value = start_ + n;
	}
	MSTL_CONSTEXPR20 explicit vector(const size_type n, const T& value) {
		vector::fill_initialize(n, value);
	}
	MSTL_CONSTEXPR20 explicit vector(const int16_t n, const T& value) {
		vector::fill_initialize(n, value);
	}
	MSTL_CONSTEXPR20 explicit vector(const int32_t n, const T& value) {
		vector::fill_initialize(n, value);
	}
	MSTL_CONSTEXPR20 explicit vector(const int64_t n, const T& value) {
		vector::fill_initialize(n, value);
	}

	MSTL_CONSTEXPR20 vector(const vector& x) {
		const size_type n = x.size();
		start_ = vector::allocate_and_copy(n, x.begin(), x.end());
		finish_ = start_ + n;
		pair_.value = finish_;
	}
	MSTL_CONSTEXPR20 vector& operator =(const vector& x) {
		if (_MSTL addressof(x) == this) return *this;
		vector::clear();
		vector::insert(vector::end(), x.cbegin(), x.cend());
		return *this;
	}

	MSTL_CONSTEXPR20 vector(vector&& x) noexcept {
		vector::swap(x);
	}
	MSTL_CONSTEXPR20 vector& operator =(vector&& x) noexcept {
		if (_MSTL addressof(x) == this) return *this;
		vector::clear();
		vector::swap(x);
		return *this;
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 vector(Iterator first, Iterator last) {
		MSTL_DEBUG_VERIFY(first <= last, "vector iterator-constructor out of ranges.");
		vector::range_initialize(first, last);
	}

	MSTL_CONSTEXPR20 vector(std::initializer_list<T> x)
		: vector(x.begin(), x.end()) {}

	MSTL_CONSTEXPR20 vector& operator =(std::initializer_list<T> x) {
		if (x.size() > vector::capacity()) {
			pointer new_ = vector::allocate_and_move(x.end() - x.begin(), x.begin(), x.end());
			_MSTL destroy(start_, finish_);
			vector::deallocate();
			start_ = new_;
			finish_ = start_ + x.size();
			pair_.value = start_ + x.size();
		}
		else if (size() >= x.size()) {
			iterator i = _MSTL copy(x.begin(), x.end(), vector::begin());
			_MSTL destroy(i.base(), finish_);
		}
		else {
			_MSTL copy(x.begin(), x.begin() + vector::size(), start_);
			_MSTL uninitialized_copy(x.begin() + vector::size(), x.end(), finish_);
		}
		finish_ = start_ + x.size();
		return *this;
	}

	MSTL_CONSTEXPR20 ~vector() {
		vector::clear();
		vector::deallocate();
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 iterator begin() noexcept { return iterator(start_, this); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator end() noexcept { return iterator(finish_, this); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator begin() const noexcept { return vector::cbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator end() const noexcept { return vector::cend(); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cbegin() const noexcept { return const_iterator(start_, this); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cend() const noexcept { return const_iterator(finish_, this); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rbegin() noexcept { return reverse_iterator(vector::end()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rend() noexcept { return reverse_iterator(vector::begin()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rbegin() const noexcept { return vector::crbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rend() const noexcept { return vector::crend(); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(vector::cend()); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crend() const noexcept { return const_reverse_iterator(vector::cbegin()); }

	MSTL_NODISCARD MSTL_CONSTEXPR20 size_type size() const noexcept {
		return static_cast<size_type>(finish_ - start_);
	}
	MSTL_NODISCARD static constexpr size_type max_size() noexcept {
		return static_cast<size_type>(-1) / sizeof(T);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 size_type capacity() const noexcept {
		return static_cast<size_type>(pair_.value - start_);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool empty() const noexcept {
		return start_ == finish_;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 pointer data() noexcept {
		MSTL_DEBUG_VERIFY(!empty(), "data called on empty vector");
		return start_;
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer data() const noexcept {
		MSTL_DEBUG_VERIFY(!empty(), "data called on empty vector");
		return start_;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 reference front() noexcept {
		MSTL_DEBUG_VERIFY(!empty(), "front called on empty vector");
		return *start_;
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference front() const noexcept {
		MSTL_DEBUG_VERIFY(!empty(), "front called on empty vector");
		return *start_;
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 reference back() noexcept {
		MSTL_DEBUG_VERIFY(!empty(), "back called on empty vector");
		return *(finish_ - 1);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference back() const noexcept {
		MSTL_DEBUG_VERIFY(!empty(), "back called on empty vector");
		return *(finish_ - 1);
	}

	MSTL_CONSTEXPR20 void reserve(const size_type n) {
		MSTL_DEBUG_VERIFY(n < max_size(), "vector reserve out of allocate bounds.");
		if (vector::capacity() >= n) return;

		size_type new_capacity = _MSTL max(vector::capacity() * 2, n);
		pointer new_start = pair_.get_base().allocate(new_capacity);
		pointer new_finish = new_start;

		try {
			new_finish = _MSTL uninitialized_move(start_, finish_, new_start);
		} catch (...) {
			_MSTL destroy(new_start, new_finish);
			pair_.get_base().deallocate(new_start, new_capacity);
			throw;
		}

		_MSTL destroy(start_, finish_);
		vector::deallocate();
		start_ = new_start;
		finish_ = new_finish;
		pair_.value = start_ + new_capacity;
	}

	MSTL_CONSTEXPR20 void resize(size_type new_size, const T& x) {
		if (new_size < vector::size()) {
			vector::erase(vector::begin() + new_size, vector::end());
		} else {
			vector::insert(vector::end(), new_size - vector::size(), x);
		}
	}
	MSTL_CONSTEXPR20 void resize(const size_type new_size) {
		vector::resize(new_size, T());
	}

	template <typename... Args>
	MSTL_CONSTEXPR20 void emplace(iterator position, Args&&... args) {
		if (finish_ != pair_.value) {
			_MSTL construct(finish_, _MSTL move(*(finish_ - 1)));
			++finish_;
			_MSTL move_backward(position, _MSTL prev(vector::end(), -2), _MSTL prev(vector::end()));
			_MSTL construct(&*position, _MSTL forward<Args>(args)...);
			return;
		}
		const size_type old_size = vector::size();
		const size_type len = old_size != 0 ? 2 * old_size : 1;
		pointer new_start = pair_.get_base().allocate(len);
		pointer new_finish = _MSTL uninitialized_move(begin(), position, new_start);
		_MSTL construct(new_finish, _MSTL forward<Args>(args)...);
		++new_finish;
		new_finish = _MSTL uninitialized_move(position, end(), new_finish);
		_MSTL destroy(begin(), end());
		vector::deallocate();
		start_ = new_start;
		finish_ = new_finish;
		pair_.value = new_start + len;
	}

	template <typename... Args>
	MSTL_CONSTEXPR20 void emplace_back(Args&&... args) {
		if (finish_ != pair_.value) {
			_MSTL construct(finish_, _MSTL forward<Args>(args)...);
			++finish_;
		}
		else vector::emplace(vector::end(), _MSTL forward<Args>(args)...);
	}

	MSTL_CONSTEXPR20 void push_back(const T& val) {
	    if (finish_ != pair_.value) {
	        _MSTL construct(finish_, val);
	        ++finish_;
	    }
	    else vector::emplace(vector::end(), val);
	}
	MSTL_CONSTEXPR20 void push_back(T&& val) {
		vector::emplace_back(_MSTL move(val));
	}

	MSTL_CONSTEXPR20 void pop_back() noexcept {
		MSTL_DEBUG_VERIFY(!empty(), "pop called in an empty vector")
		_MSTL destroy(finish_ - 1);
		--finish_;
	}

	MSTL_CONSTEXPR20 void assign(size_type n, const value_type& value) {
		if (n > vector::capacity()) {
			vector::clear();
			vector::reserve(n);
			vector::insert(vector::begin(), n, value);
		}
		else if (n > vector::size()) {
			_MSTL fill(vector::begin(), vector::end(), value);
			finish_ = _MSTL uninitialized_fill_n(finish_, n - vector::size(), value);
		}
		else {
			_MSTL fill_n(vector::begin(), n, value);
			vector::erase(vector::begin() + n, vector::end());
		}
	}

	template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void assign(Iterator first, Iterator last) {
		vector::assign_aux(first, last);
	}

	MSTL_CONSTEXPR20 void assign(std::initializer_list<value_type> l) {
		vector::assign(l.begin(), l.end());
	}

	MSTL_CONSTEXPR20 iterator insert(iterator position, const value_type& x) {
		size_type n = position - vector::begin();
		vector::emplace(position, x);
		return vector::begin() + n;
	}
	MSTL_CONSTEXPR20 iterator insert(iterator position, value_type&& x) {
		size_type n = position - vector::begin();
		vector::emplace(position, _MSTL move(x));
		return vector::begin() + n;
	}

	MSTL_CONSTEXPR20 iterator insert(iterator position) {
		return vector::insert(position, T());
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 void insert(iterator position, Iterator first, Iterator last) {
		MSTL_DEBUG_VERIFY(
			_MSTL distance(first, last) >= 0, "vector insert resource iterator out of ranges."
		);
		vector::range_insert(position, first, last);
	}

	MSTL_CONSTEXPR20 void insert(iterator position, std::initializer_list<value_type> l) {
		vector::range_insert(position, l.begin(), l.end());
	}
	MSTL_CONSTEXPR20 void insert(iterator position, size_type n, const value_type& x) {
		if (n == 0) return;
		if (static_cast<size_type>(pair_.value - finish_) >= n) {
			const size_type elems_after = _MSTL distance(vector::begin(), position);
			iterator old_finish = vector::end();
			if (elems_after > n) {
				_MSTL uninitialized_copy(finish_ - n, finish_, finish_);
				finish_ += n;
				_MSTL copy_backward(position, old_finish - n, old_finish);
				_MSTL fill(position, position + n, x);
			}
			else {
				_MSTL uninitialized_fill_n(finish_, n - elems_after, x);
				finish_ += n - elems_after;
				_MSTL uninitialized_move(position, old_finish, vector::end());
				finish_ += elems_after;
				_MSTL destroy(position, old_finish);
				_MSTL uninitialized_fill(position, old_finish, x);
			}
		}
		else {
			const size_type old_size = vector::size();
			const size_type len = old_size + _MSTL max(old_size, n);
			pointer new_start = pair_.get_base().allocate(len);
			pointer new_finish = _MSTL uninitialized_copy(vector::begin(), position, new_start);
			new_finish = _MSTL uninitialized_fill_n(new_finish, n, x);
			new_finish = _MSTL uninitialized_copy(position, vector::end(), new_finish);
			_MSTL destroy(start_, finish_);
			vector::deallocate();
			start_ = new_start;
			finish_ = new_finish;
			pair_.value = new_start + len;
		}
	}

	MSTL_CONSTEXPR20 iterator erase(iterator first, iterator last)
		noexcept(is_nothrow_move_assignable_v<value_type>) {
		MSTL_DEBUG_VERIFY(_MSTL distance(first, last) >= 0, "vector erase out of ranges.");

	    const auto elems_after = vector::end() - last;
	    if (elems_after > 0) {
	        _MSTL move_backward(last, vector::end(), first + elems_after);
	    }
	    pointer new_finish = finish_ - (last - first);
	    _MSTL destroy(new_finish, finish_);
	    finish_ = new_finish;
	    return first;
	}

	MSTL_CONSTEXPR20 iterator erase(iterator position)
		noexcept(is_nothrow_move_assignable_v<value_type>) {
		if (position + 1 != vector::end()) {
		    _MSTL move(position + 1, vector::end(), position);
		}
		--finish_;
		_MSTL destroy(finish_);
		return position;
	}

	MSTL_CONSTEXPR20 void shrink_to_fit() {
		if (vector::capacity() == vector::size()) return;
		if (vector::size() == 0) {
			vector::deallocate();
			start_ = finish_ = pair_.value = nullptr;
			return;
		}
		pointer new_start = pair_.get_base().allocate(vector::size());
		pointer new_finish = new_start;
		try {
			new_finish = _MSTL uninitialized_move(start_, finish_, new_start);
		} catch (...) {
			_MSTL destroy(new_start, new_finish);
			pair_.get_base().deallocate(new_start, vector::size());
			throw;
		}
		_MSTL destroy(start_, finish_);
		vector::deallocate();
		start_ = new_start;
		finish_ = new_finish;
		pair_.value = new_start + vector::size();
	}

	MSTL_CONSTEXPR20 void clear() noexcept {
		if (vector::empty()) return;
		_MSTL destroy(start_, finish_);
		finish_ = start_;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference at(const size_type position) const noexcept {
		MSTL_DEBUG_VERIFY(position < vector::size(), "vector access out of range");
		return *(start_ + position);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 reference at(const size_type position) noexcept {
		return const_cast<reference>(static_cast<const vector*>(this)->at(position));
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference operator [](const size_type position) const noexcept {
		return vector::at(position);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](const size_type position) noexcept {
		return vector::at(position);
	}

	MSTL_CONSTEXPR20 void swap(vector& x) noexcept {
		if (_MSTL addressof(x) == this) return;
		_MSTL swap(start_, x.start_);
		_MSTL swap(finish_, x.finish_);
		_MSTL swap(pair_, x.pair_);
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(const vector& rhs) const
	noexcept(noexcept(vector::size() == rhs.size() && _MSTL equal(vector::cbegin(), vector::cend(), rhs.cbegin()))) {
		return vector::size() == rhs.size() && _MSTL equal(vector::cbegin(), vector::cend(), rhs.cbegin());
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(const vector& rhs) const
	noexcept(noexcept(_MSTL lexicographical_compare(vector::cbegin(), vector::cend(), rhs.cbegin(), rhs.cend()))) {
		return _MSTL lexicographical_compare(vector::cbegin(), vector::cend(), rhs.cbegin(), rhs.cend());
	}
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T, typename Alloc>
vector(T, Alloc = Alloc()) -> vector<T, Alloc>;

template <typename Iterator, typename Alloc>
vector(Iterator, Iterator, Alloc = Alloc()) -> vector<iter_value_t<Iterator>, Alloc>;
#endif

using bvector = vector<byte_t>;


MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_VECTOR_HPP__
