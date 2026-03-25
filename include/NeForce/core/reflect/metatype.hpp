#ifndef NEFORCE_CORE_REFLECT_METATYPE_HPP__
#define NEFORCE_CORE_REFLECT_METATYPE_HPP__
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/reflect/property.hpp"
#include "NeForce/core/reflect/function.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

class registry;


class meta_type {
public:
    using constructor_func = _NEFORCE function<any(const vector<any>&)>;

private:
    reflect::type_id type_id_;
    string_view name_;
    size_t size_;
    constructor_func constructor_;
    vector<meta_type*> base_types_;
    vector<string> pending_base_names_;
    unordered_map<string, unique_ptr<meta_property>> properties_;
    unordered_map<string, unique_ptr<meta_function>> functions_;

    void collect_properties(vector<pair<string, const meta_property*>>& result,
                            vector<reflect::type_id>* visited = nullptr) const {
        vector<reflect::type_id> local_visited;
        if (!visited) visited = &local_visited;

        if (find(visited->begin(), visited->end(), type_id_) != visited->end()) {
            return;
        }
        visited->push_back(type_id_);

        for (auto* base : base_types_) {
            if (base) {
                base->collect_properties(result, visited);
            }
        }

        for (const auto& [name, prop] : properties_) {
            result.emplace_back(name, prop.get());
        }
    }

    void collect_functions(vector<pair<string, const meta_function*>>& result,
                           vector<reflect::type_id>* visited = nullptr) const {
        vector<reflect::type_id> local_visited;
        if (!visited) visited = &local_visited;

        if (find(visited->begin(), visited->end(), type_id_) != visited->end()) {
            return;
        }
        visited->push_back(type_id_);

        for (auto* base : base_types_) {
            if (base) {
                base->collect_functions(result, visited);
            }
        }

        for (const auto& [name, func] : functions_) {
            result.emplace_back(name, func.get());
        }
    }

public:
    meta_type(string_view name, reflect::type_id id, size_t size)
    : type_id_(id), name_(name), size_(size) {}

    NEFORCE_NODISCARD reflect::type_id type_id() const noexcept { return type_id_; }
    NEFORCE_NODISCARD string_view name() const noexcept { return name_; }
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }
    NEFORCE_NODISCARD const vector<meta_type*>& base_types() const { return base_types_; }

    meta_type& base_type(meta_type* base) {
        if (base) base_types_.push_back(base);
        return *this;
    }

    meta_type& base_type(string_view base_name) {
        pending_base_names_.push_back(base_name);
        return *this;
    }

    NEFORCE_NODISCARD bool is_derived_from(reflect::type_id base_id) const {
        if (type_id_ == base_id) return true;
        for (auto* base : base_types_) {
            if (base && base->is_derived_from(base_id)) return true;
        }
        return false;
    }

    NEFORCE_NODISCARD bool is_derived_from(string_view base_name) const {
        return is_derived_from(base_name.to_hash());
    }

    meta_type& property(string_view name,
                       reflect::type_id prop_type_id,
                       meta_property::getter getter,
                       meta_property::setter setter) {
        properties_.emplace(name, make_unique<meta_property>(name, prop_type_id, move(getter), move(setter)));
        return *this;
    }

    meta_function* function(string_view name, meta_function::invoker invoker) {
        auto [it, inserted] = functions_.emplace(name, make_unique<meta_function>(name, move(invoker)));
        return it->second.get();
    }

    meta_type& constructor(constructor_func ctor) {
        constructor_ = move(ctor);
        return *this;
    }

    NEFORCE_NODISCARD const meta_property* get_property(string_view name) const {
        auto it = properties_.find(string(name));
        if (it != properties_.end()) {
            return it->second.get();
        }

        for (auto* base : base_types_) {
            if (base) {
                if (auto* prop = base->get_property(name)) {
                    return prop;
                }
            }
        }
        return nullptr;
    }

    NEFORCE_NODISCARD const meta_function* get_function(string_view name) const {
        auto it = functions_.find(string(name));
        if (it != functions_.end()) {
            return it->second.get();
        }

        for (auto* base : base_types_) {
            if (base) {
                if (auto* func = base->get_function(name)) {
                    return func;
                }
            }
        }
        return nullptr;
    }

    NEFORCE_NODISCARD any create() const {
        return constructor_ ? constructor_({}) : any{};
    }

    NEFORCE_NODISCARD any create(const vector<any>& args) const {
        return constructor_ ? constructor_(args) : any{};
    }

    NEFORCE_NODISCARD const auto& properties() const { return properties_; }
    NEFORCE_NODISCARD const auto& functions() const { return functions_; }

    NEFORCE_NODISCARD vector<pair<string, const meta_property*>> all_properties() const {
        vector<pair<string, const meta_property*>> result;
        collect_properties(result);
        return result;
    }

    NEFORCE_NODISCARD vector<pair<string, const meta_function*>> all_functions() const {
        vector<pair<string, const meta_function*>> result;
        collect_functions(result);
        return result;
    }

    void resolve_bases(registry* registry);
    void resolve_bases_unlocked(registry* registry);
};

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_METATYPE_HPP__
