#include <NeForce/core/reflect/registry.hpp>
#include <NeForce/core/async/signals.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

meta_type* registry::find_unlocked(string_view name) {
    const auto name_hash = name.to_hash();
    const auto map_it = name_hash_to_type_id_.find(name_hash);
    if (map_it == name_hash_to_type_id_.end()) {
        return nullptr;
    }
    const auto it = types_.find(map_it->second);
    return it != types_.end() ? it->second.get() : nullptr;
}

meta_type* registry::find(string_view name) {
    lock<mutex> lock(mutex_);
    return find_unlocked(name);
}

meta_type* registry::find(type_id id) {
    lock<mutex> lock(mutex_);
    const auto it = types_.find(id);
    return it != types_.end() ? it->second.get() : nullptr;
}

void registry::resolve_all_bases() {
    lock<mutex> lk(mutex_);
    for (const auto& type: types_) {
        type.second->resolve_bases_unlocked(this);
    }
}

bool registry::dynamic_connect(void* sender_obj, string_view sender_type, string_view signal_name, size_t signal_offset,
                               void* receiver_obj, string_view receiver_type, string_view slot_name) {
    const auto* s_meta = instance().find(sender_type);
    const auto* r_meta = instance().find(receiver_type);
    if ((s_meta == nullptr) || (r_meta == nullptr)) {
        return false;
    }

    const auto* slot = r_meta->get_function(slot_name);
    if (slot == nullptr) {
        return false;
    }

    auto* sig_base = reinterpret_cast<signal_base*>(static_cast<char*>(sender_obj) + signal_offset);
    if (sig_base == nullptr) {
        return false;
    }

    return connect_signal_to_slot(sig_base, slot, receiver_obj);
}

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
