#include "task_store.hpp"

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/db/db_config.hpp>
#include <NeForce/db/repository.hpp>
#include <NeForce/db/sql_builder.hpp>
#include <NeForce/logging/logger.hpp>
#include <NeForce/db/sqlite/sqlite_connect.hpp>

using namespace neforce;

bool TaskStore::initialize() {
    database_pool::pool_config cfg;
    cfg.min_size = 2;
    cfg.init_size = 2;
    cfg.max_size = 8;
    cfg.max_idle_time = seconds{60};
    cfg.acquire_timeout = milliseconds{5000};

    db_config db_cfg = db_config::for_sqlite("task_server.db");
    pool_ = make_unique<database_pool>(db_type::SQLITE3, db_cfg, cfg);
    pool_->warm_up(3);

    auto conn = pool_->get_tb_connect();
    if (!conn) {
        return false;
    }

    repository<task, idb_tb_connect> repo{*conn};
    if (!repo.create_table()) {
        return false;
    }

    NEFORCE_LOGGER_LOGF_INFO("app.task", "连接池就绪: total={}, idle={}", pool_->total_count(), pool_->idle_count());
    return true;
}

void TaskStore::shutdown() {
    if (pool_) {
        pool_->stop();
    }
}

shared_ptr<idb_tb_connect> TaskStore::get_connection() {
    if (!pool_) {
        return nullptr;
    }
    return pool_->get_tb_connect();
}

vector<task> TaskStore::find_all() {
    auto conn = get_connection();
    if (!conn) {
        return {};
    }
    repository<task, idb_tb_connect> repo{*conn};
    return repo.find_all();
}

optional<task> TaskStore::find_by_id(const string& id) {
    auto conn = get_connection();
    if (!conn) {
        return none;
    }
    repository<task, idb_tb_connect> repo{*conn};
    auto tasks = repo.find_where("id = '" + id + "'");
    if (tasks.empty()) {
        return none;
    }
    return tasks[0];
}

void TaskStore::insert(idb_tb_connect& conn, task& task) {
    repository<task, idb_tb_connect> repo{conn};
    repo.insert(task);
}

void TaskStore::update(idb_tb_connect& conn, const task& task) {
    repository<task, idb_tb_connect> repo{conn};
    repo.update(task);
}

void TaskStore::remove(idb_tb_connect& conn, const task& task) {
    repository<task, idb_tb_connect> repo{conn};
    repo.remove(task);
}

string TaskStore::stats() {
    auto conn = get_connection();
    if (!conn) {
        return R"({"total":0,"pending":0,"completed":0})";
    }

    json_builder jb;
    jb.begin_object();

    sql_builder cnt;
    cnt.select_count().from("tasks");
    auto r1 = conn->query(cnt.build());
    if (r1 && r1->next()) {
        jb.key("total").value(static_cast<double>(r1->get_int32(0)));
    }

    cnt.reset();
    cnt.select_count().from("tasks").where_eq("status", "'pending'");
    auto r2 = conn->query(cnt.build());
    if (r2 && r2->next()) {
        jb.key("pending").value(static_cast<double>(r2->get_int32(0)));
    }

    cnt.reset();
    cnt.select_count().from("tasks").where_eq("status", "'completed'");
    auto r3 = conn->query(cnt.build());
    if (r3 && r3->next()) {
        jb.key("completed").value(static_cast<double>(r3->get_int32(0)));
    }

    jb.end_object();
    return jb.build()->to_string();
}
