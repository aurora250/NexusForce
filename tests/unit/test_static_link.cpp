#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/file/path.hpp>
#include <NeForce/core/string/basic_string.hpp>
#include <gtest/gtest.h>

TEST(StaticLinkTest, PathReturnedFromLibrarySurvives) {
    using namespace neforce;

    const path directory = filesystem::temp_directory();
    ASSERT_FALSE(directory.str().empty());

    const string normalized = directory.to_string();
    EXPECT_FALSE(normalized.empty());

    string copy = directory.str();
    copy += "/static-link";
    EXPECT_GT(copy.size(), directory.str().size());
    EXPECT_EQ(copy.substr(0, directory.str().size()), directory.str());
}
