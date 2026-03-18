#ifndef NEFORCE_CORE_REFLECT_REGISTRY_HPP__
#define NEFORCE_CORE_REFLECT_REGISTRY_HPP__
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/reflect/metatype.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

class registry {
private:
    mutex mutex_;
    unordered_map<type_id, unique_ptr<meta_type>> types_;

    registry() = default;

public:
    static registry& instance() {
        static registry inst;
        return inst;
    }

    template <typename T>
    meta_type& register_type(string_view name) {
        type_id id = name.to_hash();

        lock<mutex> lk(mutex_);
        auto it = types_.find(id);
        if (it != types_.end()) {
            return *it->second;
        }

        auto new_it = types_.emplace(id, make_unique<meta_type>(name, id, sizeof(T))).first;
        return *new_it->second;
    }

    meta_type* find_unlocked(string_view name) {
        auto it = types_.find(name.to_hash());
        return it != types_.end() ? it->second.get() : nullptr;
    }

    meta_type* find(string_view name) {
        lock<mutex> lock(mutex_);
        return find_unlocked(name);
    }

    meta_type* find(type_id id) {
        lock<mutex> lock(mutex_);
        auto it = types_.find(id);
        return it != types_.end() ? it->second.get() : nullptr;
    }

    template <typename Func>
    void for_each(Func&& func) {
        lock<mutex> lock(mutex_);
        for (auto& type : types_) {
            func(*type.second);
        }
    }

    void resolve_all_bases() {
        lock<mutex> lk(mutex_);
        for (auto& type : types_) {
            type.second->resolve_bases_unlocked(this);
        }
    }
};


inline void meta_type::resolve_bases(registry* registry) {
    for (auto& base_name : pending_base_names_) {
        if (registry) {
            auto* base = registry->find(base_name.view());
            if (base) {
                base_types_.push_back(base);
            }
        }
    }
    pending_base_names_.clear();
}

inline void meta_type::resolve_bases_unlocked(registry* registry) {
    for (auto& base_name : pending_base_names_) {
        if (registry) {
            auto* base = registry->find_unlocked(base_name.view());
            if (base) {
                base_types_.push_back(base);
            }
        }
    }
    pending_base_names_.clear();
}

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_REGISTRY_HPP__
