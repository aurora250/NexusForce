#ifndef NEFORCE_CORE_STRING_LEXICAL_CAST_HPP__
#define NEFORCE_CORE_STRING_LEXICAL_CAST_HPP__
#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename To, typename From, typename Dummy = void>
struct lexical_caster;


template <typename To, typename From>
To lexical_cast(const From& from) {
    return lexical_caster<To, From>::cast(from);
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_LEXICAL_CAST_HPP__
