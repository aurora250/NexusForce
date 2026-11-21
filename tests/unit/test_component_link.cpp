#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/network/http/http_constants.hpp>
#include <gtest/gtest.h>

TEST(ComponentLinkTest, CoreAndNetworkComponentsLink) {
    using namespace neforce;
    using namespace neforce::http;

    const path directory = filesystem::temp_directory();
    EXPECT_FALSE(directory.str().empty());
    EXPECT_FALSE(http_status_message(http_status::ok).empty());
}
