#ifndef NEFORCE_TEST_TUI_SNAPSHOT_HPP__
#define NEFORCE_TEST_TUI_SNAPSHOT_HPP__
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/file/file.hpp"
#include "NeForce/core/system/environment.hpp"
#include <gtest/gtest.h>

#ifndef NEFORCE_SNAPSHOT_DIR
#    define NEFORCE_SNAPSHOT_DIR "snapshots"
#endif

namespace tui_snapshot {
    inline neforce::string directory() { return NEFORCE_SNAPSHOT_DIR; }

    inline neforce::string path_for(const neforce::string& name) { return directory() + "/" + name + ".snap"; }

    inline bool update_mode() {
        const neforce::string env = neforce::environment::get("NEFORCE_UPDATE_SNAPSHOTS");
        return !env.empty() && env[0] != '0';
    }

    inline neforce::string read_file(const neforce::string& path) {
        neforce::file in(neforce::path{path});
        if (!in.is_opened()) {
            return {};
        }
        return in.read_binary();
    }

    inline void write_file(const neforce::string& path, const neforce::string& content) {
        neforce::file out(neforce::path{path}, true);
        out.write(content);
    }

    inline neforce::string normalize(neforce::string text) {
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
            text.pop_back();
        }
        return text;
    }

    inline ::testing::AssertionResult check(const neforce::string& name, const neforce::string& actual_raw) {
        const neforce::string actual = normalize(actual_raw);
        const neforce::string path = path_for(name);

        if (update_mode()) {
            write_file(path, actual + "\n");
            return ::testing::AssertionSuccess();
        }

        const neforce::string expected = normalize(read_file(path));
        if (expected.empty()) {
            return ::testing::AssertionFailure()
                   << "missing snapshot: " << path.data() << "\n(regenerate with NEFORCE_UPDATE_SNAPSHOTS=1)\n"
                   << "actual:\n"
                   << actual.data();
        }
        if (expected == actual) {
            return ::testing::AssertionSuccess();
        }

        return ::testing::AssertionFailure() << "snapshot mismatch: " << path.data() << "\n--- expected ---\n"
                                             << expected.data() << "\n--- actual ---\n"
                                             << actual.data();
    }

    inline ::testing::AssertionResult check(const neforce::string& name, const char* actual_raw) {
        return check(name, neforce::string(actual_raw));
    }
} // namespace tui_snapshot

#define NEFORCE_EXPECT_SNAPSHOT(name, actual) EXPECT_TRUE(::tui_snapshot::check(name, actual)) << "snapshot: " #name

#endif // NEFORCE_TEST_TUI_SNAPSHOT_HPP__
