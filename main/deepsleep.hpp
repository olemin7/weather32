/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#include <chrono>
namespace deepsleep {
int get_boot_count();

void deep_sleep(const std::chrono::milliseconds duration);

void set_timeout(const std::chrono::milliseconds duration, const std::chrono::milliseconds sleep_duration);
void extend_timeout(const std::chrono::milliseconds duration);
void sleep();
} // namespace deepsleep
