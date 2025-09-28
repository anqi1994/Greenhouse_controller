//
// Created by lily on 9/19/2025.
//

#ifndef FREERTOS_VENTILATION_PROJECT_TLS_COMMON_H
#define FREERTOS_VENTILATION_PROJECT_TLS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "queue.h"

#include "global_definition.h"

    bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);

#ifdef __cplusplus
}
#endif

#endif // FREERTOS_VENTILATION_PROJECT_TLS_COMMON_H
