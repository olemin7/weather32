/*
 * sensors.h
 *
 *  Created on: Jun 24, 2023
 *      Author: oleksandr
 */

#ifndef SRC_SENSORS_H_
#define SRC_SENSORS_H_

#include <functional>
#include <memory>
#include <stdint.h>
#include <optional>
#include <chrono>

namespace sensors {

typedef struct {
    float temperature;
    float humidity;
    float pressure;
} bme280_t;

enum class status_t {
    ok = 0,
    timeout,
    stoped,
};

typedef struct {
    std::optional<bme280_t> bme280;
    status_t                status;
} result_t;

class CManager {
 public:
    using cb_t = std::function<void(const result_t&)>;
    CManager();
    ~CManager();
    void init();
    void begin(cb_t&& cb, std::chrono::milliseconds timeout);
    void stop();

 private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sensors

#endif /* SRC_SENSORS_H_ */
