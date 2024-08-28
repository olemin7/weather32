/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#include <chrono>
namespace deepsleep {
int get_boot_count();

void deep_sleep(const std::chrono::microseconds duration);
} // namespace deepsleep
