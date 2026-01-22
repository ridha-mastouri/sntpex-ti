# SNTP Extended Library – API Reference

**Author:** Ridha MASTOURI  
**Date:** Jan 6, 2026
**Version:** 1.0.0

This document describes the **public API** of the **Extended SNTP library for TI SimpleLink**.

The library implements a **client-side SNTP protocol** aligned with **RFC 4330**, using
TI SimpleLink socket APIs, and is designed to be executed from the **SimpleLink Spawn task context**.

---

## Data Types & Callback Interfaces

### pf_eventCallback

```c
typedef void (*pf_eventCallback)(uint8_t eventFiled);
````

User callback invoked by the SNTP client to notify application-level events.

---

### struct ud_op_vtable

This structure provides **platform abstraction hooks** that must be implemented
by the user application.

#### Members

* `get_sntp_time(uint32_t *pulseconds, uint32_t *pulfraction)`
* `get_unix_timestamp(void)`
* `get_os_tick(void)`

---

## Client Initialization & Lifecycle

### sntpex_clientInitialization

```c
sntp_ud_t sntpex_clientInitialization(
    sntpex_client_handle_t *p_client,
    struct ud_op_vtable *p_vtable_api
);
```

Initializes an SNTP client instance.

**Parameters**

* `p_client` : Pointer to SNTP client handle
* `p_vtable_api` : Platform abstraction vtable

**Returns**

* `SNTPEX_SUCCESS` on success
* Error code of type `sntp_ud_t` otherwise

---

### sntpex_client_deinitialization

```c
sntp_ud_t sntpex_client_deinitialization(
    sntpex_client_handle_t *p_client
);
```

Deinitializes the SNTP client and releases internal resources.

---

## Network Configuration

### sntpex_client_bind_to_interface

```c
sntp_ud_t sntpex_client_bind_to_interface(
    sntpex_client_handle_t *p_client,
    InterfaceIndex_t *interface
);
```

Binds the SNTP client socket to a specific network interface.

---

### sntpex_client_set_server_address

```c
sntp_ud_t sntpex_client_set_server_address(
    sntpex_client_handle_t *p_client,
    const SlNetSock_Addr_t *serverIpAddr
);
```

Configures the SNTP server IP address (IPv4 or IPv6).

---

### sntpex_resolve_server

```c
sntp_ud_t sntpex_resolve_server(
    const char *pcHostname,
    SlNetSock_Addr_t *pxhost_address,
    uint8_t Family
);
```

Resolves an SNTP server hostname to an IP address.

---

### sntpex_SetClientTimeout

```c
sntp_ud_t sntpex_SetClientTimeout(
    sntpex_client_handle_t *p_client,
    uint32_t timeout
);
```

Sets the SNTP client timeout in milliseconds.

---

## Timestamp Handling

### sntpex_extract_timestamp

```c
sntp_ud_t sntpex_extract_timestamp(
    sntpex_client_handle_t *p_client,
    struct xTimestampCtx_t *xTimestampCtx
);
```

Extracts SNTP timestamps (T1, T2, T3, T4) from received packets.

Supports:

* Unix 64-bit timestamp format
* SNTP fractional timestamp format

---

## Kiss-of-Death (KoD)

### sntpex_client_Kiss_code_get

```c
uint8_t sntpex_client_Kiss_code_get(
    sntpex_client_handle_t *p_client
);
```

Retrieves the **Kiss-of-Death (KoD)** code received from the SNTP server.

---

## Event Handling (CRITICAL)

### sntpex_eventTriggingFromISR

```c
sntp_ud_t sntpex_eventTriggingFromISR(void);
```

Handles SNTP internal events.

⚠ **MANDATORY REQUIREMENT**

This function **must be called from the SimpleLink internal Spawn task context**
(e.g. `_SlInternalSpawn()`), otherwise timing accuracy and protocol behavior
are not guaranteed.

**Returns**

* `SNTPEX_SUCCESS` if successful
* Error code of type `sntp_ud_t` otherwise

---

## Execution Model Summary

* Event-driven SNTP client
* Non-blocking socket operations
* All protocol logic runs inside the **SimpleLink Spawn task**
* ISR-safe event triggering

---

## Error Handling

All APIs return a value of type `sntp_ud_t`, including:

* `SNTPEX_SUCCESS`
* Protocol-related errors
* Network and timeout errors

Refer to the header file for the complete enumeration.

---

## Notes

* This is **not an official Texas Instruments library**
* Designed for **TI SimpleLink Wi-Fi devices**
* RFC 4330 compliant SNTP client


