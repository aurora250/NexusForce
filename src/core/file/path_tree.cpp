#include <NeForce/core/file/path_tree.hpp>
#include <NeForce/core/container/queue.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <NeForce/core/config/windef.hpp>
#include <windef.h>
#include <WinBase.h>
#include <fileapi.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <sys/stat.h>
#include <dirent.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

path_tree::node::ptr path_tree::node::find_child(const string_view name) const noexcept {
    for (const auto& child : children_) {
        if (child->get_path().filename() == name) {
            return child;
        }
    }
    return nullptr;
}

void path_tree::node::add_child(ptr child) {
    child->parent_ = weak_ptr(child);
    children_.push_back(_NEFORCE move(child));
}

bool path_tree::node::remove_child(const string_view name) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if ((*it)->get_path().filename() == name) {
            children_.erase(it);
            return true;
        }
    }
    return false;
}

path_tree path_tree::scan(const path& root, const scan_options& options) {
    path_tree tree;

    if (!root.exists()) return tree;

    const node_type root_type = root.is_directory()
        ? node_type::directory
        : node_type::file;

    tree.root_ = make_shared<node>(root.absolute(), root_type, 0);

    if (root_type == node_type::directory) {
        scan_impl(tree.root_, options, 1);
    }

    return tree;
}

void path_tree::scan_impl(
    const node::ptr&    parent,
    const scan_options& options,
    const size_t        current_depth
) {
    if (options.max_depth > 0 && current_depth > options.max_depth) return;

    const path& dir = parent->get_path();

#ifdef NEFORCE_PLATFORM_WINDOWS

    WIN32_FIND_DATAA fdata;
    const path search_path = dir / "*";
    const HANDLE hFind = ::FindFirstFileA(search_path.data(), &fdata);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        const string_view name(fdata.cFileName);
        if (name == "." || name == "..") continue;

        if (!options.include_hidden && !name.empty() && name.front() == '.') continue;

        const path child_path = dir / name;
        const bool is_dir = (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        node_type type = is_dir ? node_type::directory : node_type::file;

        const bool type_ok = !(options.files_only && is_dir) &&
                             !(options.dirs_only  && !is_dir);

        bool ext_ok = true;
        if (!is_dir && !options.extensions.empty()) {
            const string_view ext = child_path.extension();
            ext_ok = false;
            for (const auto& e : options.extensions) {
                if (ext == e.view()) { ext_ok = true; break; }
            }
        }

        auto child_node = make_shared<node>(child_path, type, current_depth);
        child_node->parent_ = parent;

        const bool custom_ok = !options.custom_filter ||
                                options.custom_filter(*child_node);

        if (type_ok && ext_ok && custom_ok) {
            parent->children_.push_back(child_node);
        }

        if (is_dir) {
            node::ptr& recurse_node =
                (type_ok && ext_ok && custom_ok)
                    ? parent->children_.back()
                    : child_node;
            scan_impl(recurse_node, options, current_depth + 1);
        }

    } while (::FindNextFileA(hFind, &fdata));

    ::FindClose(hFind);

#else
    ::DIR* dp = ::opendir(dir.data());
    if (!dp) return;

    struct ::dirent* entry;
    while ((entry = ::readdir(dp)) != nullptr) {
        const string_view name(entry->d_name);
        if (name == "." || name == "..") continue;

        if (!options.include_hidden && !name.empty() && name.front() == '.') continue;

        const path child_path = dir / name;

        node_type type = node_type::unknown;
        bool is_dir = false;

#ifdef _DIRENT_HAVE_D_TYPE
        if (entry->d_type == DT_DIR) {
            type   = node_type::directory;
            is_dir = true;
        } else if (entry->d_type == DT_LNK) {
            type = node_type::symlink;
            if (options.follow_symlinks) {
                is_dir = child_path.is_directory();
                if (is_dir) type = node_type::directory;
            }
        } else if (entry->d_type == DT_REG) {
            type = node_type::file;
        } else {
            struct ::stat64 st{};
            if (::stat64(child_path.data(), &st) == 0) {
                if (S_ISDIR(st.st_mode))      { type = node_type::directory; is_dir = true; }
                else if (S_ISREG(st.st_mode)) { type = node_type::file; }
                else if (S_ISLNK(st.st_mode)) { type = node_type::symlink; }
            }
        }
#else
        struct ::stat64 st{};
        if (::stat64(child_path.data(), &st) == 0) {
            if      (S_ISDIR(st.st_mode)) { type = node_type::directory; is_dir = true; }
            else if (S_ISREG(st.st_mode)) { type = node_type::file; }
            else if (S_ISLNK(st.st_mode)) { type = node_type::symlink; }
        }
#endif

        const bool type_ok = !(options.files_only && is_dir) &&
                             !(options.dirs_only  && !is_dir);

        bool ext_ok = true;
        if (!is_dir && !options.extensions.empty()) {
            const string_view ext = child_path.extension();
            ext_ok = false;
            for (const auto& e : options.extensions) {
                if (ext == e.view()) { ext_ok = true; break; }
            }
        }

        auto child_node = make_shared<node>(child_path, type, current_depth);
        child_node->parent_ = parent;

        const bool custom_ok = !options.custom_filter ||
                                options.custom_filter(*child_node);

        if (type_ok && ext_ok && custom_ok) {
            parent->children_.push_back(child_node);
        }

        if (is_dir) {
            node::ptr& recurse_node =
                (type_ok && ext_ok && custom_ok)
                    ? parent->children_.back()
                    : child_node;
            scan_impl(recurse_node, options, current_depth + 1);
        }
    }

    ::closedir(dp);
#endif
}

size_t path_tree::size() const noexcept {
    if (!root_) return 0;
    size_t count = 0;
    traverse_dfs([&](const node& /*n*/) -> visit_result {
        ++count;
        return visit_result::proceed;
    });
    return count;
}

size_t path_tree::max_depth() const noexcept {
    if (!root_) return 0;
    size_t max_d = 0;
    traverse_dfs([&](const node& n) -> visit_result {
        if (n.depth() > max_d) max_d = n.depth();
        return visit_result::proceed;
    });
    return max_d;
}

path_tree::node::ptr path_tree::find(const path& p) const noexcept {
    if (!root_) return nullptr;

    const path target = p.absolute();

    node::ptr result;
    traverse_dfs([&](const node& n) -> visit_result {
        if (n.get_path() == target) {
            result = const_cast<node&>(n).shared_from_this();
            return visit_result::stop;
        }
        return visit_result::proceed;
    });
    return result;
}

vector<path_tree::node::ptr> path_tree::find_all(const string_view name) const {
    return find_if([&](const node& n) {
        return n.get_path().filename() == name;
    });
}

vector<path_tree::node::ptr> path_tree::find_if(const filter& f) const {
    vector<node::ptr> result;
    collect_impl(root_, f, result);
    return result;
}

vector<path_tree::node::ptr> path_tree::find_by_extension(const string_view ext) const {
    return find_if([&](const node& n) {
        return n.is_file() && n.get_path().extension() == ext;
    });
}

void path_tree::collect_impl(
    const node::ptr&    current,
    const filter&       f,
    vector<node::ptr>&  result
) {
    if (!current) return;
    if (f(*current)) result.push_back(current);
    for (const auto& child : current->children_) {
        collect_impl(child, f, result);
    }
}

void path_tree::traverse_dfs(const visitor& v) const {
    if (!root_) return;
    bool stopped = false;
    traverse_dfs_impl(root_, v, stopped);
}

void path_tree::traverse_dfs_impl(
    const node::ptr& current,
    const visitor&   v,
    bool&            stopped
) {
    if (!current || stopped) return;

    const visit_result res = v(*current);
    if (res == visit_result::stop)  { stopped = true; return; }
    if (res == visit_result::skip)  { return; }

    for (const auto& child : current->children_) {
        if (stopped) break;
        traverse_dfs_impl(child, v, stopped);
    }
}

void path_tree::traverse_bfs(const visitor& v) const {
    if (!root_) return;

    queue<node::ptr> q;
    q.push(root_);

    while (!q.empty()) {
        node::ptr current = q.front();
        q.pop();

        const visit_result res = v(*current);
        if (res == visit_result::stop) return;
        if (res == visit_result::skip) continue;

        for (const auto& child : current->children_) {
            q.push(child);
        }
    }
}

void path_tree::traverse_files(const visitor& v) const {
    traverse_dfs([&](const node& n) -> visit_result {
        if (n.is_file()) return v(n);
        return visit_result::proceed;
    });
}

void path_tree::traverse_dirs(const visitor& v) const {
    traverse_dfs([&](const node& n) -> visit_result {
        if (n.is_directory()) return v(n);
        return visit_result::proceed;
    });
}

path_tree::node::ptr path_tree::insert(const path& p, const node_type type) {
    if (!root_) return nullptr;

    const path abs = p.is_file() ? p.parent_path().absolute() / p.filename()
                                 : p.absolute();
    node::ptr current = root_;

    for (auto it = abs.begin(); it != abs.end(); ++it) {
        const string_view part = *it;
        if (part.empty()) continue;

        node::ptr child = current->find_child(part);
        if (!child) {
            auto next_it = it;
            ++next_it;
            const bool is_last = (next_it == abs.end());
            const node_type child_type = is_last ? type : node_type::directory;

            child = make_shared<node>(
                current->get_path() / part,
                child_type,
                current->depth_ + 1
            );
            child->parent_ = current;
            current->children_.push_back(child);
        }
        current = child;
    }

    return current;
}

bool path_tree::remove(const path& p) {
    if (!root_) return false;

    const node::ptr target = find(p);
    if (!target) return false;
    if (target->is_root()) { root_ = nullptr; return true; }

    const node::ptr parent = target->parent();
    if (!parent) return false;

    return parent->remove_child(target->get_path().filename());
}

void path_tree::merge(const path_tree& other) {
    if (!other.root_) return;

    other.traverse_dfs([&](const node& n) -> visit_result {
        if (!n.is_root()) {
            insert(n.get_path(), n.type());
        }
        return visit_result::proceed;
    });
}

path_tree path_tree::subtree(const path& p) const {
    const node::ptr target = find(p);
    if (!target) return {};

    path_tree result;
    result.root_ = clone_node(target, nullptr, 0);
    return result;
}

path_tree::node::ptr path_tree::clone_node(
    const node::ptr& src,
    const node::ptr& new_parent,
    const size_t     depth
) {
    if (!src) return nullptr;

    auto cloned = make_shared<node>(src->get_path(), src->type(), depth);
    if (new_parent) cloned->parent_ = new_parent;

    for (const auto& child : src->children_) {
        auto cloned_child = clone_node(child, cloned, depth + 1);
        cloned->children_.push_back(cloned_child);
    }
    return cloned;
}

path_tree path_tree::prune(const filter& f) const {
    if (!root_) return {};

    path_tree result;
    result.root_ = prune_impl(root_, f, nullptr, 0);
    return result;
}

path_tree::node::ptr path_tree::prune_impl(
    const node::ptr& src,
    const filter&    f,
    const node::ptr& new_parent,
    const size_t     depth
) {
    if (!src) return nullptr;

    auto cloned = make_shared<node>(src->get_path(), src->type(), depth);
    if (new_parent) cloned->parent_ = new_parent;

    for (const auto& child : src->children_) {
        auto pruned_child = prune_impl(child, f, cloned, depth + 1);
        if (pruned_child) {
            cloned->children_.push_back(pruned_child);
        }
    }

    if (src->is_directory()) {
        if (!cloned->children_.empty() || f(*src)) return cloned;
        return nullptr;
    }

    return f(*src) ? cloned : nullptr;
}

vector<path> path_tree::all_paths() const {
    vector<path> result;
    traverse_dfs([&](const node& n) -> visit_result {
        result.push_back(n.get_path());
        return visit_result::proceed;
    });
    return result;
}

vector<path> path_tree::all_file_paths() const {
    vector<path> result;
    traverse_files([&](const node& n) -> visit_result {
        result.push_back(n.get_path());
        return visit_result::proceed;
    });
    return result;
}

vector<path> path_tree::all_dir_paths() const {
    vector<path> result;
    traverse_dirs([&](const node& n) -> visit_result {
        result.push_back(n.get_path());
        return visit_result::proceed;
    });
    return result;
}

string path_tree::to_string() const {
    if (!root_) return {};
    return to_string_impl(root_, "  ", 0);
}

string path_tree::to_string(const string_view indent) const {
    if (!root_) return {};
    return to_string_impl(root_, indent, 0);
}

string path_tree::to_string_impl(
    const node::ptr& n,
    const string_view indent,
    const size_t depth
) const {
    if (!n) return {};

    string result;
    for (size_t i = 0; i < depth; ++i) result += indent;

    const string_view fname = n->is_root()
        ? n->get_path().view()
        : n->get_path().filename();

    result += fname;
    if (n->is_directory()) result += '/';
    result += '\n';

    for (const auto& child : n->children_) {
        result += to_string_impl(child, indent, depth + 1);
    }
    return result;
}

NEFORCE_END_NAMESPACE__
