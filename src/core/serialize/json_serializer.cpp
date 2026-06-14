#include <NeForce/core/serialize/json_serializer.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SERIALIZE__

namespace {
    void serialize_reflected(json_builder& builder, const reflect::meta_any& obj, const reflect::meta_type& meta,
                             const serialize_context& ctx);

    void serialize_arithmetic(json_builder& builder, const reflect::meta_any& value, const reflect::type_id tid) {
        if (tid == reflect::type_id_for<bool>()) {
            builder.value(*static_cast<const bool*>(value.raw()));
        } else if (tid == reflect::type_id_for<float>()) {
            builder.value(static_cast<double>(*static_cast<const float*>(value.raw())));
        } else if (tid == reflect::type_id_for<double>()) {
            builder.value(*static_cast<const double*>(value.raw()));
        } else if (tid == reflect::type_id_for<long double>()) {
            builder.value(static_cast<double>(*static_cast<const long double*>(value.raw())));
        } else if (is_char_type(tid)) {
            if (tid == reflect::type_id_for<char>()) {
                builder.value(string(1, *static_cast<const char*>(value.raw())));
            } else {
                builder.value(static_cast<double>(*static_cast<const int*>(value.raw())));
            }
        } else if (tid == reflect::type_id_for<int>()) {
            builder.value(static_cast<double>(*static_cast<const int*>(value.raw())));
        } else if (tid == reflect::type_id_for<unsigned int>()) {
            builder.value(static_cast<double>(*static_cast<const unsigned int*>(value.raw())));
        } else {
            builder.value(static_cast<double>(*static_cast<const int64_t*>(value.raw())));
        }
    }

    void serialize_value(json_builder& builder, const reflect::meta_any& value, const serialize_context& ctx) {
        if (!value.has_value()) {
            builder.value(nullptr);
            return;
        }

        const auto tid = value.type_id();

        if (is_arithmetic_type(tid)) {
            serialize_arithmetic(builder, value, tid);
            return;
        }

        if (is_string_type(tid)) {
            builder.value(*static_cast<const string*>(value.raw()));
            return;
        }

        const auto* meta = reflect::registry::instance().find(tid);
        if (meta != nullptr) {
            if (meta->is_enum() && meta->enum_info() != nullptr) {
                const auto* raw_ptr = value.raw();
                if (raw_ptr != nullptr) {
                    const auto int_val = static_cast<int64_t>(*static_cast<const int*>(raw_ptr));
                    const auto enum_name = meta->enum_info()->name_of(int_val);
                    if (!enum_name.empty()) {
                        builder.value(enum_name);
                        return;
                    }
                }
                return;
            }

            if (meta->is_container()) {
                builder.begin_array();
                const void* container_ptr = value.raw();
                const size_t count = meta->container_element_count(container_ptr);
                for (size_t i = 0; i < count; ++i) {
                    auto element = meta->container_element_at(container_ptr, i);
                    serialize_value(builder, element, ctx);
                }
                builder.end_array();
                return;
            }

            serialize_reflected(builder, value, *meta, ctx);
            return;
        }

        NEFORCE_THROW_EXCEPTION(serialize_exception("Unsupported type for serialization"));
    }

    void serialize_reflected(json_builder& builder, const reflect::meta_any& obj, const reflect::meta_type& meta,
                             const serialize_context& ctx) {
        builder.begin_object();

        for (const auto& pair: meta.all_properties()) {
            const auto& prop_name = pair.first;
            const auto* prop = pair.second;

            if (!ctx.include_transient && prop->is_transient()) {
                continue;
            }

            auto value = prop->get(obj.raw());

            if (!value.has_value()) {
                continue;
            }

            builder.key(prop_name);
            serialize_value(builder, value, ctx);
        }

        builder.end_object();
    }

    reflect::meta_any deserialize_object(const json_object& obj, const reflect::meta_type& type,
                                         const serialize_context& ctx) {
        auto instance = type.create();
        if (!instance.has_value()) {
            NEFORCE_THROW_EXCEPTION(deserialize_exception("Failed to create instance of type"));
        }

        void* raw_ptr = instance.raw();

        for (const auto& json_member: obj.get_members()) {
            const auto& member_name = json_member.first;
            const auto& member_value = json_member.second;

            const auto* prop = type.get_property(member_name.view());
            if (prop == nullptr) {
                continue;
            }

            reflect::meta_any prop_value;

            if (member_value->is_null()) {
                prop_value = reflect::meta_any{};
            } else if (member_value->is_number()) {
                const auto* num_val = member_value->as_number();
                if (num_val != nullptr) {
                    const auto num = num_val->get_value();
                    const auto prop_tid = prop->type_id();

                    if (prop_tid == reflect::type_id_for<double>()) {
                        prop_value = reflect::meta_any(num);
                    } else if (prop_tid == reflect::type_id_for<float>()) {
                        prop_value = reflect::meta_any(static_cast<float>(num));
                    } else if (prop_tid == reflect::type_id_for<bool>()) {
                        prop_value = reflect::meta_any(num != 0.0);
                    } else if (prop_tid == reflect::type_id_for<uint64_t>() ||
                               prop_tid == reflect::type_id_for<long long>()) {
                        prop_value = reflect::meta_any(static_cast<int64_t>(num));
                    } else {
                        prop_value = reflect::meta_any(static_cast<int>(num));
                    }
                }
            } else if (member_value->is_bool()) {
                const auto* bool_val = member_value->as_bool();
                if (bool_val != nullptr) {
                    prop_value = reflect::meta_any(bool_val->get_value());
                }
            } else if (member_value->is_string()) {
                const auto* str_val = member_value->as_string();
                if (str_val != nullptr) {
                    prop_value = reflect::meta_any(str_val->get_value());
                }
            } else if (member_value->is_object()) {
                const auto prop_tid = prop->type_id();
                const auto* prop_meta = reflect::registry::instance().find(prop_tid);
                if (prop_meta != nullptr) {
                    if (prop_meta->is_container()) {
                        prop_value = json_serializer::deserialize(*member_value, *prop_meta, ctx);
                    } else {
                        prop_value =
                                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                                deserialize_object(static_cast<const json_object&>(*member_value), *prop_meta, ctx);
                    }
                }
            } else if (member_value->is_array()) {
                const auto prop_tid = prop->type_id();
                const auto* prop_meta = reflect::registry::instance().find(prop_tid);
                if (prop_meta != nullptr && prop_meta->is_container()) {
                    prop_value = json_serializer::deserialize(*member_value, *prop_meta, ctx);
                }
            }

            prop->set(raw_ptr, prop_value);
        }

        return instance;
    }

    reflect::meta_any deserialize_primitive(const json_value& value, const reflect::type_id expected_tid) {
        if (value.is_null()) {
            return {};
        }
        if (value.is_bool()) {
            const auto* bv = value.as_bool();
            if (bv != nullptr) {
                return {bv->get_value()};
            }
        }
        if (value.is_number()) {
            const auto* nv = value.as_number();
            if (nv != nullptr) {
                const auto num = nv->get_value();
                if (expected_tid == reflect::type_id_for<int>()) {
                    return {static_cast<int>(num)};
                }
                if (expected_tid == reflect::type_id_for<double>()) {
                    return {num};
                }
                if (expected_tid == reflect::type_id_for<float>()) {
                    return {static_cast<float>(num)};
                }
                return {static_cast<int64_t>(num)};
            }
        }
        if (value.is_string()) {
            const auto* sv = value.as_string();
            if (sv != nullptr) {
                return {sv->get_value()};
            }
        }
        return {};
    }
} // namespace


unique_ptr<json_value> json_serializer::serialize(const reflect::meta_any& obj, const serialize_context& ctx) {
    if (!obj.has_value()) {
        return make_unique<json_null>();
    }

    const auto tid = obj.type_id();
    const auto* meta = reflect::registry::instance().find(tid);

    if (meta == nullptr) {
        NEFORCE_THROW_EXCEPTION(serialize_exception("Unregistered type in serialization"));
    }

    json_builder builder;
    serialize_reflected(builder, obj, *meta, ctx);
    return builder.build();
}

string json_serializer::to_string(const reflect::meta_any& obj, const serialize_context& ctx) {
    const auto value = serialize(obj, ctx);
    return value ? value->to_string() : string{};
}

reflect::meta_any json_serializer::deserialize(const json_value& value, const reflect::meta_type& type,
                                               const serialize_context& ctx) {
    if (value.is_null()) {
        return {};
    }

    if (type.is_container() && value.is_array()) {
        auto instance = type.create();
        if (!instance.has_value()) {
            NEFORCE_THROW_EXCEPTION(deserialize_exception("Failed to create container instance"));
        }

        void* container_ptr = instance.raw();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        const auto& arr = static_cast<const json_array&>(value);
        const auto& elements = arr.get_elements();
        const auto elem_tid = type.element_type_id();

        for (const auto& elem: elements) {
            reflect::meta_any elem_value;

            if (elem->is_null()) {
                elem_value = reflect::meta_any{};
            } else if (elem->is_object()) {
                const auto* elem_meta = reflect::registry::instance().find(elem_tid);
                if (elem_meta != nullptr) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                    elem_value = deserialize_object(static_cast<const json_object&>(*elem), *elem_meta, ctx);
                }
            } else {
                elem_value = deserialize_primitive(*elem, elem_tid);
            }

            type.container_push_back(container_ptr, elem_value);
        }

        return instance;
    }

    if (!value.is_object()) {
        NEFORCE_THROW_EXCEPTION(deserialize_exception("Expected JSON object for reflected type deserialization"));
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return deserialize_object(static_cast<const json_object&>(value), type, ctx);
}

reflect::meta_any json_serializer::from_string(const string& json_str, const reflect::meta_type& type,
                                               const serialize_context& ctx) {
    json_parser parser{json_str};
    const auto root = parser.parse();
    if (root == nullptr) {
        NEFORCE_THROW_EXCEPTION(deserialize_exception("Failed to parse JSON string"));
    }
    return deserialize(*root, type, ctx);
}

NEFORCE_END_SERIALIZE__
NEFORCE_END_NAMESPACE__
