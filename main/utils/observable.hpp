
#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>

namespace utils
{

    /**
     * @brief observable implement observer pattern, any object derived from this class will be observable
     * @tparam T A subject type
     */
    template <typename T>
    class observable
    {
    public:
        using observable_t = std::function<void(const T &subject)>;

    private:
        std::vector<observable_t> observers_;

    public:
        observable() = default;
        /**
         * @brief subscribe observer to subject, once subject is changed, update method of observer will be called
         * @param observer[in] A shared pointer to observer object for tracking
         */
        void subscribe(observable_data_t observer)
        {
            observers_.push_back(observer);
        }

              /**
         * @brief For updating subject
         * @param subject[in]
         */
        virtual void notify(const T &subject)
        {
            for (auto o : observers_)
            {
                o(subject);
            }
        }
        virtual ~observable() = default;
    };

}
