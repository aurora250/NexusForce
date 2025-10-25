#include <MSTL/core/device.hpp>
#include <MSTL/core/console.hpp>
#include <MSTL/core/file.hpp>
using namespace MSTL;

int main() {
    auto devs = diskdrive::enumerate_all();
    for (const auto& dev : devs) {
        println(dev);
    }
    console.pause();
}
