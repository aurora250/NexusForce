#include <NeForce/core/string/basic_string.hpp>
NEFORCE_BEGIN_NAMESPACE__

template class basic_string_view<char>;
template class basic_string_view<wchar_t>;
#ifdef NEFORCE_STANDARD_20
template class basic_string_view<char8_t>;
#endif
template class basic_string_view<char16_t>;
template class basic_string_view<char32_t>;


template class basic_string<char>;
template class basic_string<wchar_t>;
#ifdef NEFORCE_STANDARD_20
template class basic_string<char8_t>;
#endif
template class basic_string<char16_t>;
template class basic_string<char32_t>;

NEFORCE_END_NAMESPACE__
