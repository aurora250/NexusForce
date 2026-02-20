#include <MSTL/core/async/hazard_ptr.hpp>
MSTL_BEGIN_NAMESPACE__

thread_local retire_list hazard_pointer_domain::tl_retire_list_;

MSTL_END_NAMESPACE__
