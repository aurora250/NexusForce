#ifndef MSTL_CORE_SERIALIZE_CONCEPTS_HPP__
#define MSTL_CORE_SERIALIZE_CONCEPTS_HPP__
#include "../typeinfo/concepts.hpp"
#include "../memory/memory_view.hpp"
MSTL_BEGIN_NAMESPACE__

template<typename T>
void serialize(string_serializer_tag, const T& obj, string& output);

template<typename T>
T deserialize(string_serializer_tag, string input);

template<typename T>
void serialize(binary_serializer_tag, const T& obj, vector<uint8_t>& output);

template<typename T>
T deserialize(binary_serializer_tag, span<const byte_t> input);

template<typename T>
void serialize(json_serializer_tag, const T& obj, json_ptr& output);

template<typename T>
T deserialize(json_serializer_tag, const json_ptr& input);


#ifdef MSTL_STANDARD_20__

template<typename T>
concept HasToString = requires(const T& obj) {
  { obj.to_string() } -> convertible_to<string>;
};

template<typename T>
concept HasParse = requires(string_view str) {
  { T::parse(str) } -> convertible_to<T>;
};

template<typename T>
concept StringSerializable = HasToString<T> && HasParse<T>;

template<typename T>
concept HasBinarySerialize = requires(const T& obj) {
  { serialize(binary_serializer_tag{}, obj) } -> convertible_to<vector<uint8_t>>;
};

template<typename T>
concept HasBinaryDeserialize = requires(span<const byte_t> data) {
  { deserialize<T>(binary_serializer_tag{}, data) } -> convertible_to<T>;
};

template<typename T>
concept BinarySerializable = HasBinarySerialize<T> && HasBinaryDeserialize<T>;

#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SERIALIZE_CONCEPTS_HPP__
