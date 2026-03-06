#include <NeForce/core/async/hazard_ptr.hpp>
NEFORCE_BEGIN_NAMESPACE__

thread_local retire_list hazard_pointer_domain::tl_retire_list_;

NEFORCE_END_NAMESPACE__
