#ifndef MSTL_CORE_CONTAINER_BLOOM_FILTER_HPP__
#define MSTL_CORE_CONTAINER_BLOOM_FILTER_HPP__
#include "MSTL/core/container/bitmap.hpp"
#include "MSTL/core/numeric/math.hpp"
MSTL_BEGIN_NAMESPACE__

template<typename T, typename Hash = hash<T>>
class bloom_filter {
private:
    size_t m_;
    size_t k_;
    bitmap bits_;
    Hash hasher_;

    static size_t compute_m(const size_t n, const double p) noexcept {
        const double ln2 = _MSTL logarithm_e(2.);
        const double m = - static_cast<double>(n) * _MSTL logarithm_e(p) / (ln2 * ln2);
        return static_cast<size_t>(_MSTL ceil(m));
    }

    static size_t compute_k(const size_t n, const size_t m) noexcept {
        const double k = (static_cast<double>(m) / n) * _MSTL logarithm_e(2.);
        return static_cast<size_t>(_MSTL max(static_cast<decimal_t>(1), _MSTL round(k)));
    }

    static size_t rotl(const size_t x, const unsigned int shift) noexcept {
        constexpr unsigned int bits = sizeof(size_t) * 8;
        return (x << shift) | (x >> (bits - shift));
    }

    pair<size_t, size_t> hash_values(const T& key) const noexcept {
        size_t h1 = hasher_(key);
        size_t h2 = rotl(h1, 17);
        if (h2 == 0) h2 = 1;
        return {h1, h2};
    }

    size_t nth_hash(const size_t i, const size_t h1, const size_t h2) const noexcept {
        return (h1 + i * h2) % m_;
    }

public:
    bloom_filter(const bloom_filter&) noexcept = default;
    bloom_filter(bloom_filter&&) noexcept = default;
    bloom_filter& operator =(const bloom_filter&) = default;
    bloom_filter& operator =(bloom_filter&&) = default;
    ~bloom_filter() = default;

    bloom_filter(const size_t expected_insertions, const double false_positive_prob)
    : m_(compute_m(expected_insertions, false_positive_prob)),
      k_(compute_k(expected_insertions, m_)), bits_(m_, false), hasher_(Hash()) {
        if (expected_insertions == 0 || false_positive_prob <= 0.0 || false_positive_prob >= 1.0) {
            throw_exception(value_exception("expected_insertions must be positive and false_positive_prob in (0,1)"));
        }
    }

    bloom_filter(const size_t m, const size_t k)
    : m_(m), k_(k), bits_(m_, false), hasher_(Hash()) {
        if (m == 0 || k == 0) {
            throw_exception(value_exception("m and k must be positive"));
        }
    }

    void insert(const T& key) noexcept {
        auto h = hash_values(key);
        for (size_t i = 0; i < k_; ++i) {
            const size_t index = bloom_filter::nth_hash(i, h.first, h.second);
            bits_[index] = true;
        }
    }

    bool contains(const T& key) const noexcept {
        auto h = hash_values(key);
        for (size_t i = 0; i < k_; ++i) {
            const size_t index = bloom_filter::nth_hash(i, h.first, h.second);
            if (!bits_[index]) return false;
        }
        return true;
    }

    void clear() noexcept {
        _MSTL fill(bits_.begin(), bits_.end(), false);
    }

    size_t bit_size() const noexcept {
        return m_;
    }

    size_t hash_count() const noexcept {
        return k_;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_BLOOM_FILTER_HPP__
