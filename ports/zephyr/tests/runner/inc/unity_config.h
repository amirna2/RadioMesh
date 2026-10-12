#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Unity configuration for the Zephyr on-board runner (Unity requires this file
// name). Unity writes through printk, the same console sink as the RadioMesh
// Logger, so test results and library logs come out in order.
#include <zephyr/sys/printk.h>

#define UNITY_OUTPUT_CHAR(c) printk("%c", (c))

// No setjmp/longjmp: on the ESP32-S3 they link to ROM, whose register-window
// spill syscall Zephyr does not service (it prints a register dump instead).
// A failed assertion is still recorded and reported; the test body then runs
// on instead of jumping out.
#define UNITY_EXCLUDE_SETJMP_H

#endif // UNITY_CONFIG_H
