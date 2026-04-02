#ifndef NEFORCE_CORE_SYSTEM_CMDLINE_HPP__
#define NEFORCE_CORE_SYSTEM_CMDLINE_HPP__
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct cmdline_exception
 * @brief 命令行解析异常
 */
struct cmdline_exception final : system_exception {
    explicit cmdline_exception(const char* info = "CmdLine Operation Failed.", const char* type = static_type,
                               const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit cmdline_exception(const exception& e) :
    system_exception(e) {}

    ~cmdline_exception() override = default;
    static constexpr auto static_type = "cmdline_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup CommandLine 命令行
 * @brief 命令行参数解析工具
 * @{
 */

/**
 * @class cmdline
 * @brief 命令行参数解析器
 *
 * 支持解析符合POSIX风格和GNU风格扩展的命令行参数。
 * 特性：
 * - 长选项（--option）和短选项（-o）
 * - 选项值（--option=value 或 -o value）
 * - 多值选项
 * - 默认值
 * - 位置参数
 * - 自动生成帮助信息
 */
class NEFORCE_API cmdline {
public:
    /**
     * @struct option
     * @brief 选项定义结构
     *
     * 描述一个命令行选项的所有属性。
     */
    struct option {
        string long_name;            ///< 长选项名称
        char short_name = 0;         ///< 短选项字符
        string description;          ///< 选项描述
        bool requires_value = false; ///< 是否需要值
        bool allow_multiple = false; ///< 是否允许多次出现
        string default_value;        ///< 默认值

        vector<string> values; ///< 解析得到的值列表

        option() = default;

        /**
         * @brief 构造函数
         * @param lname 长选项名称
         * @param sname 短选项字符
         * @param desc 描述
         * @param req_val 是否需要值
         * @param allow_multi 是否允许多次出现
         * @param def_val 默认值
         */
        option(string lname, char sname, string desc, bool req_val, bool allow_multi, string def_val);
    };

private:
    string program_name_;                         ///< 程序名称
    vector<option> options_;                      ///< 选项列表
    unordered_map<string, option*> options_long_; ///< 长选项名到选项的映射
    unordered_map<char, option*> options_short_;  ///< 短选项字符到选项的映射
    vector<string> positional_;                   ///< 位置参数

    /**
     * @brief 根据长选项名查找选项
     * @param name 长选项名
     * @return 选项指针，未找到返回nullptr
     */
    option* find_option_long(const string& name);

    /**
     * @brief 根据短选项字符查找选项
     * @param name 短选项字符
     * @return 选项指针，未找到返回nullptr
     */
    option* find_option_short(char name);

    /**
     * @brief 解析长选项（--option）
     * @param arg 当前参数
     * @param args 所有参数列表
     * @param index 当前索引
     * @throws cmdline_exception 解析失败时抛出
     */
    void parse_long_option(const string& arg, const vector<string>& args, size_t& index);

    /**
     * @brief 解析短选项组合（-abc）
     * @param arg 当前参数
     * @param args 所有参数列表
     * @param index 当前索引
     * @throws cmdline_exception 解析失败时抛出
     */
    void parse_short_options(const string& arg, const vector<string>& args, size_t& index);

public:
    /**
     * @brief 添加选项定义
     * @param long_name 长选项名称
     * @param short_name 短选项字符
     * @param description 选项描述
     * @param requires_value 是否需要值
     * @param allow_multiple 是否允许多次出现
     * @param default_value 默认值
     * @throws cmdline_exception 选项名重复时抛出
     */
    void add_option(const string& long_name, char short_name, const string& description, bool requires_value = false,
                    bool allow_multiple = false, const string& default_value = "");

    /**
     * @brief 解析操作系统提供的命令行参数
     * @throws cmdline_exception 解析失败时抛出
     */
    void parse_os_args();

    /**
     * @brief 解析main函数参数
     * @param argc 参数个数
     * @param argv 参数数组
     * @throws cmdline_exception 解析失败时抛出
     */
    void parse(int argc, char* argv[]);

    /**
     * @brief 解析字符串向量参数
     * @param args 参数字符串向量
     * @throws cmdline_exception 解析失败时抛出
     */
    void parse(const vector<string>& args);

    /**
     * @brief 获取选项值
     * @param long_name 长选项名称
     * @param index 对于多值选项，指定索引
     * @return 选项值
     * @throws cmdline_exception 选项不存在或无值时抛出
     */
    string get(const string& long_name, size_t index = 0) const;

    /**
     * @brief 检查选项是否存在
     * @param name 长选项名称
     * @return 是否在命令行中指定了该选项
     */
    bool has(const string& name) const;

    /**
     * @brief 获取选项出现的次数
     * @param name 长选项名称
     * @return 选项出现次数
     */
    size_t count(const string& name) const;

    /**
     * @brief 获取位置参数
     * @return 位置参数向量引用
     */
    const vector<string>& positional_args() const noexcept { return positional_; }

    /**
     * @brief 获取程序名称
     * @return 程序名称
     */
    const string& program_name() const noexcept { return program_name_; }

    /**
     * @brief 打印帮助信息
     *
     * 根据所有注册的选项自动生成格式化的帮助文本。
     */
    void print_help() const;

    /**
     * @brief 获取操作系统提供的命令行参数
     * @return 参数字符串向量
     * @throws cmdline_exception 获取失败时抛出
     */
    static vector<string> get_os_argv();
};

/** @} */ // CommandLine

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_CMDLINE_HPP__
