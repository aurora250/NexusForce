#include <NeForce/core/numeric/random/lcg.hpp>
#include <NeForce/core/time/datetime.hpp>
NEFORCE_BEGIN_NAMESPACE__

random_lcd::random_lcd() noexcept :
seed_(static_cast<seed_type>(timestamp::now().value())) {}

NEFORCE_END_NAMESPACE__
