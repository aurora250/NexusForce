#include <NeForce/core/reflect/registry.hpp>
#include <NeForce/core/async/signals.hpp>

NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

bool registry::connect_signal_to_slot(signal_base* sig, const meta_function* slot, void* receiver) {
    if ((sig == nullptr) || (slot == nullptr) || (receiver == nullptr)) {
        return false;
    }

    sig->connect_dynamic([slot, receiver](const vector<meta_any>& args) { slot->invoke(receiver, args); });
    return true;
}

registry& registry::instance() {
    static registry inst;
    return inst;
}

void meta_type::resolve_bases(registry* reg) {
    for (auto& base_name: pending_base_names_) {
        if (reg != nullptr) {
            auto* base = reg->find(base_name.view());
            if (base != nullptr) {
                base_types_.push_back(base);
            }
        }
    }
    pending_base_names_.clear();
}

void meta_type::resolve_bases_unlocked(registry* reg) {
    for (auto& base_name: pending_base_names_) {
        if (reg != nullptr) {
            auto* base = reg->find_unlocked(base_name.view());
            if (base != nullptr) {
                base_types_.push_back(base);
            }
        }
    }
    pending_base_names_.clear();
}

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
