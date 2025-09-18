#pragma once
#include <functional>
namespace sntp
{
    void init(std::function<void()> &&onsync_cb);
    void start();
}
