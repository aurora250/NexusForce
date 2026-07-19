/**
 * @brief 任务实体
 *
 * NEFORCE_REFLECT_* 供 NFRS 扫描器在构建时自动生成反射注册代码。
 */

#ifndef INTEGRATION_TASK_HPP__
#define INTEGRATION_TASK_HPP__

#include <NeForce/core/reflect/reflect_macros.hpp>
#include <NeForce/core/string/string.hpp>

struct Task {
    NEFORCE_REFLECT_OBJ(Task)
    NEFORCE_DB_TABLE("tasks")

    NEFORCE_REFLECT_PROP_ATTR(neforce::string, id, PROP_PRIMARY_KEY)
    neforce::string id;

    NEFORCE_REFLECT_PROP_ATTR(neforce::string, title, PROP_REQUIRED)
    neforce::string title;

    NEFORCE_REFLECT_PROP(neforce::string, status)
    neforce::string status; // "pending" | "in_progress" | "completed"

    NEFORCE_REFLECT_PROP_ATTR(int, priority, PROP_INDEX)
    int priority = 0;

    NEFORCE_REFLECT_PROP(neforce::string, created_at)
    neforce::string created_at;

    NEFORCE_REFLECT_PROP(neforce::string, updated_at)
    neforce::string updated_at;
};

#endif
