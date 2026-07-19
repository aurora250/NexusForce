/**
 * @brief 任务持久化层——连接池、CRUD、统计
 */

#ifndef INTEGRATION_TASK_STORE_HPP__
#define INTEGRATION_TASK_STORE_HPP__

#include "task.hpp"

#include <NeForce/core/utility/optional.hpp>
#include <NeForce/db/database_pool.hpp>

class TaskStore {
public:
    /// 初始化连接池并建表
    bool initialize();

    /// 关闭连接池
    void shutdown();

    /// 查询全部任务
    neforce::vector<Task> find_all();

    /// 按 ID 查找
    neforce::optional<Task> find_by_id(const neforce::string& id);

    /// 插入任务（调用方负责事务）
    void insert(neforce::idb_tb_connect& conn, Task& task);

    /// 更新任务（调用方负责事务）
    void update(neforce::idb_tb_connect& conn, const Task& task);

    /// 删除任务（调用方负责事务）
    void remove(neforce::idb_tb_connect& conn, const Task& task);

    /// 获取数据库连接
    neforce::shared_ptr<neforce::idb_tb_connect> get_connection();

    /// 统计信息：total、pending、completed
    neforce::string stats();

private:
    neforce::unique_ptr<neforce::database_pool> pool_;
};

#endif
