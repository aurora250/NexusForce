#include "try.h"

int main(int argc, char* argv[]) {
    uint64_t sum = 0;
    constexpr int count = 1;
    for (int i = 0; i < count; i++) {
        click clk;
        {
            scoped_click grd(clk);
            test_tpool();
        }
        sum += clk.during().count();
        clk.reset();
    }
    println(sum / count);
    console.pause();
}
