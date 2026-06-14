#include <NeForce/core/serialize/binary_serializer.hpp>
#include <NeForce/core/serialize/serialize_exception.hpp>
#include <NeForce/core/memory/endian.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SERIALIZE__

namespace {
    void write_type_table(binary_serializer::buffer& buf, const vector<reflect::type_id>& types) {
        const uint32_t count_be = endian::host_to_be(static_cast<uint32_t>(types.size()));
        buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&count_be),
                   reinterpret_cast<const byte_t*>(&count_be) + sizeof(count_be));

        for (const auto tid: types) {
            const auto* meta = reflect::registry::instance().find(tid);
            if (meta == nullptr) {
                continue;
            }

            const string_view name = meta->name();
            const uint16_t name_len_be = endian::host_to_be(static_cast<uint16_t>(name.length()));
            buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&name_len_be),
                       reinterpret_cast<const byte_t*>(&name_len_be) + sizeof(name_len_be));

            buf.insert(buf.end(), reinterpret_cast<const byte_t*>(name.data()),
                       reinterpret_cast<const byte_t*>(name.data()) + name.length());

            const auto tid_be = endian::host_to_be<uint64_t>(tid);
            buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&tid_be),
                       reinterpret_cast<const byte_t*>(&tid_be) + sizeof(tid_be));
        }
    }

    void write_value(binary_serializer::buffer& buf, const reflect::meta_any& value, const serialize_context& ctx) {
        if (!value.has_value()) {
            buf.push_back(0);
            return;
        }

        const auto tid = value.type_id();
        const void* raw = value.raw();
        if (raw == nullptr) {
            buf.push_back(0);
            return;
        }

        if (is_arithmetic_type(tid)) {
            buf.push_back(1);

            if (tid == reflect::type_id_for<bool>()) {
                buf.push_back(*static_cast<const bool*>(raw) ? 1 : 0);
            } else if (tid == reflect::type_id_for<float>()) {
                const uint32_t int_bits = endian::host_to_be(*static_cast<const uint32_t*>(raw));
                buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&int_bits),
                           reinterpret_cast<const byte_t*>(&int_bits) + sizeof(int_bits));
            } else if (tid == reflect::type_id_for<double>()) {
                const uint64_t int_bits = endian::host_to_be(*static_cast<const uint64_t*>(raw));
                buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&int_bits),
                           reinterpret_cast<const byte_t*>(&int_bits) + sizeof(int_bits));
            } else {
                int64_t int_val = 0;
                if (tid == reflect::type_id_for<int>()) {
                    int_val = static_cast<int64_t>(*static_cast<const int*>(raw));
                } else if (tid == reflect::type_id_for<unsigned int>()) {
                    int_val = static_cast<int64_t>(*static_cast<const unsigned int*>(raw));
                } else if (tid == reflect::type_id_for<short>()) {
                    int_val = static_cast<int64_t>(*static_cast<const short*>(raw));
                } else if (tid == reflect::type_id_for<unsigned short>()) {
                    int_val = static_cast<int64_t>(*static_cast<const unsigned short*>(raw));
                } else if (tid == reflect::type_id_for<long>()) {
                    int_val = static_cast<int64_t>(*static_cast<const long*>(raw));
                } else if (tid == reflect::type_id_for<unsigned long>()) {
                    int_val = static_cast<int64_t>(*static_cast<const unsigned long*>(raw));
                } else if (tid == reflect::type_id_for<long long>()) {
                    int_val = *static_cast<const long long*>(raw);
                } else if (tid == reflect::type_id_for<unsigned long long>()) {
                    int_val = static_cast<int64_t>(*static_cast<const unsigned long long*>(raw));
                } else if (tid == reflect::type_id_for<char>()) {
                    int_val = static_cast<int64_t>(*static_cast<const byte_t*>(raw));
                } else {
                    int_val = *static_cast<const int64_t*>(raw);
                }

                const uint64_t be_val = endian::host_to_be(static_cast<uint64_t>(int_val));
                buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&be_val),
                           reinterpret_cast<const byte_t*>(&be_val) + sizeof(be_val));
            }
            return;
        }

        if (is_string_type(tid)) {
            buf.push_back(4);
            const auto* str = static_cast<const string*>(raw);
            const uint32_t len_be = endian::host_to_be(static_cast<uint32_t>(str->length()));
            buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&len_be),
                       reinterpret_cast<const byte_t*>(&len_be) + sizeof(len_be));
            buf.insert(buf.end(), reinterpret_cast<const byte_t*>(str->data()),
                       reinterpret_cast<const byte_t*>(str->data()) + str->length());
            return;
        }

        const auto* meta = reflect::registry::instance().find(tid);
        if (meta != nullptr) {
            if (meta->is_enum()) {
                buf.push_back(1);
                const auto int_val = static_cast<int64_t>(*static_cast<const int*>(raw));
                const uint64_t be_val = endian::host_to_be(static_cast<uint64_t>(int_val));
                buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&be_val),
                           reinterpret_cast<const byte_t*>(&be_val) + sizeof(be_val));
                return;
            }

            if (meta->is_container()) {
                buf.push_back(3);
                const void* container_ptr = value.raw();
                const size_t count = meta->container_element_count(container_ptr);
                const uint32_t count_be = endian::host_to_be(static_cast<uint32_t>(count));
                buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&count_be),
                           reinterpret_cast<const byte_t*>(&count_be) + sizeof(count_be));
                for (size_t i = 0; i < count; ++i) {
                    auto element = meta->container_element_at(container_ptr, i);
                    write_value(buf, element, ctx);
                }
                return;
            }

            buf.push_back(2);
            auto nested_data = binary_serializer::serialize(value, ctx);
            const uint32_t len_be = endian::host_to_be(static_cast<uint32_t>(nested_data.size()));
            buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&len_be),
                       reinterpret_cast<const byte_t*>(&len_be) + sizeof(len_be));
            buf.insert(buf.end(), nested_data.begin(), nested_data.end());
            return;
        }

        buf.push_back(0);
    }

    void write_property(binary_serializer::buffer& buf, const reflect::meta_property& prop,
                        const reflect::meta_any& value) {
        const string_view name = prop.name();
        const uint16_t name_len_be = endian::host_to_be(static_cast<uint16_t>(name.length()));
        buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&name_len_be),
                   reinterpret_cast<const byte_t*>(&name_len_be) + sizeof(name_len_be));

        buf.insert(buf.end(), reinterpret_cast<const byte_t*>(name.data()),
                   reinterpret_cast<const byte_t*>(name.data()) + name.length());

        write_value(buf, value, {});
    }
} // namespace


binary_serializer::buffer binary_serializer::serialize(const reflect::meta_any& obj, const serialize_context& ctx) {
    buffer buf;
    if (!obj.has_value()) {
        return buf;
    }

    buf.reserve(256);

    constexpr uint32_t magic_be = endian::host_to_be(MAGIC);
    buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&magic_be),
               reinterpret_cast<const byte_t*>(&magic_be) + sizeof(magic_be));

    constexpr uint16_t version_be = endian::host_to_be(VERSION);
    buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&version_be),
               reinterpret_cast<const byte_t*>(&version_be) + sizeof(version_be));

    const auto* meta = reflect::registry::instance().find(obj.type_id());
    if (meta == nullptr) {
        NEFORCE_THROW_EXCEPTION(serialize_exception("Unregistered type in binary serialization"));
    }

    vector<reflect::type_id> type_table;
    type_table.push_back(meta->type_id());
    write_type_table(buf, type_table);

    constexpr uint32_t obj_count_be = endian::host_to_be(1);
    buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&obj_count_be),
               reinterpret_cast<const byte_t*>(&obj_count_be) + sizeof(obj_count_be));

    const auto tid_be = endian::host_to_be<uint64_t>(obj.type_id());
    buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&tid_be),
               reinterpret_cast<const byte_t*>(&tid_be) + sizeof(tid_be));

    const auto all_props = meta->all_properties();
    uint32_t prop_count = 0;
    for (const auto& pair: all_props) {
        if (!ctx.include_transient && pair.second->is_transient()) {
            continue;
        }
        if (pair.second->is_readonly()) {
            continue;
        }
        ++prop_count;
    }

    const uint32_t prop_count_be = endian::host_to_be(prop_count);
    buf.insert(buf.end(), reinterpret_cast<const byte_t*>(&prop_count_be),
               reinterpret_cast<const byte_t*>(&prop_count_be) + sizeof(prop_count_be));

    const auto* raw_ptr = obj.raw();
    if (raw_ptr == nullptr) {
        NEFORCE_THROW_EXCEPTION(serialize_exception("Failed to get raw object pointer"));
    }

    for (const auto& pair: all_props) {
        const auto* prop = pair.second;
        if (!ctx.include_transient && prop->is_transient()) {
            continue;
        }
        if (prop->is_readonly()) {
            continue;
        }

        write_property(buf, *prop, prop->get(raw_ptr));
    }

    return buf;
}

reflect::meta_any binary_serializer::deserialize(const byte_t* data, const size_t size) {
    if (data == nullptr || size < 14) {
        NEFORCE_THROW_EXCEPTION(deserialize_exception("Invalid binary data"));
    }

    const byte_t* cursor = data;
    const byte_t* const data_end = data + size;

    const auto magic = endian::read_be32(cursor);
    cursor += sizeof(uint32_t);
    if (magic != MAGIC) {
        NEFORCE_THROW_EXCEPTION(deserialize_exception("Invalid binary magic number"));
    }

    cursor += sizeof(uint16_t);

    const auto type_count = endian::read_be32(cursor);
    cursor += sizeof(uint32_t);

    for (uint32_t i = 0; i < type_count; ++i) {
        const auto name_len = endian::read_be16(cursor);
        cursor += sizeof(uint16_t);
        cursor += name_len;
        cursor += sizeof(uint64_t);
    }

    const auto obj_count = endian::read_be32(cursor);
    cursor += sizeof(uint32_t);

    if (obj_count == 0) {
        return reflect::meta_any{};
    }

    const auto obj_tid = static_cast<reflect::type_id>(endian::read_be64(cursor));
    cursor += sizeof(uint64_t);

    const auto* meta = reflect::registry::instance().find(obj_tid);
    if (meta == nullptr) {
        NEFORCE_THROW_EXCEPTION(deserialize_exception("Unknown type ID in binary data"));
    }

    auto instance = meta->create();
    if (!instance.has_value()) {
        NEFORCE_THROW_EXCEPTION(deserialize_exception("Failed to create instance"));
    }

    const auto prop_count = endian::read_be32(cursor);
    cursor += sizeof(uint32_t);

    void* raw_ptr = instance.raw();
    if (raw_ptr == nullptr) {
        NEFORCE_THROW_EXCEPTION(deserialize_exception("Failed to get raw instance pointer"));
    }

    for (uint32_t i = 0; i < prop_count; ++i) {
        if (cursor + sizeof(uint16_t) > data_end) {
            break;
        }

        const auto name_len = endian::read_be16(cursor);
        cursor += sizeof(uint16_t);

        if (cursor + name_len > data_end) {
            break;
        }

        string_view prop_name(reinterpret_cast<const char*>(cursor), name_len);
        cursor += name_len;

        const auto* prop = meta->get_property(prop_name);
        if (prop == nullptr || cursor >= data_end) {
            const auto type_tag = *cursor;
            ++cursor;
            if (type_tag == 1) {
                cursor += sizeof(uint64_t);
            } else if (type_tag == 2) {
                if (cursor + sizeof(uint32_t) <= data_end) {
                    const auto nested_len = endian::read_be32(cursor);
                    cursor += sizeof(uint32_t) + nested_len;
                }
            } else if (type_tag == 3) {
                if (cursor + sizeof(uint32_t) <= data_end) {
                    const auto elem_count = endian::read_be32(cursor);
                    cursor += sizeof(uint32_t);
                    for (uint32_t j = 0; j < elem_count; ++j) {
                        const auto tag = *cursor;
                        ++cursor;
                        if (tag == 0) {
                        } else if (tag == 1) {
                            cursor += sizeof(uint64_t);
                        } else if (tag == 2) {
                            if (cursor + sizeof(uint32_t) <= data_end) {
                                const auto nl = endian::read_be32(cursor);
                                cursor += sizeof(uint32_t) + nl;
                            }
                        } else if (tag == 4) {
                            if (cursor + sizeof(uint32_t) <= data_end) {
                                const auto sl = endian::read_be32(cursor);
                                cursor += sizeof(uint32_t) + sl;
                            }
                        }
                    }
                }
            } else if (type_tag == 4) {
                if (cursor + sizeof(uint32_t) <= data_end) {
                    const auto str_len = endian::read_be32(cursor);
                    cursor += sizeof(uint32_t) + str_len;
                }
            }
            continue;
        }

        const auto type_tag = *cursor;
        ++cursor;

        if (type_tag == 0) {
            prop->set(raw_ptr, reflect::meta_any{});
        } else if (type_tag == 1) {
            if (cursor + sizeof(uint64_t) > data_end) {
                break;
            }

            const auto prop_tid = prop->type_id();
            const auto bits = endian::read_be64(cursor);
            cursor += sizeof(uint64_t);

            if (prop_tid == reflect::type_id_for<bool>()) {
                prop->set(raw_ptr, reflect::meta_any(bits != 0));
            } else if (prop_tid == reflect::type_id_for<float>()) {
                const auto as_uint = static_cast<uint32_t>(bits);
                const auto fval = *reinterpret_cast<const float*>(&as_uint);
                prop->set(raw_ptr, reflect::meta_any(fval));
            } else if (prop_tid == reflect::type_id_for<double>()) {
                const auto dval = *reinterpret_cast<const double*>(&bits);
                prop->set(raw_ptr, reflect::meta_any(dval));
            } else if (prop_tid == reflect::type_id_for<int>()) {
                prop->set(raw_ptr, reflect::meta_any(static_cast<int>(bits)));
            } else if (prop_tid == reflect::type_id_for<unsigned int>()) {
                prop->set(raw_ptr, reflect::meta_any(static_cast<unsigned int>(bits)));
            } else if (prop_tid == reflect::type_id_for<short>()) {
                prop->set(raw_ptr, reflect::meta_any(static_cast<short>(bits)));
            } else if (prop_tid == reflect::type_id_for<unsigned short>()) {
                prop->set(raw_ptr, reflect::meta_any(static_cast<unsigned short>(bits)));
            } else if (prop_tid == reflect::type_id_for<char>()) {
                prop->set(raw_ptr, reflect::meta_any(static_cast<char>(bits)));
            } else {
                prop->set(raw_ptr, reflect::meta_any(static_cast<int64_t>(bits)));
            }
        } else if (type_tag == 2) {
            if (cursor + sizeof(uint32_t) > data_end) {
                break;
            }
            const auto nested_len = endian::read_be32(cursor);
            cursor += sizeof(uint32_t);

            if (cursor + nested_len > data_end) {
                break;
            }

            const auto nested_obj = binary_serializer::deserialize(cursor, nested_len);
            cursor += nested_len;

            if (nested_obj.has_value()) {
                prop->set(raw_ptr, nested_obj);
            }
        } else if (type_tag == 3) {
            if (cursor + sizeof(uint32_t) > data_end) {
                break;
            }

            const auto elem_count = endian::read_be32(cursor);
            cursor += sizeof(uint32_t);

            const auto* container_meta = reflect::registry::instance().find(prop->type_id());
            if (container_meta != nullptr && container_meta->is_container()) {
                auto container_obj = container_meta->create();
                void* container_ptr = container_obj.raw();

                for (uint32_t j = 0; j < elem_count && cursor < data_end; ++j) {
                    const auto elem_tag = *cursor;
                    ++cursor;

                    if (elem_tag == 0) {
                        container_meta->container_push_back(container_ptr, reflect::meta_any{});
                    } else if (elem_tag == 1) {
                        if (cursor + sizeof(uint64_t) > data_end) {
                            break;
                        }
                        const auto bits = static_cast<int64_t>(endian::read_be64(cursor));
                        cursor += sizeof(uint64_t);
                        const auto elem_tid = container_meta->element_type_id();
                        reflect::meta_any val;
                        if (elem_tid == reflect::type_id_for<int>()) {
                            val = reflect::meta_any(static_cast<int>(bits));
                        } else if (elem_tid == reflect::type_id_for<double>()) {
                            val = reflect::meta_any(*reinterpret_cast<const double*>(&bits));
                        } else {
                            val = reflect::meta_any(bits);
                        }
                        container_meta->container_push_back(container_ptr, val);
                    } else if (elem_tag == 2) {
                        if (cursor + sizeof(uint32_t) > data_end) {
                            break;
                        }
                        const auto nl = endian::read_be32(cursor);
                        cursor += sizeof(uint32_t);
                        if (cursor + nl > data_end) {
                            break;
                        }
                        const auto nested_obj = binary_serializer::deserialize(cursor, nl);
                        cursor += nl;
                        if (nested_obj.has_value()) {
                            container_meta->container_push_back(container_ptr, nested_obj);
                        }
                    } else if (elem_tag == 4) {
                        if (cursor + sizeof(uint32_t) > data_end) {
                            break;
                        }
                        const auto sl = endian::read_be32(cursor);
                        cursor += sizeof(uint32_t);
                        if (cursor + sl > data_end) {
                            break;
                        }
                        string se(reinterpret_cast<const char*>(cursor), sl);
                        cursor += sl;
                        container_meta->container_push_back(container_ptr, reflect::meta_any(se));
                    }
                }

                prop->set(raw_ptr, container_obj);
            }
        } else if (type_tag == 4) {
            if (cursor + sizeof(uint32_t) > data_end) {
                break;
            }
            const auto str_len = endian::read_be32(cursor);
            cursor += sizeof(uint32_t);
            if (cursor + str_len > data_end) {
                break;
            }
            string str_val(reinterpret_cast<const char*>(cursor), str_len);
            cursor += str_len;
            prop->set(raw_ptr, reflect::meta_any(str_val));
        }
    }

    return instance;
}

NEFORCE_END_SERIALIZE__
NEFORCE_END_NAMESPACE__
