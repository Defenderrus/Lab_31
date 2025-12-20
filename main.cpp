#include <windows.h>
#include "tests/test_ls.hpp"
#include "tests/test_rws.hpp"


int main() {
    SetConsoleOutputCP(65001);
    run_test_ls();
    run_test_rws();
    return 0;
}
