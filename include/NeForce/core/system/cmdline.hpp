#ifndef NEFORCE_CORE_SYSTEM_CMDLINE_HPP__
#define NEFORCE_CORE_SYSTEM_CMDLINE_HPP__

/**
 * @file cmdline.hpp
 * @brief 命令行参数解析工具
 *
 * 本文件提供了 cmdline 类，用于解析符合 POSIX 风格和 GNU 风格扩展的命令行参数。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
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
 *
 * 用于解析符合 POSIX 风格和 GNU 风格扩展的命令行参数。
 * 支持长选项（--option）、短选项（-o）、带值选项、多值选项、默认值、位置参数以及自动生成帮助信息。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下命令行接口与实用程序规范：
 *
 * **命令行接口规范：**
 * - **POSIX.1-2017 (IEEE Std 1003.1)**：第12章 — 实用程序约定（命令行参数语法）
 *   https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
 * - **POSIX.1-2017 Utility Syntax Guidelines**：实用程序语法指南（指南3-13）
 *   https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html#tag_12_02
 *
 * **GNU 扩展规范：**
 * - **GNU Coding Standards**：第4.6节 — 命令行接口标准
 *   https://www.gnu.org/prep/standards/html_node/Command_002dLine-Interfaces.html
 * - **GNU libc Manual**：第25.1.1节 — 程序参数语法约定
 *   https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html
 * - **GNU getopt_long(3)**：长选项扩展规范
 *   https://man7.org/linux/man-pages/man3/getopt.3.html
 *
 * **C/C++ 程序入口标准：**
 * - **ISO/IEC 9899:2018**：C 语言标准（§5.1.2.2.1 程序启动）
 *   https://www.iso.org/standard/74528.html
 * - **ISO/IEC 14882:2020**：C++ 编程语言标准（§6.9.3.1 main 函数）
 *   https://www.iso.org/standard/79358.html
 *
 * **帮助信息格式化标准：**
 * - **The Linux Man-pages Project**：手册页格式化规范
 *   https://man7.org/linux/man-pages/man7/man-pages.7.html
 *
 * @section option_syntax 选项语法规则
 * 根据 POSIX.1-2017 实用程序语法指南和 GNU 编码标准，支持的语法如下：
 *
 * **短选项（POSIX 指南3-5）：**
 * | 语法         | 示例          | 说明                                   |
 * |--------------|---------------|----------------------------------------|
 * | -o           | -v            | 单字符短选项                           |
 * | -o value     | -f input.txt  | 短选项带值（空格分隔）                 |
 * | -ovalue      | -finput.txt   | 短选项带值（无空格）                   |
 * | -abc         | -xvf          | 多个短选项组合（布尔型选项）           |
 * | -abco value  | -xvfo output  | 组合短选项，最后一个带值               |
 *
 * **长选项（GNU 扩展）：**
 * | 语法               | 示例               | 说明                         |
 * |--------------------|--------------------|------------------------------|
 * | --option           | --verbose          | 长选项（布尔型）             |
 * | --option=value     | --file=config.json | 长选项带值（等号分隔）       |
 * | --option value     | --file config.json | 长选项带值（空格分隔）       |
 *
 * **特殊参数（POSIX 指南10-11）：**
 * | 语法         | 说明                                   |
 * |--------------|----------------------------------------|
 * | --           | 选项结束标记，后续参数视为位置参数     |
 * | -            | 短横线单独出现，视为位置参数           |
 *
 * @section guideline_reference POSIX 实用程序语法指南
 * 本实现遵循的 POSIX 语法指南：
 *
 * | 指南编号 | 内容                                                       | 支持状态 |
 * |----------|------------------------------------------------------------|----------|
 * | 3        | 选项名应为单字符（-o）                                     | ✓        |
 * | 4        | 所有选项前应有 '-' 字符                                    | ✓        |
 * | 5        | 无参数的选项可以组合（-abc 等价于 -a -b -c）               | ✓        |
 * | 6        | 带参数的选项参数应用空格或直接跟随                         | ✓        |
 * | 7        | 选项参数不应可选（明确区分需要值和不需要值）               | ✓        |
 * | 8        | 选项应先于操作数出现                                       | ✓        |
 * | 9        | '--' 参数应被识别为选项结束标记                            | ✓        |
 * | 10       | 第一个 '--' 后的参数视为操作数（位置参数）                 | ✓        |
 * | 11       | '-' 作为操作数时不应被解释为选项                           | ✓        |
 *
 * @section gnu_extensions GNU 扩展特性
 * 本实现额外支持的 GNU 编码标准扩展：
 *
 * | 扩展特性           | 示例               | 说明                         |
 * |--------------------|--------------------|------------------------------|
 * | 长选项             | --help             | 多字符选项名                 |
 * | 长选项带等号       | --file=config.json | 等号分隔选项和值             |
 * | 选项值可选         | --color[=when]     | 通过 requires_value 控制     |
 * | 多值选项           | -I/usr/include -I/opt/include | allow_multiple 支持   |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 短选项字符        | ASCII 字母数字（a-z, A-Z, 0-9）           |
 * | 长选项名称        | 字母数字、连字符、下划线                  |
 * | 值分隔符          | 空格或 '='（仅长选项）                    |
 * | 选项结束标记      | '--'                                      |
 * | 最大选项数        | 无限制                                    |
 * | 多值选项存储      | vector<string>                            |
 * | 重复选项行为      | 覆盖或追加（由 allow_multiple 控制）      |
 *
 * @note 本实现同时支持 POSIX 标准的短选项语法和 GNU 标准的长选项扩展，
 *       适用于大多数命令行工具的开发。
 *
 * @warning 根据 POSIX 指南8，选项应先于位置参数出现。虽然本实现支持选项与位置参数
 *          混合出现（GNU 风格），但为了最大兼容性，建议将选项放在位置参数之前。
 *
 * @see https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
 * @see https://www.gnu.org/prep/standards/html_node/Command_002dLine-Interfaces.html
 * @see https://man7.org/linux/man-pages/man3/getopt.3.html
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
    struct NEFORCE_API option {
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

    /**
     * @enum conflict_behavior
     * @brief 约束冲突处理行为
     */
    enum class conflict_behavior {
        error,   /**< 冲突时抛出异常 */
        warning, /**< 冲突时打印警告 */
        ignore   /**< 冲突时忽略 */
    };

    /**
     * @struct option_dependency
     * @brief 选项依赖关系
     *
     * 若 present 选项被指定，则 required 选项也必须被指定。
     */
    struct option_dependency {
        string present;  /**< 触发依赖的选项名 */
        string required; /**< 必须同时出现的选项名 */
    };

    /**
     * @struct option_conflict
     * @brief 选项冲突关系
     *
     * option_a 和 option_b 不能同时出现。
     */
    struct option_conflict {
        string option_a; /**< 冲突选项A */
        string option_b; /**< 冲突选项B */
    };

    /**
     * @brief 选项值验证器类型
     * @param name 选项名称
     * @param value 选项值
     * @return 验证通过返回空字符串，失败返回错误信息
     */
    using validator = function<string(const string& name, const string& value)>;

private:
    string program_name_;                        ///< 程序名称
    vector<option> options_;                     ///< 选项列表
    unordered_map<string, size_t> options_long_; ///< 长选项名索引
    unordered_map<char, size_t> options_short_;  ///< 短选项字符索引
    vector<string> positional_;                  ///< 位置参数

    vector<option_dependency> dependencies_;                        ///< 选项依赖关系列表
    vector<option_conflict> conflicts_;                             ///< 选项冲突关系列表
    conflict_behavior conflict_behavior_{conflict_behavior::error}; ///< 冲突处理行为
    vector<validator> validators_;                                  ///< 选项值验证器列表

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

    /**
     * @brief 验证选项值
     * @throws cmdline_exception 验证失败时抛出
     *
     * 依次调用所有已注册的验证器。
     */
    void validate_values();

    /**
     * @brief 验证选项约束
     * @throws cmdline_exception 约束违反且行为为 error 时抛出
     *
     * 检查所有依赖关系和冲突关系。
     */
    void validate_constraints();

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
    void parse(int argc, const char* argv[]);

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
    NEFORCE_NODISCARD string get(const string& long_name, size_t index = 0) const;

    /**
     * @brief 检查选项是否存在
     * @param name 长选项名称
     * @return 是否在命令行中指定了该选项
     */
    NEFORCE_NODISCARD bool has(const string& name) const;

    /**
     * @brief 获取选项出现的次数
     * @param name 长选项名称
     * @return 选项出现次数
     */
    NEFORCE_NODISCARD size_t count(const string& name) const;

    /**
     * @brief 获取位置参数
     * @return 位置参数向量引用
     */
    NEFORCE_NODISCARD const vector<string>& positional_args() const noexcept { return positional_; }

    /**
     * @brief 获取程序名称
     * @return 程序名称
     */
    NEFORCE_NODISCARD const string& program_name() const noexcept { return program_name_; }

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
    NEFORCE_NODISCARD static vector<string> get_os_argv();

    /**
     * @brief 添加选项依赖关系
     * @param present 当此选项出现时
     * @param required 此选项也必须出现
     */
    void add_dependency(const string& present, const string& required);

    /**
     * @brief 添加选项冲突关系
     * @param option_a 选项A
     * @param option_b 选项B
     */
    void add_conflict(const string& option_a, const string& option_b);

    /**
     * @brief 设置冲突处理行为
     * @param behavior 冲突时的处理方式
     */
    void set_conflict_behavior(conflict_behavior behavior) noexcept { conflict_behavior_ = behavior; }

    /**
     * @brief 从配置文件加载选项值
     * @param config_path 配置文件路径
     * @param section 配置节名或嵌套键名
     * @throws cmdline_exception 文件不存在或解析失败时抛出
     *
     * 支持的文件格式：
     * - .ini：INI 格式，通过 section 参数指定节
     * - .json：JSON 格式，通过 section 参数指定顶层键导航到嵌套对象
     * - .toml：TOML 格式，通过 section 参数指定顶层键导航到嵌套表
     * - .yaml / .yml：YAML 格式，通过 section 参数指定顶层键导航到嵌套映射
     * - .env：环境变量格式（section 参数忽略）
     *
     * 配置文件中的值仅填充命令行未指定的选项，命令行参数优先级更高。
     */
    void load_config(const string& config_path, const string& section = "");

    /**
     * @brief 添加选项验证器
     * @param v 验证器函数
     *
     * 可以多次调用以添加多个验证器，所有验证器都会被依次调用。
     * 验证在 parse() 完成后、validate_constraints() 之前执行。
     */
    void add_validator(validator v);

    /**
     * @brief 添加选项值范围验证器（数值范围）
     * @param long_name 选项名称
     * @param min_val 最小值（包含）
     * @param max_val 最大值（包含）
     * @throws cmdline_exception 选项不存在时抛出
     */
    void add_range_validator(string long_name, int64_t min_val, int64_t max_val);

    /**
     * @brief 添加选项值正则验证器
     * @param long_name 选项名称
     * @param pattern 正则表达式模式
     * @throws cmdline_exception 选项不存在时抛出
     */
    void add_regex_validator(string long_name, const string& pattern);
};

/** @} */ // CommandLine

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_CMDLINE_HPP__
