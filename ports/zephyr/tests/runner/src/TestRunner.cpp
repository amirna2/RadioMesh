/**
 * On-board Unity test runner for Zephyr.
 *
 * Runs the one test/ suite built into this image (CMake -DSUITE=<name>) and
 * reports the result on the console:
 *   RM_TEST_START          the suite is about to run
 *   RM_TEST_DONE rc=<n>    n failed tests, or -1 if no test ran
 * `make test` (ports/zephyr/tools/rmon.py) waits for the DONE line.
 *
 * Suites use either entry style of the PlatformIO test environments:
 *  - Arduino style: setup() runs the tests; loop() is empty and never called.
 *  - Native style: main(), renamed to rm_test_main for the suite's sources.
 */
#include <unity.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void setup() __attribute__((weak));
int rm_test_main(int argc, char** argv) __attribute__((weak));

// Empty default fixtures, as PlatformIO provides: most suites define neither.
extern "C"
{
__attribute__((weak)) void setUp(void)
{
}

__attribute__((weak)) void tearDown(void)
{
}
}

int main(void)
{
    // USB-Serial-JTAG discards console output until the host opens the port
    // (~1.5 s after reset); give the monitor time to attach.
    k_msleep(3000);
    printk("RM_TEST_START\n");

    if (rm_test_main != nullptr) {
        rm_test_main(0, nullptr);
    } else if (setup != nullptr) {
        setup();
    }

    // UNITY_END() leaves the run's totals in the Unity state. A suite that ran
    // no test must never read as a pass.
    int rc = (Unity.NumberOfTests == 0) ? -1 : static_cast<int>(Unity.TestFailures);
    printk("RM_TEST_DONE rc=%d\n", rc);
    return 0;
}
