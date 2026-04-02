#ifndef NEFORCE_CORE_FILE_PATH_TREE_HPP__
#define NEFORCE_CORE_FILE_PATH_TREE_HPP__

/**
 * @file path_tree.hpp
 * @brief 文件路径树类
 *
 * 提供路径树结构，用于表示目录层级关系，
 * 支持树的遍历、查找、过滤等操作，便于后续文件操作。
 */

#include "NeForce/core/file/path.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/weak_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class path_tree
 * @brief 文件路径树
 *
 * 以树形结构表示文件系统中的目录层次。每个节点对应一个路径，
 * 可包含子节点（子目录或文件）。支持：
 * - 从文件系统扫描构建树
 * - 手动构建树
 * - 深度/广度优先遍历
 * - 过滤器筛选节点
 * - 路径查找
 * - 树的合并与裁剪
 */
class NEFORCE_API path_tree : public istringify<path_tree> {
public:
    /**
     * @brief 节点类型枚举
     */
    enum class node_type : uint8_t {
        unknown   = 0,  ///< 未知
        directory = 1,  ///< 目录
        file      = 2,  ///< 普通文件
        symlink   = 3,  ///< 符号链接
    };

    /**
     * @brief 遍历控制指令
     */
    enum class visit_result : uint8_t {
        proceed     = 0,  ///< 继续遍历
        skip        = 1,  ///< 跳过当前节点的子节点
        stop        = 2,  ///< 终止整个遍历
    };

    /**
     * @class node
     * @brief 路径树节点
     *
     * 表示路径树中的一个节点，包含路径信息及子节点列表。
     * 节点以共享所有权的方式持有子节点，父节点以弱引用持有。
     */
    class node : public enable_shared_from_this<node> {
        friend class path_tree;

    public:
        using ptr = shared_ptr<node>;       ///< 节点共享指针
        using weak_ptr = weak_ptr<node>;    ///< 节点弱指针
        using children_list = vector<ptr>;  ///< 子节点列表

    private:
        path path_;                 ///< 节点对应路径
        node_type type_;            ///< 节点类型
        weak_ptr parent_;           ///< 父节点
        children_list children_;    ///< 子节点列表
        size_t depth_;              ///< 节点深度

    public:
        node() = default;

        /**
         * @brief 构造节点
         * @param p 节点路径
         * @param type 节点类型
         * @param depth 节点深度
         */
        explicit node(path p, node_type type, size_t depth = 0)
        : path_(_NEFORCE move(p)), type_(type), depth_(depth) {}

        node(const node&) = delete;
        node& operator =(const node&) = delete;
        node(node&&) noexcept = default;
        node& operator =(node&&) noexcept = default;

        /** @brief 获取路径 */
        NEFORCE_NODISCARD const path& get_path() const noexcept { return path_; }

        /** @brief 获取节点类型 */
        NEFORCE_NODISCARD node_type type() const noexcept { return type_; }

        /** @brief 获取节点深度 */
        NEFORCE_NODISCARD size_t depth() const noexcept { return depth_; }

        /** @brief 是否为目录 */
        NEFORCE_NODISCARD bool is_directory() const noexcept {
            return type_ == node_type::directory;
        }

        /** @brief 是否为文件 */
        NEFORCE_NODISCARD bool is_file() const noexcept {
            return type_ == node_type::file || type_ == node_type::symlink;
        }

        /** @brief 是否为根节点 */
        NEFORCE_NODISCARD bool is_root() const noexcept {
            return parent_.expired();
        }

        /** @brief 是否为叶节点（无子节点） */
        NEFORCE_NODISCARD bool is_leaf() const noexcept {
            return children_.empty();
        }

        /** @brief 获取父节点 */
        NEFORCE_NODISCARD ptr parent() const noexcept {
            return parent_.lock();
        }

        /** @brief 获取子节点列表 */
        NEFORCE_NODISCARD const children_list& children() const noexcept {
            return children_;
        }

        /** @brief 获取子节点数量 */
        NEFORCE_NODISCARD size_t child_count() const noexcept {
            return children_.size();
        }

        /**
         * @brief 按名称查找直接子节点
         * @param name 文件名
         * @return 找到的子节点，未找到返回nullptr
         */
        NEFORCE_NODISCARD ptr find_child(string_view name) const noexcept;

        /**
         * @brief 添加子节点
         * @param child 子节点共享指针
         */
        void add_child(ptr child);

        /**
         * @brief 移除指定子节点
         * @param name 要移除的子节点名称
         * @return 是否成功移除
         */
        bool remove_child(string_view name);
    };

    /**
     * @brief 节点访问回调类型
     *
     * 参数为当前节点引用，返回 visit_result 控制遍历行为。
     */
    using visitor = function<visit_result(const node&)>;

    /**
     * @brief 节点过滤器类型
     *
     * 返回 true 表示节点通过过滤，false 表示被过滤掉。
     */
    using filter = function<bool(const node&)>;

    /**
     * @struct scan_options
     * @brief 文件系统扫描选项
     */
    struct scan_options {
        /** @brief 最大扫描深度，0表示无限制 */
        size_t max_depth = 0;

        /** @brief 是否包含隐藏文件 */
        bool include_hidden = false;

        /** @brief 是否跟随符号链接 */
        bool follow_symlinks = false;

        /** @brief 是否只包含文件（不包含目录节点） */
        bool files_only = false;

        /** @brief 是否只包含目录 */
        bool dirs_only = false;

        /** @brief 扩展名过滤 */
        vector<string> extensions;

        /** @brief 自定义过滤器 */
        filter custom_filter;
    };

private:
    node::ptr root_;  ///< 根节点

    static void scan_impl(const node::ptr& parent, const scan_options& options, size_t current_depth);
    static void traverse_dfs_impl(const node::ptr& current, const visitor& v, bool& stopped);
    static void collect_impl(const node::ptr& current, const filter& f, vector<node::ptr>& result);
    static node::ptr clone_node(const node::ptr& src, const node::ptr& new_parent, size_t depth);

    static node::ptr prune_impl(const node::ptr& src, const filter& f,
                                const node::ptr& new_parent, size_t depth);

    string to_string_impl(const node::ptr& n, string_view indent, size_t depth) const;

public:
    path_tree() = default;

    /**
     * @brief 从路径构造树
     * @param root_path 根路径
     */
    explicit path_tree(path root_path)
    : root_(make_shared<node>(_NEFORCE move(root_path), node_type::directory, 0)) {}

    path_tree(const path_tree&) = default;
    path_tree(path_tree&&) noexcept = default;
    path_tree& operator =(const path_tree&) = default;
    path_tree& operator =(path_tree&&) noexcept = default;

    /**
     * @brief 从文件系统扫描构建路径树
     * @param root 扫描的根路径
     * @param options 扫描选项
     * @return 构建好的路径树
     *
     * 递归扫描给定路径下的所有文件和目录，
     * 根据选项进行过滤和深度控制。
     */
    NEFORCE_NODISCARD static path_tree scan(
        const path& root,
        const scan_options& options = {}
    );

    /** @brief 获取根节点 */
    NEFORCE_NODISCARD node::ptr root() const noexcept { return root_; }

    /** @brief 树是否为空 */
    NEFORCE_NODISCARD bool empty() const noexcept { return !root_; }

    /**
     * @brief 获取树中节点总数
     * @return 节点数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept;

    /**
     * @brief 获取树的最大深度
     * @return 最大深度
     */
    NEFORCE_NODISCARD size_t max_depth() const noexcept;

    /**
     * @brief 通过绝对路径查找节点
     * @param p 要查找的路径
     * @return 找到的节点，未找到返回nullptr
     */
    NEFORCE_NODISCARD node::ptr find(const path& p) const noexcept;

    /**
     * @brief 通过文件名查找所有匹配节点
     * @param name 文件名
     * @return 所有匹配节点的列表
     */
    NEFORCE_NODISCARD vector<node::ptr> find_all(string_view name) const;

    /**
     * @brief 通过过滤器查找所有匹配节点
     * @param f 过滤器
     * @return 所有满足条件的节点列表
     */
    NEFORCE_NODISCARD vector<node::ptr> find_if(const filter& f) const;

    /**
     * @brief 通过扩展名查找所有文件节点
     * @param ext 扩展名
     * @return 所有匹配的文件节点列表
     */
    NEFORCE_NODISCARD vector<node::ptr> find_by_extension(string_view ext) const;

    /**
     * @brief 深度优先遍历（前序）
     * @param visitor 访问回调
     *
     * 对树中每个节点调用visitor，
     * 根据返回值控制遍历行为（proceed/skip/stop）。
     */
    void traverse_dfs(const visitor& visitor) const;

    /**
     * @brief 广度优先遍历
     * @param visitor 访问回调
     */
    void traverse_bfs(const visitor& visitor) const;

    /**
     * @brief 仅遍历文件节点（深度优先）
     * @param visitor 访问回调
     */
    void traverse_files(const visitor& visitor) const;

    /**
     * @brief 仅遍历目录节点（深度优先）
     * @param visitor 访问回调
     */
    void traverse_dirs(const visitor& visitor) const;

    /**
     * @brief 手动插入路径节点
     * @param p 要插入的路径（相对或绝对）
     * @param type 节点类型
     * @return 插入的节点，如果路径已存在则返回已有节点
     *
     * 会自动创建路径中缺失的中间目录节点。
     */
    node::ptr insert(const path& p, node_type type = node_type::file);

    /**
     * @brief 移除指定路径的节点及其子树
     * @param p 要移除的路径
     * @return 是否成功移除
     */
    bool remove(const path& p);

    /**
     * @brief 将另一棵树合并到当前树
     * @param other 要合并的树（根路径需与当前树兼容）
     *
     * 将other中的所有节点合并入当前树，重复节点不覆盖。
     */
    void merge(const path_tree& other);

    /**
     * @brief 获取以指定路径为根的子树
     * @param p 子树根路径
     * @return 子树
     *
     * 返回以p为根的独立路径树副本。
     */
    NEFORCE_NODISCARD path_tree subtree(const path& p) const;

    /**
     * @brief 按过滤器裁剪树，返回只含满足条件节点的新树
     * @param f 过滤器
     * @return 裁剪后的新树
     *
     * 若某目录下所有子节点均被过滤，该目录节点也被移除。
     */
    NEFORCE_NODISCARD path_tree prune(const filter& f) const;

    /**
     * @brief 收集树中所有路径
     * @return 所有节点路径列表（深度优先顺序）
     */
    NEFORCE_NODISCARD vector<path> all_paths() const;

    /**
     * @brief 收集树中所有文件路径
     * @return 所有文件节点的路径列表
     */
    NEFORCE_NODISCARD vector<path> all_file_paths() const;

    /**
     * @brief 收集树中所有目录路径
     * @return 所有目录节点的路径列表
     */
    NEFORCE_NODISCARD vector<path> all_dir_paths() const;

    /**
     * @brief 将树格式化为可读字符串
     * @return 格式化后的字符串
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 将树格式化为可读字符串
     * @param indent 每层缩进字符串，默认为两个空格
     * @return 格式化后的字符串
     */
    NEFORCE_NODISCARD string to_string(string_view indent) const;
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_PATH_TREE_HPP__
