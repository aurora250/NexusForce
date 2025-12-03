#ifndef MSTL_CORE_SERIALIZE_SERIALIZE_TRAITS_HPP__
#define MSTL_CORE_SERIALIZE_SERIALIZE_TRAITS_HPP__
#include "concepts.hpp"
MSTL_BEGIN_NAMESPACE__

template<typename SerializerTag, typename T, typename = void>
struct serializer_traits;

#ifdef MSTL_STANDARD_20__

template<typename T>
struct serializer_traits<string_serializer_tag, T> {
  static string serialize(const T& obj) {
    if constexpr (HasToString<T>) {
      return obj.to_string();
    } else {
      static_assert(HasToString<T>, 
          "Type T must implement to_string() for string serialization");
    }
  }
    
  static T deserialize(string_view data) {
    if constexpr (HasParse<T>) {
      return T::parse(data);
    } else {
      static_assert(HasParse<T>,
          "Type T must implement static parse(string_view) for string deserialization");
    }
  }
    
  static constexpr bool is_supported = StringSerializable<T>;
};

template<typename T>
struct serializer_traits<binary_serializer_tag, T> {
  static vector<uint8_t> serialize(const T& obj) {
    if constexpr (requires { serialize(binary_serializer_tag{}, obj); }) {
      return serialize(binary_serializer_tag{}, obj);
    } else {
      static_assert(sizeof(T) == 0, 
          "Type T must support ADL serialize(binary_serializer_tag, const T&)");
    }
  }
    
  static T deserialize(span<const uint8_t> data) {
    if constexpr (requires { deserialize(binary_serializer_tag{}, data); }) {
      return deserialize(binary_serializer_tag{}, data);
    } else {
      static_assert(sizeof(T) == 0,
          "Type T must support ADL deserialize(binary_serializer_tag, span<const uint8_t>)");
    }
  }

  static constexpr bool is_supported = 
      requires(const T& obj) { serialize(binary_serializer_tag{}, obj); } &&
      requires(span<const uint8_t> data) { deserialize(binary_serializer_tag{}, data); };
};


template<typename T>
struct default_serializer {
  using type = conditional_t<
      StringSerializable<T>,
      string_serializer_tag,
      conditional_t<
          BinarySerializable<T>,
          binary_serializer_tag,
          void
      >
  >;

  static constexpr bool is_supported = !is_void_v<type>;
};

template<typename T>
using default_serializer_t = typename default_serializer<T>::type;

#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SERIALIZE_SERIALIZE_TRAITS_HPP__
