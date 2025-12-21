#include <windows.h>
#include "tests/test_ls.hpp"
#include "tests/test_rws.hpp"
#include "tests/test_ses.hpp"


int main() {
    SetConsoleOutputCP(65001);
    run_test_ls();
    run_test_rws();
    run_test_se();
    run_test_ss();
    run_test_final();
    return 0;
}
