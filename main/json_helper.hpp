/*
 * json_helper.hpp
 *
 *  Created on: Aug 27, 2024
 *      Author: oleksandr
 */

#ifndef MAIN_JSON_HELPER_HPP_
#define MAIN_JSON_HELPER_HPP_
#include <cstdio>
#include <string>
#include "cJSON.h"

namespace json {
class CreateObject {
 private:
    cJSON* item;

 public:
    CreateObject()
        : item(cJSON_CreateObject()) {}
    auto get() const {
        return item;
    }
    ~CreateObject() {
        cJSON_Delete(item);
    }
};

inline std::string PrintUnformatted(const CreateObject& obj) {
    const auto  my_json = cJSON_PrintUnformatted(obj.get());
    std::string res(my_json);
    cJSON_free(my_json);
    return res;
}

template<typename T>
void AddFormatedToObject(const CreateObject& obj, const char* const name, const char* format, T var) {
    char       tt[255];
    const auto cnt = snprintf(tt, sizeof(tt) - 1, format, var);
    tt[cnt]        = 0;
    cJSON_AddRawToObject(obj.get(), name, tt);
}

} // namespace json

#endif /* MAIN_JSON_HELPER_HPP_ */
