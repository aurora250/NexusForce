#include "try.h"

int main(int argc, char* argv[]) {
    uint64_t sum = 0;
    for (int i = 1; i < 10; i++) {
        click clk;
        {
            scoped_click grd(clk);
            test_tpool();
        }
        sum += clk.during().count();
        clk.reset();
    }
    println(sum / 10);
    console.pause();
}
