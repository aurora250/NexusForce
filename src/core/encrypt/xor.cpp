#include <NeForce/core/encrypt/xor.hpp>
NEFORCE_BEGIN_NAMESPACE__

byte_vector XOR::encrypt(const cbyte_view data, const cbyte_view key) {
    if (key.empty()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Key cannot be empty"));
    }

    byte_vector result;
    result.reserve(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        result.push_back(data[i] ^ key[i % key.size()]);
    }
    return result;
}

NEFORCE_END_NAMESPACE__
