/**
 * @file main_exemple.c
 * @brief Example usage of the Extended SNTP library for TI SimpleLink
 *
 * This example demonstrates the **correct, real integration flow**
 * of the library, including the **main API used by applications**:
 *
 *   👉 sntpex_client_timestamp_get()
 *
 * which is the primary function used to retrieve the synchronized
 * SNTP / Unix timestamp.
 *
 * IMPORTANT ASSUMPTIONS:
 *  - Wi-Fi connection and IP acquisition are already completed
 *  - SimpleLink Spawn task is running
 *  - This example focuses only on SNTP integration
 */

#include <stdint.h>
#include <string.h>

#include "sntp_ex_lib_ti.h"

/* =========================================================================
 * Platform abstraction (ud_op_vtable implementation)
 * ========================================================================= */

/* Get current SNTP time (seconds + fractional part) */
static void app_get_sntp_time(uint32_t *pulseconds, uint32_t *pulfraction)
{
    /* Provide current local time if available (optional) */
    *pulseconds  = 0;
    *pulfraction = 0;
}

/* Get current Unix timestamp (seconds) */
static uint32_t app_get_unix_timestamp(void)
{
    /* Return current system Unix time if RTC is already set */
    return 0;
}

/* Get OS tick count */
static uint32_t app_get_os_tick(void)
{
    /* Return OS tick or system uptime counter */
    return 0;
}


/* =========================================================================
 * SNTP client objects
 * ========================================================================= */

static sntpex_client_handle_t g_sntp_client;

/* Platform abstraction vtable */
static struct ud_op_vtable g_ud_vtable =
{
    .get_sntp_time      = app_get_sntp_time,
    .get_unix_timestamp = app_get_unix_timestamp,
    .get_os_tick        = app_get_os_tick
};

/* =========================================================================
 * SNTP initialization
 * ========================================================================= */

void app_sntp_init(void)
{
    sntp_ud_t ret;
    SlNetSock_Addr_t serverAddr;

    /* Initialize SNTP client */
    ret = sntpex_clientInitialization(&g_sntp_client, &g_ud_vtable);
    if (ret != SNTPEX_SUCCESS)
    {
        /* Handle initialization error */
        return;
    }

    /* Resolve NTP server hostname */
    ret = sntpex_resolve_server(
            "pool.ntp.org",
            &serverAddr,
            SLNETSOCK_AF_INET
          );
    if (ret != SNTPEX_SUCCESS)
    {
        /* Handle DNS resolution error */
        return;
    }

    /* Configure SNTP server address */
    ret = sntpex_client_set_server_address(&g_sntp_client, &serverAddr);
    if (ret != SNTPEX_SUCCESS)
    {
        /* Handle configuration error */
        return;
    }

    /* Optional: configure client timeout (milliseconds) */
    sntpex_SetClientTimeout(&g_sntp_client, 3000);
}

/* =========================================================================
 * SimpleLink Spawn task hook (MANDATORY)
 * ========================================================================= */

/*
 * This function MUST be called from the SimpleLink internal
 * Spawn task context (for example inside _SlInternalSpawn()).
 */
void SimpleLinkSpawnHook(void)
{
    sntpex_eventTriggingFromISR();
}

/* =========================================================================
 * Application main loop
 * ========================================================================= */

void app_main_loop(void)
{
    sntp_ud_t ret;
    struct xTimestampCtx_t timestampCtx;

    /*
     * Retrieve synchronized timestamp
     * This is the PRIMARY API used by applications
     */
    ret = sntpex_client_timestamp_get(&g_sntp_client, &timestampCtx);
    if (ret == SNTPEX_SUCCESS)
    {
        /*
         * timestampCtx typically contains:
         *  - Unix seconds
         *  - Fractional seconds
         *  - SNTP timestamps (T1–T4)
         *
         * TODO:
         *  - Update RTC
         *  - Update system clock
         *  - Use timestamp for logging / TLS / scheduling
         */
    }

    /*
     * Optional: check Kiss-of-Death status
     */
    if (sntpex_client_Kiss_code_get(&g_sntp_client) != 0)
    {
        /* Handle KoD condition (server rate limiting, etc.) */
    }
}

/************************ (C) COPYRIGHT RIDHA MASTOURI *****END OF FILE*****************/
