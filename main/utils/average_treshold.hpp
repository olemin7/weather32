/*
 *
 *  Created on: Jun 14, 2025
 *      Author: oleksandr
 */

#pragma once
#include <inttypes.h>
#include <queue>
#include <numeric>
#include <chrono>

namespace utils
{

    template <typename T, typename S = T>
    class average
    {
    protected:
        const uint8_t deep_;
        std::queue<T> queue_;
        S summ_ = 0;

    public:
        average(uint8_t deep = 3) : deep_(deep) {}
        void push(T value)
        {
            while (deep_ <= queue_.size())
            {
                summ_ -= queue_.front();
                queue_.pop();
            }
            queue_.push(value);
            summ_ += value;
        }
        T get_size() const
        {
            return queue_.size();
        }
        T get_average() const
        {
            return summ_ / get_size();
        }
    };

    template <typename T, typename S = T>
    class average_treshold : public average<T, S>
    {
    private:
        const T treshold_;
        T prev_;

    protected:
        void re_arm()
        {
            prev_ = this->get_average();
        }

    public:
        average_treshold(T treshold, uint8_t deep = 3) : average<T, S>(deep), treshold_(treshold) {}
        virtual bool push(T value)
        {
            average<T, S>::push(value);
            const auto avg = this->get_average();
            if ((this->queue_.size() == 1) || (std::abs(prev_ - avg) > treshold_))
            {
                prev_ = avg;
                return true;
            }
            return false;
        }
    };

    template <typename T, typename S = T>
    class average_treshold_timeout : public average_treshold<T, S>
    {
    private:
        const std::chrono::milliseconds keep_alive_;
        std::chrono::steady_clock::time_point next_;

    public:
        average_treshold_timeout(T treshold, uint8_t deep, std::chrono::milliseconds keep_alive) : average_treshold<T, S>(treshold, deep), keep_alive_(keep_alive) {}
        bool push(T value) override
        {
            const auto now = std::chrono::steady_clock::now();
            if (average_treshold<T, S>::push(value) || now >= next_)
            {
                this->re_arm();
                next_ = now + keep_alive_;
                return true;
            }
            return false;
        }
    };
} // namespace utils
