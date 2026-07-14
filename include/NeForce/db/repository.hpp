#ifndef NEFORCE_DATABASE_REPOSITORY_HPP__
#define NEFORCE_DATABASE_REPOSITORY_HPP__

/**
 * @file repository.hpp
 * @brief 泛型 Repository 模式实现
 *
 * 在 sql_mapper 之上提供开箱即用的 CRUD 操作。
 * 通过反射元数据自动完成实体与数据库表之间的映射。
 *
 * 使用示例：
 * @code
 * repository<User, idb_tb_connect> repo(conn);
 * repo.create_table();
 *
 * User u{0, "Alice"};
 * repo.insert(u);
 *
 * auto users = repo.find_all();
 * auto user  = repo.find_by_id(meta_any(1));
 * @endcode
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/reflect/reflect.hpp"
#include "NeForce/db/db_interface.hpp"
#include "NeForce/db/sql_builder.hpp"
#include "NeForce/db/sql_mapper.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Repository Repository
 * @brief 开箱即用的 CRUD 操作
 * @{
 */

/**
 * @class repository
 * @brief 泛型 Repository
 * @tparam T 实体类型（需已通过反射注册）
 * @tparam Connect 连接类型，默认为 idb_tb_connect
 *
 * 封装常见的数据访问操作，通过 sql_mapper 自动生成 SQL。
 */
template <typename T, typename Connect = idb_tb_connect>
class repository {
    static_assert(is_base_of_v<idb_connect, Connect>, "Connect must be derived from idb_connect");

    Connect& conn_; ///< 数据库连接引用

    /**
     * @brief 使用实际值生成 INSERT SQL
     */
    static string build_insert_with_values(const T& entity) {
        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());
        const string tbl = sql_mapper<T>::table_name();

        sql_builder b;
        vector<string> columns;
        vector<string> vals;

        for (const auto& entry: meta->all_properties()) {
            const auto* prop = entry.second;
            if (prop->is_transient() || prop->is_auto_increment()) {
                continue;
            }
            columns.push_back(prop->name());
            const reflect::meta_any val = prop->get(&entity);
            vals.push_back(meta_any_to_sql_literal(val));
        }

        b.insert_into(tbl, columns);
        b.values(vals);
        return b.build();
    }

    /**
     * @brief 使用实际值生成 UPDATE SQL
     */
    static string build_update_with_values(const T& entity) {
        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());
        const string tbl = sql_mapper<T>::table_name();

        const reflect::meta_property* pk = nullptr;
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_primary_key()) {
                pk = entry.second;
                break;
            }
        }
        if (pk == nullptr) {
            NEFORCE_THROW_EXCEPTION(value_exception("repository::update: no primary key defined"));
        }

        sql_builder b;
        b.update(tbl);

        for (const auto& entry: meta->all_properties()) {
            const auto* prop = entry.second;
            if (prop->is_transient() || prop->is_primary_key() || prop->is_auto_increment()) {
                continue;
            }
            const reflect::meta_any val = prop->get(&entity);
            b.set(prop->name(), meta_any_to_sql_literal(val));
        }

        const reflect::meta_any pk_val = pk->get(&entity);
        b.where_eq(pk->name(), meta_any_to_sql_literal(pk_val));
        return b.build();
    }

    /**
     * @brief 使用实际 PK 值生成 DELETE SQL
     */
    static string build_delete_with_pk(const T& entity) {
        const auto pkv = sql_mapper<T>::get_pk_value(entity);
        const string tbl = sql_mapper<T>::table_name();

        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());
        const reflect::meta_property* pk = nullptr;
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_primary_key()) {
                pk = entry.second;
                break;
            }
        }
        if (pk == nullptr) {
            NEFORCE_THROW_EXCEPTION(value_exception("repository::remove: no primary key defined"));
        }

        sql_builder b;
        b.delete_from(tbl);
        b.where_eq(pk->name(), meta_any_to_sql_literal(pkv));
        return b.build();
    }

public:
    /**
     * @brief 构造函数
     * @param conn 数据库连接引用
     */
    explicit repository(Connect& conn) noexcept :
    conn_(conn) {}

    /**
     * @brief 获取内部连接引用
     * @return 连接引用
     */
    NEFORCE_NODISCARD Connect& connection() noexcept { return conn_; }

    /**
     * @brief 创建数据库表
     * @param table_override 可选表名覆盖
     * @return 执行成功返回 true
     */
    bool create_table(string_view table_override = "") {
        const string sql = sql_mapper<T>::create_table_sql(table_override);
        return conn_.update(sql);
    }

    /**
     * @brief 删除数据库表
     * @param table_override 可选表名覆盖
     * @return 执行成功返回 true
     */
    bool drop_table(string_view table_override = "") {
        const string sql = sql_mapper<T>::drop_table_sql(table_override);
        return conn_.update(sql);
    }

    /**
     * @brief 按主键查找实体
     * @param id 主键值（包装为 meta_any）
     * @return 找到的实体，未找到返回空 unique_ptr
     */
    unique_ptr<T> find_by_id(const reflect::meta_any& id) {
        const string tbl = sql_mapper<T>::table_name();
        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());

        const reflect::meta_property* pk = nullptr;
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_primary_key()) {
                pk = entry.second;
                break;
            }
        }
        if (pk == nullptr) {
            return nullptr;
        }

        sql_builder b;
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_transient()) {
                continue;
            }
            b.select(entry.second->name());
        }
        b.from(tbl);
        b.where_eq(pk->name(), meta_any_to_sql_literal(id));

        auto result = conn_.query(b.build());
        if (result == nullptr || result->empty()) {
            return nullptr;
        }

        if (!result->next()) {
            return nullptr;
        }

        return make_unique<T>(sql_mapper<T>::from_row(*result));
    }

    /**
     * @brief 查询全部实体
     * @return 实体列表
     */
    vector<T> find_all() {
        const string tbl = sql_mapper<T>::table_name();
        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());

        sql_builder b;
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_transient()) {
                continue;
            }
            b.select(entry.second->name());
        }
        b.from(tbl);

        auto result = conn_.query(b.build());
        return sql_mapper<T>::from_result(move(result));
    }

    /**
     * @brief 按 WHERE 条件查询实体
     * @param where_clause 原始 WHERE 子句（不含 "WHERE" 关键字）
     * @return 实体列表
     */
    vector<T> find_where(const string& where_clause) {
        const string tbl = sql_mapper<T>::table_name();
        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());

        sql_builder b;
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_transient()) {
                continue;
            }
            b.select(entry.second->name());
        }
        b.from(tbl);
        b.where(where_clause);

        auto result = conn_.query(b.build());
        return sql_mapper<T>::from_result(move(result));
    }

    /**
     * @brief 分页查询实体
     * @param page_num 页码（从 1 开始）
     * @param page_size 每页大小
     * @param order_by 排序字段（为空则不排序）
     * @return 实体列表
     */
    vector<T> find_page(const size_t page_num, const size_t page_size, string_view order_by = "") {
        const string tbl = sql_mapper<T>::table_name();
        auto* meta = reflect::registry::instance().find(reflect::type_id_for<T>());

        sql_builder b;
        for (const auto& entry: meta->all_properties()) {
            if (entry.second->is_transient()) {
                continue;
            }
            b.select(entry.second->name());
        }
        b.from(tbl);

        if (!order_by.empty()) {
            b.order_by_asc(order_by);
        }

        b.page(static_cast<int>(page_num), static_cast<int>(page_size));

        auto result = conn_.query(b.build());
        return sql_mapper<T>::from_result(move(result));
    }

    /**
     * @brief 插入实体
     * @param entity 实体对象
     * @return 执行成功返回 true
     *
     * @note 自增主键字段在 INSERT 时被跳过，值由数据库自动生成。
     *       如需获取自增主键值，可在此方法返回后执行 SELECT LAST_INSERT_ID()。
     */
    bool insert(T& entity) {
        const string sql = build_insert_with_values(entity);
        return conn_.update(sql);
    }

    /**
     * @brief 更新实体（按主键）
     * @param entity 实体对象
     * @return 执行成功返回 true
     */
    bool update(const T& entity) {
        const string sql = build_update_with_values(entity);
        return conn_.update(sql);
    }

    /**
     * @brief 删除实体（按主键）
     * @param entity 实体对象
     * @return 执行成功返回 true
     */
    bool remove(const T& entity) {
        const string sql = build_delete_with_pk(entity);
        return conn_.update(sql);
    }

    /**
     * @brief 获取实体总数
     * @return 行数，查询失败返回 0
     */
    size_t count() {
        const string tbl = sql_mapper<T>::table_name();
        sql_builder b;
        b.select_count();
        b.from(tbl);

        auto result = conn_.query(b.build());
        if (result == nullptr || result->empty()) {
            return 0;
        }
        if (!result->next()) {
            return 0;
        }
        return static_cast<size_t>(result->get_int64(0));
    }

    /**
     * @brief 检查表是否存在
     * @return 存在返回 true
     */
    bool table_exists() { return conn_.table_exists(sql_mapper<T>::table_name()); }
};

/** @} */ // Repository

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_REPOSITORY_HPP__
