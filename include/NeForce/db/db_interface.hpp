#ifndef NEFORCE_DATABASE_DB_INTERFACE_HPP__
#define NEFORCE_DATABASE_DB_INTERFACE_HPP__

/**
 * @file db_interface.hpp
 * @brief 数据库抽象接口层
 *
 * 此文件定义了数据库访问的抽象接口，支持关系型数据库（SQL）和键值存储（NoSQL）。
 * 通过统一的接口抽象，实现对不同数据库后端的透明访问。
 *
 * 设计目标：
 * - 统一的关系型数据库访问接口
 * - 统一的键值存储访问接口
 * - 支持预处理语句和参数绑定
 * - 支持连接池和连接管理
 * - 工厂模式创建具体实现
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/db/db_config.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @defgroup DatabaseInterface 数据库接口
 * @brief 数据库相关功能的接口
 * @{
 */

/**
 * @struct idb_result
 * @brief 数据库结果集抽象基类
 *
 * 提供结果集的基本迭代接口，支持遍历查询结果。
 */
struct NEFORCE_API idb_result {
    using size_type = size_t;          ///< 大小类型
    using difference_type = ptrdiff_t; ///< 差值类型

    virtual ~idb_result() = default;

    /**
     * @brief 检查结果集是否为空
     * @return 无数据返回true
     */
    virtual bool empty() const = 0;

    /**
     * @brief 移动到下一行
     * @return 成功移动返回true，到达末尾返回false
     */
    virtual bool next() = 0;
};

/**
 * @struct idb_tb_result
 * @brief 关系型数据库结果集抽象基类
 *
 * 提供表格数据的访问接口，支持按列名或索引获取值。
 */
struct NEFORCE_API idb_tb_result : idb_result {
    ~idb_tb_result() override = default;

    /**
     * @brief 获取结果集行数
     * @return 行数
     */
    virtual size_type row_count() const = 0;

    /**
     * @brief 获取结果集列数
     * @return 列数
     */
    virtual size_type column_count() const = 0;

    /**
     * @brief 获取所有列名
     * @return 列名列表（按查询顺序）
     */
    virtual const vector<string_view>& column_names() const = 0;

    virtual string_view get(size_type n) const = 0; ///< 字符串

    virtual bool get_bool(size_type n) const = 0;           ///< 布尔值
    virtual int16_t get_int16(size_type n) const = 0;       ///< 16位整数
    virtual int32_t get_int32(size_type n) const = 0;       ///< 32位整数
    virtual int64_t get_int64(size_type n) const = 0;       ///< 64位整数
    virtual float32_t get_float32(size_type n) const = 0;   ///< 32位浮点数
    virtual float64_t get_float64(size_type n) const = 0;   ///< 64位浮点数
    virtual decimal_t get_decimal(size_type n) const = 0;   ///< 高精度十进制数
    virtual vector<char> get_blob(size_type n) const = 0;   ///< BLOB二进制数据
    virtual uint64_t get_bit(size_type n) const = 0;        ///< 位字段值
    virtual date get_date(size_type n) const = 0;           ///< 日期类型
    virtual time get_time(size_type n) const = 0;           ///< 时间类型
    virtual datetime get_datetime(size_type n) const = 0;   ///< 日期时间类型
    virtual timestamp get_timestamp(size_type n) const = 0; ///< 时间戳类型
};

/**
 * @struct idb_kv_result
 * @brief 键值存储结果集抽象基类
 *
 * 提供键值对数据的访问接口。
 */
struct NEFORCE_API idb_kv_result : idb_result {
    ~idb_kv_result() override = default;

    virtual string_view key() const = 0;   ///< 获取键
    virtual string_view value() const = 0; ///< 获取字符串值

    virtual bool value_bool() const = 0;                                ///< 获取布尔值
    virtual int64_t value_int64() const = 0;                            ///< 获取64位整数值
    virtual double value_double() const = 0;                            ///< 获取浮点值
    virtual vector<string> value_array() const = 0;                     ///< 获取数组值
    virtual const vector<pair<string, string>>& value_hash() const = 0; ///< 获取哈希表值
};

/**
 * @struct idb_prepared_result
 * @brief 预处理语句执行结果
 *
 * 表示预处理语句查询的结果集。
 */
struct NEFORCE_API idb_prepared_result : idb_tb_result {
    ~idb_prepared_result() override = default;
};

/**
 * @struct idb_prepared_statement
 * @brief 预处理语句抽象基类
 *
 * 提供参数绑定和执行的接口，支持SQL注入防护和性能优化。
 */
struct NEFORCE_API idb_prepared_statement {
    virtual ~idb_prepared_statement() = default;

    /**
     * @brief 获取参数数量
     * @return 占位符数量
     */
    virtual uint32_t param_count() const noexcept = 0;

    virtual bool bind_param(uint32_t index, const string& value) = 0;             ///< 绑定字符串
    virtual bool bind_param(uint32_t index, string_view value) = 0;               ///< 绑定字符串视图
    virtual bool bind_param(uint32_t index, const char* value) = 0;               ///< 绑定C字符串
    virtual bool bind_param(uint32_t index, int32_t value) = 0;                   ///< 绑定32位整数
    virtual bool bind_param(uint32_t index, int64_t value) = 0;                   ///< 绑定64位整数
    virtual bool bind_param(uint32_t index, float64_t value) = 0;                 ///< 绑定浮点数
    virtual bool bind_param(uint32_t index, const void* data, size_t length) = 0; ///< 绑定二进制数据

    /**
     * @brief 执行非查询语句（UPDATE/INSERT/DELETE）
     * @return 执行成功返回true
     */
    virtual bool execute() = 0;

    /**
     * @brief 执行查询语句（SELECT）
     * @return 查询结果集
     */
    virtual unique_ptr<idb_prepared_result> execute_query() = 0;

    /**
     * @brief 获取错误信息
     * @return 错误描述字符串
     */
    virtual string_view get_error() const noexcept = 0;

    /**
     * @brief 获取错误码
     * @return 数据库错误码
     */
    virtual uint32_t get_errno() const noexcept = 0;
};

/**
 * @struct idb_connect
 * @brief 数据库连接抽象基类
 *
 * 提供连接管理、错误处理、存活检测等基础功能。
 */
struct NEFORCE_API idb_connect {
public:
    using clock_type = milliseconds; ///< 存活时间计时器类型

private:
    clock_type alive_time_{0}; ///< 连接存活时间（毫秒）

public:
    virtual ~idb_connect() = default;

    /**
     * @name 连接管理
     * @{
     */
    virtual bool connect(const db_config& config) = 0;   ///< 建立连接
    virtual bool reconnect(const db_config& config) = 0; ///< 重新连接
    virtual void close() = 0;                            ///< 关闭连接
    /** @} */

    /**
     * @name 字符集设置
     * @{
     */
    virtual bool set_character_set(const string& encoding) const = 0; ///< 设置字符集
    virtual string_view get_character_set() const = 0;                ///< 获取字符集
    /** @} */

    /**
     * @name 错误处理
     * @{
     */
    virtual string_view get_error() const = 0; ///< 获取错误信息
    virtual uint32_t get_errno() const = 0;    ///< 获取错误码
    /** @} */

    /**
     * @brief 执行非查询SQL语句
     * @param sql SQL语句
     * @return 执行成功返回true
     */
    virtual bool update(const string& sql) const = 0;

    /**
     * @name 连接状态检测
     * @{
     */
    virtual bool connected() const = 0; ///< 检查连接状态
    virtual bool is_valid() const = 0;  ///< 检查连接有效性
    /** @} */

    /**
     * @brief 刷新连接存活时间
     *
     * 标记当前连接为活动状态，用于连接池空闲检测。
     */
    void refresh_alive() noexcept { alive_time_ = time_cast<milliseconds>(steady_clock::now().since_epoch()); }

    /**
     * @brief 获取连接空闲时间
     * @return 自上次刷新以来的毫秒数
     */
    NEFORCE_NODISCARD clock_type get_alive() const noexcept {
        return time_cast<milliseconds>(steady_clock::now().since_epoch()) - alive_time_;
    }
};

/**
 * @struct idb_tb_connect
 * @brief 关系型数据库连接抽象基类
 *
 * 提供表格数据查询和预处理语句功能。
 */
struct NEFORCE_API idb_tb_connect : idb_connect {
    ~idb_tb_connect() override = default;

    /**
     * @brief 执行查询SQL语句
     * @param sql SELECT语句
     * @return 查询结果集
     */
    virtual unique_ptr<idb_tb_result> query(const string& sql) const = 0;

    /**
     * @brief 创建预处理语句
     * @param sql 带占位符的SQL语句
     * @return 预处理语句对象
     */
    virtual unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const = 0;
};

/**
 * @struct idb_kv_connect
 * @brief 键值存储连接抽象基类
 *
 * 提供键值对操作的完整接口，支持字符串、哈希、列表、集合等数据结构。
 */
struct NEFORCE_API idb_kv_connect : idb_connect {
    ~idb_kv_connect() override = default;

    /**
     * @brief 执行键值存储查询命令
     * @param sql 命令字符串
     * @return 执行结果
     */
    virtual unique_ptr<idb_kv_result> query(const string& sql) const = 0;

    virtual bool set(const string& key, const string& value) = 0;                ///< 设置键值
    virtual bool setex(const string& key, const string& value, int seconds) = 0; ///< 设置键值并指定过期时间
    virtual unique_ptr<idb_kv_result> get(const string& key) = 0;                ///< 获取键值
    virtual bool del(const string& key) = 0;                                     ///< 删除键
    virtual bool exists(const string& key) = 0;                                  ///< 检查键是否存在
    virtual bool expire(const string& key, int seconds) = 0;                     ///< 设置过期时间

    virtual bool hset(const string& key, const string& field, const string& value) = 0; ///< 设置哈希字段
    virtual unique_ptr<idb_kv_result> hget(const string& key, const string& field) = 0; ///< 获取哈希字段
    virtual unique_ptr<idb_kv_result> hgetall(const string& key) = 0;                   ///< 获取所有哈希字段

    virtual bool lpush(const string& key, const string& value) = 0;                       ///< 左推入列表
    virtual bool rpush(const string& key, const string& value) = 0;                       ///< 右推入列表
    virtual unique_ptr<idb_kv_result> lrange(const string& key, int start, int stop) = 0; ///< 获取列表范围

    virtual bool sadd(const string& key, const string& member) = 0;    ///< 添加集合成员
    virtual unique_ptr<idb_kv_result> smembers(const string& key) = 0; ///< 获取所有集合成员
};

/**
 * @class idb_factory
 * @brief 数据库连接工厂抽象基类
 *
 * 使用工厂模式创建具体的数据库连接和结果集对象。
 * 每种数据库实现需要提供对应的工厂子类。
 */
class NEFORCE_API idb_factory {
protected:
    db_config config_; ///< 数据库配置

public:
    /**
     * @brief 构造函数
     * @param config 数据库配置
     */
    explicit idb_factory(db_config config) :
    config_(move(config)) {}

    virtual ~idb_factory() = default;

    /**
     * @brief 创建数据库连接对象
     * @return 连接对象指针
     */
    virtual idb_connect* create_connect() = 0;

    /**
     * @brief 创建结果集对象
     * @param native_result 原生数据库结果集句柄
     * @return 结果集对象指针
     */
    virtual idb_result* create_result(void* native_result) = 0;
};

/** @} */ // DatabaseInterface

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_DB_INTERFACE_HPP__
