/*
 * provision.h
 *
 *  Created on: Jun 14, 2024
 *      Author: oleksandr
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void provision_main(void);
esp_err_t provision_reset(void);
#ifdef __cplusplus
}
#endif

