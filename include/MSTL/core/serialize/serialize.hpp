#ifndef MSTL_CORE_SERIALIZE_SERIALIZE_HPP__
#define MSTL_CORE_SERIALIZE_SERIALIZE_HPP__
#include "../container/vector.hpp"
#include "serialize_traits.hpp"
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_STANDARD_20__

template<typename SerializerTag = void, typename T>
constexpr auto serialize(const T& obj) {
    using tag = conditional_t<
        is_void_v<SerializerTag>,
        default_serializer_t<T>,
        SerializerTag
    >;

    static_assert(!is_void_v<tag>,
        "No default serializer found for type T and no explicit serializer specified");

    return serializer_traits<tag, T>::serialize(obj);
}

template<typename T, typename SerializerTag = void>
constexpr T deserialize(auto&& data) {
    using tag = conditional_t<
        is_void_v<SerializerTag>,
        default_serializer_t<T>,
        SerializerTag
    >;

    static_assert(!is_void_v<tag>,
        "No default serializer found for type T and no explicit serializer specified");

    return serializer_traits<tag, T>::deserialize(forward<decltype(data)>(data));
}


// template<typename T>
// string to_string(const T& obj) {
//     return serialize<string_serializer_tag>(obj);
// }

template<typename T>
T from_string(string_view str) {
    return deserialize<T, string_serializer_tag>(str);
}

template<typename T>
vector<uint8_t> to_binary(const T& obj) {
    return serialize<binary_serializer_tag>(obj);
}

template<typename T>
T from_binary(span<const uint8_t> data) {
    return deserialize<T, binary_serializer_tag>(data);
}

template<typename T>
string to_json(const T& obj) {
    return serialize<json_serializer_tag>(obj);
}

template<typename T>
T from_json(string_view json_str) {
    return deserialize<T, json_serializer_tag>(json_str);
}

#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SERIALIZE_SERIALIZE_HPP__
