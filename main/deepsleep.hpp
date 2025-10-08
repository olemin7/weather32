/*
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#include <chrono>

namespace deepsleep {
int get_boot_count();
void sleep(const std::chrono::milliseconds duration);

} // namespace deepsleep
