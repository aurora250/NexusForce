#include <NeForce/core/system/pipe.hpp>
#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
    neforce::pipe::ignore_sigpipe();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
