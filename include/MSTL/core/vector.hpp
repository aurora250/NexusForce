#ifndef MSTL_VECTOR_HPP__
#define MSTL_VECTOR_HPP__
#include "serialize.hpp"
#include "undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

template <bool IsConst, typename Vector>
struct vector_iterator {
private:
	using self				= vector_iterator;
	using iterator			= vector_iterator<false, Vector>;
	using const_iterator	= vector_iterator<true, Vector>;

public:
	using iterator_category =
#ifdef MSTL_VERSION_20__
		contiguous_iterator_tag;
#else
		random_access_iterator_tag;
#endif
	using value_type		= typename Vector::value_type;
	using reference			= conditional_t<IsConst, typename Vector::const_reference, typename Vector::reference>;
	using pointer			= conditional_t<IsConst, typename Vector::const_pointer, typename Vector::pointer>;
	using difference_type	= typename Vector::difference_type;
	using size_type			= typename Vector::size_type;

private:
	pointer ptr_ = nullptr;
	const Vector* vec_ = nullptr;

	template <bool, typename> friend struct vector_iterator;
	template <typename, typename> friend class vector;

public:
	MSTL_CONSTEXPR20 vector_iterator() = default;
	MSTL_CONSTEXPR20 vector_iterator(pointer ptr, const Vector* vec) : ptr_(ptr), vec_(vec) {}

	MSTL_CONSTEXPR20 vector_iterator(const iterator& x) noexcept
	: ptr_(const_cast<pointer>(x.ptr_)), vec_(x.vec_) {}

	MSTL_CONSTEXPR20 self& operator =(const iterator& rh) noexcept {
		if (_MSTL addressof(rh) == this) return *this;
		ptr_ = const_cast<pointer>(rh.ptr_);
		vec_ = rh.vec_;
		return *this;
	}

	MSTL_CONSTEXPR20 vector_iterator(iterator&& x) noexcept
	: ptr_(const_cast<pointer>(x.ptr_)), vec_(x.vec_) {
		x.ptr_ = nullptr;
		x.vec_ = nullptr;
	}

	MSTL_CONSTEXPR20 self& operator =(iterator&& rh) noexcept {
		if (_MSTL addressof(rh) == this) return *this;
		ptr_ = const_cast<pointer>(rh.ptr_);
		vec_ = rh.vec_;
		rh.ptr_ = nullptr;
		rh.vec_ = nullptr;
		return *this;
	}

	MSTL_CONSTEXPR20 vector_iterator(const const_iterator& x) noexcept
	: ptr_(const_cast<pointer>(x.ptr_)), vec_(x.vec_) {}

	MSTL_CONSTEXPR20 self& operator =(const const_iterator& rh) noexcept {
		if (_MSTL addressof(rh) == this) return *this;
		ptr_ = const_cast<pointer>(rh.ptr_);
		vec_ = rh.vec_;
		return *this;
	}

	MSTL_CONSTEXPR20 vector_iterator(const_iterator&& x) noexcept
	: ptr_(const_cast<pointer>(x.ptr_)), vec_(x.vec_) {
		x.ptr_ = nullptr;
		x.vec_ = nullptr;
	}

	MSTL_CONSTEXPR20 self& operator =(const_iterator&& rh) noexcept {
		if (_MSTL addressof(rh) == this) return *this;
		ptr_ = const_cast<pointer>(rh.ptr_);
		vec_ = rh.vec_;
		rh.ptr_ = nullptr;
		rh.vec_ = nullptr;
		return *this;
	}

	MSTL_CONSTEXPR20 ~vector_iterator() noexcept = default;

	MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator *() const noexcept {
		MSTL_DEBUG_VERIFY(ptr_ && vec_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(vector_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
		MSTL_DEBUG_VERIFY(vec_->start_ <= ptr_ && ptr_ <= vec_->finish_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(vector_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
		return *ptr_;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 pointer operator ->() const noexcept {
		return &operator*();
	}

	MSTL_CONSTEXPR20 self& operator ++() noexcept {
		MSTL_DEBUG_VERIFY(ptr_ && vec_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(vector_iterator, __MSTL_DEBUG_TAG_INCREMENT));
		MSTL_DEBUG_VERIFY(ptr_ < vec_->finish_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(vector_iterator, __MSTL_DEBUG_TAG_INCREMENT));
		++ptr_;
		return *this;
	}

	MSTL_CONSTEXPR20 self operator ++(int) noexcept {
		self temp = *this;
		++*this;
		return temp;
	}

	MSTL_CONSTEXPR20 self& operator --() noexcept {
		MSTL_DEBUG_VERIFY(ptr_ && vec_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(vector_iterator, __MSTL_DEBUG_TAG_DECREMENT));
		MSTL_DEBUG_VERIFY(vec_->start_ < ptr_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(vector_iterator, __MSTL_DEBUG_TAG_DECREMENT));
		--ptr_;
		return *this;
	}

	MSTL_CONSTEXPR20 self operator --(int) noexcept {
		self temp = *this;
		--*this;
		return temp;
	}

	MSTL_CONSTEXPR20 self& operator +=(difference_type n) noexcept {
		if (n < 0) {
			MSTL_DEBUG_VERIFY((ptr_ && vec_) || n == 0, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(vector_iterator, __MSTL_DEBUG_TAG_DECREMENT));
			MSTL_DEBUG_VERIFY(n >= vec_->start_ - ptr_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(vector_iterator, __MSTL_DEBUG_TAG_DECREMENT));
		}
		else if (n > 0) {
			MSTL_DEBUG_VERIFY((ptr_ && vec_) || n == 0, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(vector_iterator, __MSTL_DEBUG_TAG_INCREMENT));
			MSTL_DEBUG_VERIFY(n <= vec_->finish_ - ptr_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(vector_iterator, __MSTL_DEBUG_TAG_INCREMENT));
		}
		ptr_ += n;
		return *this;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 self operator +(difference_type n) const noexcept {
		auto tmp = *this;
		tmp += n;
		return tmp;
	}

	MSTL_NODISCARD friend MSTL_CONSTEXPR20 self operator +(difference_type n, const self& it) {
		return it + n;
	}

	MSTL_CONSTEXPR20 self& operator -=(difference_type n) noexcept {
		ptr_ += -n;
		return *this;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 self operator -(difference_type n) const noexcept {
		auto tmp = *this;
		tmp -= n;
		return tmp;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 difference_type operator -(const self& x) const noexcept {
		MSTL_DEBUG_VERIFY(vec_ == x.vec_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(vector_iterator));
		return static_cast<difference_type>(ptr_ - x.ptr_);
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](difference_type n) noexcept {
		return *(*this + n);
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(const self& x) const noexcept {
		MSTL_DEBUG_VERIFY(vec_ == x.vec_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(vector_iterator));
		return ptr_ == x.ptr_;
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(const self& x) const noexcept {
		return !(*this == x);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(const self& x) const noexcept {
		MSTL_DEBUG_VERIFY(vec_ == x.vec_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(vector_iterator));
		return ptr_ < x.ptr_;
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(const self& x) const noexcept {
		return x < *this;
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(const self& x) const noexcept {
		return !(*this > x);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(const self& x) const noexcept {
		return !(*this < x);
	}
};

template <typename T, typename Alloc = allocator<T>>
class vector : public icollector<vector<T, Alloc>> {
#ifdef MSTL_VERSION_20__
	static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
	static_assert(is_same_v<T, typename Alloc::value_type>, "allocator type mismatch.");
	static_assert(is_object_v<T>, "vector only contains object types.");

	using self = vector<T, Alloc>;
	using super = icollector<self>;

public:
	MSTL_BUILD_TYPE_ALIAS(T)
	using iterator					= vector_iterator<false, vector<T, Alloc>>;
	using const_iterator			= vector_iterator<true, vector<T, Alloc>>;
	using reverse_iterator          = _MSTL reverse_iterator<iterator>;
	using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;
	using allocator_type			= Alloc;

private:
	pointer start_ = nullptr;
	pointer finish_ = nullptr;
	compressed_pair<allocator_type, pointer> pair_{ _MSTL_TAG default_construct_tag{}, nullptr };

	template <bool, typename> friend struct vector_iterator;

private:
	MSTL_CONSTEXPR20 pointer allocate_and_fill(size_type n, T&& x) {
		pointer result = pair_.get_base().allocate(n);
		_MSTL uninitialized_fill_n(result, n, _MSTL forward<T>(x));
		return result;
	}

	MSTL_CONSTEXPR20 void fill_initialize(size_type n, T&& x) {
		start_ = this->allocate_and_fill(n, _MSTL forward<T>(x));
		finish_ = start_ + n;
		pair_.value = finish_;
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 pointer allocate_and_copy(size_type n, Iterator first, Iterator last) {
		pointer result = pair_.get_base().allocate(n);
			_MSTL uninitialized_copy(first, last, result);
		return result;
	}

	template <typename Iterator, enable_if_t<
		is_iter_v<Iterator> && !is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void range_initialize(Iterator first, Iterator last) {
		for (; first != last; ++first)
			this->push_back(*first);
	}

	template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void range_initialize(Iterator first, Iterator last) {
		size_type n = _MSTL distance(first, last);
		start_ = this->allocate_and_copy(n, first, last);
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
			const auto elems_after = static_cast<size_t>(this->end() - position);
			iterator old_finish = this->end();
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
				_MSTL uninitialized_copy(position, old_finish, finish_);
				finish_ += elems_after;
				_MSTL copy(first, mid, position);
			}
		}
		else {
			const size_type old_size = this->size();
			const size_type len = old_size + _MSTL max(old_size, n);
			pointer new_start = pair_.get_base().allocate(len);
			pointer new_finish = new_start;
			new_finish = _MSTL uninitialized_copy(this->begin(), position, new_start);
			new_finish = _MSTL uninitialized_copy(first, last, new_finish);
			new_finish = _MSTL uninitialized_copy(position, this->end(), new_finish);
			_MSTL destroy(start_, finish_);
			this->deallocate();
			start_ = new_start;
			finish_ = new_finish;
			pair_.value = new_start + len;
		}
	}

	template <typename Iterator, enable_if_t<!is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void assign_aux(Iterator first, Iterator last) {
		const size_t n = _MSTL distance(first, last);
		pointer cur = start_;
		for (; first != last && cur != finish_; ++first, ++cur)
			*cur = *first;
		if (first == last)
			this->erase(cur, finish_);
		else
			this->insert(finish_, first, last);
	}

	template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void assign_aux(Iterator first, Iterator last) {
		const size_t n = _MSTL distance(first, last);
		if (n > this->capacity()) {
			this->clear();
			this->range_insert(this->begin(), first, last);
		}
		else if (n > size()) {
			Iterator mid = first;
			_MSTL advance(mid, this->size());
			_MSTL copy(first, mid, this->begin());
			finish_ = _MSTL uninitialized_copy(mid, last, finish_);
		}
		else {
			_MSTL copy(first, last, this->begin());
			this->erase(this->begin() + n, this->end());
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
		this->fill_initialize(n, _MSTL move(T()));
	}
	MSTL_CONSTEXPR20 explicit vector(const size_type n, const T& value) {
		this->fill_initialize(n, value);
	}
	MSTL_CONSTEXPR20 explicit vector(const int n, const T& value) {
		this->fill_initialize(n, value);
	}
	MSTL_CONSTEXPR20 explicit vector(const long n, const T& value) {
		this->fill_initialize(n, value);
	}
	MSTL_CONSTEXPR20 explicit vector(const size_type n, T&& value) {
		this->fill_initialize(n, _MSTL forward<T>(value));
	}
	MSTL_CONSTEXPR20 explicit vector(const int n, T&& value) {
		this->fill_initialize(n, _MSTL forward<T>(value));
	}
	MSTL_CONSTEXPR20 explicit vector(const long n, T&& value) {
		this->fill_initialize(n, _MSTL forward<T>(value));
	}

	MSTL_CONSTEXPR20 vector(const self& x) {
		start_ = this->allocate_and_copy(x.cend() - x.cbegin(), x.cbegin(), x.cend());
		finish_ = start_ + (x.cend() - x.cbegin());
		pair_.value = finish_;
	}
	MSTL_CONSTEXPR20 self& operator =(const self& x) {
		if (_MSTL addressof(x) == this) return *this;
		this->clear();
		this->insert(this->end(), x.cbegin(), x.cend());
		return *this;
	}

	MSTL_CONSTEXPR20 vector(self&& x) noexcept {
		this->swap(x);
	}
	MSTL_CONSTEXPR20 self& operator =(self&& x) noexcept {
		if (_MSTL addressof(x) == this) return *this;
		this->clear();
		this->swap(x);
		return *this;
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 vector(Iterator first, Iterator last) {
		MSTL_DEBUG_VERIFY(first <= last, "vector iterator-constructor out of ranges.");
		this->range_initialize(first, last);
	}

	MSTL_CONSTEXPR20 vector(std::initializer_list<T> x)
		: vector(x.begin(), x.end()) {}

	MSTL_CONSTEXPR20 self& operator =(std::initializer_list<T> x) {
		if (x.size() > this->capacity()) {
			iterator new_ = this->allocate_and_copy(x.end() - x.begin(), x.begin(), x.end());
			_MSTL destroy(start_, finish_);
			this->deallocate();
			start_ = new_;
			pair_.value = start_ + (x.end() - x.begin());
		}
		else if (size() >= x.size()) {
			iterator i = _MSTL copy(x.begin(), x.end(), this->begin());
			_MSTL destroy(i, finish_);
		}
		else {
			_MSTL copy(x.begin(), x.begin() + this->size(), start_);
			_MSTL uninitialized_copy(x.begin() + this->size(), x.end(), finish_);
		}
		finish_ = start_ + x.size();
		return *this;
	}

	MSTL_CONSTEXPR20 ~vector() {
		this->clear();
		this->deallocate();
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 iterator begin() noexcept { return iterator(start_, this); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator end() noexcept { return iterator(finish_, this); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator begin() const noexcept { return this->cbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator end() const noexcept { return this->cend(); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cbegin() const noexcept { return const_iterator(start_, this); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cend() const noexcept { return const_iterator(finish_, this); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rbegin() noexcept { return reverse_iterator(this->end()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rend() noexcept { return reverse_iterator(this->begin()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rbegin() const noexcept { return this->crbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rend() const noexcept { return this->crend(); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(this->cend()); }
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crend() const noexcept { return const_reverse_iterator(this->cbegin()); }

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

	MSTL_NODISCARD MSTL_CONSTEXPR20 allocator_type get_allocator() const noexcept { return allocator_type(); }

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
		if (this->capacity() >= n) return;
		size_type new_capacity = _MSTL max(this->capacity() * 2, n);
		size_type old_size = this->size();
		pointer tmp = this->allocate_and_copy(new_capacity, start_, finish_);
		_MSTL destroy(start_, finish_);
		this->deallocate();
		start_ = tmp;
		finish_ = tmp + old_size;
		pair_.value = start_ + new_capacity;
	}

	MSTL_CONSTEXPR20 void resize(size_type new_size, const T& x) {
		if (new_size < this->size()) this->erase(this->begin() + new_size, this->end());
		else this->insert(this->end(), new_size - this->size(), x);
	}
	MSTL_CONSTEXPR20 void resize(const size_type new_size) {
		this->resize(new_size, T());
	}

	template <typename... U>
	MSTL_CONSTEXPR20 void emplace(iterator position, U&&... args) {
		if (finish_ != pair_.value) {
			_MSTL construct(finish_, _MSTL move(*(finish_ - 1)));
			++finish_;
			_MSTL move_backward(position, _MSTL prev(this->end(), -2), _MSTL prev(this->end()));
			_MSTL construct(&*position, _MSTL forward<U>(args)...);
			return;
		}
		const size_type old_size = this->size();
		const size_type len = old_size != 0 ? 2 * old_size : 1;
		pointer new_start = pair_.get_base().allocate(len);
		pointer new_finish = _MSTL uninitialized_move(begin(), position, new_start);
		_MSTL construct(new_finish, _MSTL forward<U>(args)...);
		++new_finish;
		new_finish = _MSTL uninitialized_move(position, end(), new_finish);
		_MSTL destroy(begin(), end());
		deallocate();
		start_ = new_start;
		finish_ = new_finish;
		pair_.value = new_start + len;
	}

	template <typename... U>
	MSTL_CONSTEXPR20 void emplace_back(U&&... args) {
		if (finish_ != pair_.value) {
			_MSTL construct(finish_, _MSTL forward<U>(args)...);
			++finish_;
		}
		else this->emplace(this->end(), _MSTL forward<U>(args)...);
	}

	MSTL_CONSTEXPR20 void push_back(const T& val) {
	    if (finish_ != pair_.value) {
	        _MSTL construct(finish_, val);
	        ++finish_;
	    }
	    else this->emplace(this->end(), val);
	}
	MSTL_CONSTEXPR20 void push_back(T&& val) {
		this->emplace_back(_MSTL move(val));
	}

	MSTL_CONSTEXPR20 void pop_back() noexcept {
		_MSTL destroy(finish_);
		--finish_;
	}

	MSTL_CONSTEXPR20 void assign(size_type n, const value_type& value) {
		if (n > this->capacity()) {
			this->reserve(n);
			this->insert(this->begin(), n, value);
		}
		else if (n > this->size()) {
			_MSTL fill(this->begin(), this->end(), value);
			finish_ = _MSTL uninitialized_fill_n(finish_, n - this->size(), value);
		}
		else {
			_MSTL fill_n(this->begin(), n, value);
			this->erase(this->begin() + n, this->end());
		}
	}

	template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
	MSTL_CONSTEXPR20 void assign(Iterator first, Iterator last) {
		this->assign_aux(first, last);
	}

	MSTL_CONSTEXPR20 void assign(std::initializer_list<value_type> l) {
		this->assign(l.begin(), l.end());
	}

	MSTL_CONSTEXPR20 iterator insert(iterator position, const value_type& x) {
		size_type n = position - this->begin();
		this->emplace(position, x);
		return this->begin() + n;
	}
	MSTL_CONSTEXPR20 iterator insert(iterator position, value_type&& x) {
		size_type n = position - this->begin();
		this->emplace(position, _MSTL move(x));
		return this->begin() + n;
	}

	MSTL_CONSTEXPR20 iterator insert(iterator position) {
		return this->insert(position, T());
	}

	template <typename Iterator>
	MSTL_CONSTEXPR20 void insert(iterator position, Iterator first, Iterator last) {
		MSTL_DEBUG_VERIFY(
			_MSTL distance(first, last) >= 0, "vector insert resource iterator out of ranges."
		);
		this->range_insert(position, first, last);
	}

	MSTL_CONSTEXPR20 void insert(iterator position, std::initializer_list<value_type> l) {
		this->range_insert(position, l.begin(), l.end());
	}
	MSTL_CONSTEXPR20 void insert(iterator position, size_type n, const value_type& x) {
		if (n == 0) return;
		if (static_cast<size_type>(pair_.value - finish_) >= n) {
			const size_type elems_after = _MSTL distance(this->begin(), position);
			iterator old_finish = this->end();
			if (elems_after > n) {
				_MSTL uninitialized_copy(finish_ - n, finish_, finish_);
				finish_ += n;
				_MSTL copy_backward(position, old_finish - n, old_finish);
				_MSTL fill(position, position + n, x);
			}
			else {
				_MSTL uninitialized_fill_n(finish_, n - elems_after, x);
				finish_ += n - elems_after;
				_MSTL uninitialized_copy(position, old_finish, this->end());
				finish_ += elems_after;
				_MSTL fill(position, old_finish, x);
			}
		}
		else {
			const size_type old_size = this->size();
			const size_type len = old_size + _MSTL max(old_size, n);
			pointer new_start = pair_.get_base().allocate(len);
			pointer new_finish = _MSTL uninitialized_copy(this->begin(), position, new_start);
			new_finish = _MSTL uninitialized_fill_n(new_finish, n, x);
			new_finish = _MSTL uninitialized_copy(position, this->end(), new_finish);
			_MSTL destroy(start_, finish_);
			this->deallocate();
			start_ = new_start;
			finish_ = new_finish;
			pair_.value = new_start + len;
		}
	}

	MSTL_CONSTEXPR20 iterator erase(iterator first, iterator last)
		noexcept(is_nothrow_move_assignable_v<value_type>) {
		MSTL_DEBUG_VERIFY(_MSTL distance(first, last) >= 0, "vector erase out of ranges.");

	    const auto elems_after = this->end() - last;
	    if (elems_after > 0) {
	        _MSTL move(last, this->end(), first);
	    }
	    pointer new_finish = finish_ - (last - first);
	    _MSTL destroy(new_finish, finish_);
	    finish_ = new_finish;
	    return first;
	}

	MSTL_CONSTEXPR20 iterator erase(iterator position)
		noexcept(is_nothrow_move_assignable_v<value_type>) {
		if (position + 1 != this->end())
			_MSTL copy(position + 1, this->end(), position);
		--finish_;
		_MSTL destroy(finish_);
		return position;
	}

	MSTL_CONSTEXPR20 void shrink_to_fit() {
		if (this->capacity() == this->size()) return;
		if (this->size() == 0) {
			this->deallocate();
			start_ = finish_ = pair_.value = nullptr;
			return;
		}
		pointer new_start = pair_.get_base().allocate(this->size());
		pointer new_finish = _MSTL uninitialized_copy(start_, finish_, new_start);
		_MSTL destroy(start_, finish_);
		this->deallocate();
		start_ = new_start;
		finish_ = new_finish;
		pair_.value = new_start + this->size();
	}

	MSTL_CONSTEXPR20 void clear() noexcept {
		if (this->empty()) return;
		_MSTL destroy(start_, finish_);
		finish_ = start_;
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference at(const size_type position) const noexcept {
		MSTL_DEBUG_VERIFY(position < this->size(), "vector access out of range");
		return *(start_ + position);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 reference at(const size_type position) noexcept {
		return const_cast<reference>(static_cast<const self*>(this)->at(position));
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference operator [](const size_type position) const noexcept {
		return this->at(position);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](const size_type position) noexcept {
		return this->at(position);
	}

	MSTL_CONSTEXPR20 void swap(self& x) noexcept {
		if (_MSTL addressof(x) == this) return;
		_MSTL swap(start_, x.start_);
		_MSTL swap(finish_, x.finish_);
		_MSTL swap(pair_, x.pair_);
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(const self& rh) const
	noexcept(noexcept(this->size() == rh.size() && _MSTL equal(this->cbegin(), this->cend(), rh.cbegin()))) {
		return this->size() == rh.size() && _MSTL equal(this->cbegin(), this->cend(), rh.cbegin());
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(const self& rh) const
	noexcept(noexcept(!(*this == rh))) {
		return !(*this == rh);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(const self& rh) const
	noexcept(noexcept(_MSTL lexicographical_compare(this->cbegin(), this->cend(), rh.cbegin(), rh.cend()))) {
		return _MSTL lexicographical_compare(this->cbegin(), this->cend(), rh.cbegin(), rh.cend());
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(const self& rh) const
	noexcept(noexcept(rh < *this)) {
		return rh < *this;
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(const self& rh) const
	noexcept(noexcept(!(*this < rh))) {
		return !(*this < rh);
	}
	MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(const self& rh) const
	noexcept(noexcept(!(*this > rh))) {
		return !(*this > rh);
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 size_type to_hash() const noexcept {
		return super::default_to_hash(*this);
	}

	MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
		return super::default_to_string(*this);
	}
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T, typename Alloc>
vector(T, Alloc = Alloc()) -> vector<T, Alloc>;

template <typename Iterator, typename Alloc>
vector(Iterator, Iterator, Alloc = Alloc()) -> vector<iter_val_t<Iterator>, Alloc>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_VECTOR_HPP__
