# sntpex-ti

**Author:** Ridha MASTOURI  
**Date:** Jan 6, 2026
**Version:** 1.0.0

Extended SNTP (Simple Network Time Protocol) library for **TI SimpleLink™** Wi-Fi devices.

This project provides a more accurate and configurable SNTP client implementation
than the default TI SNTP library. It is designed to work with **TI SimpleLink Socket APIs**
(e.g., CC31xx / CC32xx families) and follows **RFC 4330** behavior where applicable.

---

## Features

- ✅ Higher time accuracy compared to TI default SNTP
- ✅ RFC-aligned SNTP client behavior
- ✅ Designed for TI SimpleLink Socket API
- ✅ ISR-safe event triggering (Spawn task integration)
- ✅ Lightweight and portable C implementation
- ✅ Suitable for bare-metal or RTOS environments

---

## Supported Platforms

- TI SimpleLink™ Wi-Fi devices (e.g. CC3135, CC3220, CC3235)
- Tested with CC3135 BSP (3.0.1.60-rc1.0.0)

> **Note**: This library assumes that SimpleLink socket headers and BSP are already
> available in your project.

---

## Repository Structure

```text
sntpex-ti/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── include/
│   └── sntp_ex_lib_ti.h
├── src/
│   └── sntp_ex_lib_ti.c
└── docs/
    ├── architecture.md
    └── api.md
````

---

## Integration Overview

1. Add the library source to your project
2. Ensure SimpleLink socket APIs are included
3. Initialize the SNTP client
4. Trigger SNTP events from the SimpleLink Spawn task

### Important

You **must** call:

```c
sntpex_eventTriggingFromISR();
```

from the SimpleLink internal Spawn context (e.g. `_SlInternalSpawn()`)
to allow proper event handling.

---

## Basic Usage Example

The SNTP extended library is **event-driven** and must be integrated with the
SimpleLink Spawn task. The example below shows the **minimal correct flow**
to initialize the client.

```c
#include "sntp_ex_lib_ti.h"

/* SNTP client handle */
static sntpex_client_handle_t sntpClient;

void app_init(void)
{
    SlNetSock_Addr_t serverAddr;

    /* Initialize SNTP client (vtable must be provided by application) */
    sntpex_clientInitialization(&sntpClient, &sntpVtable);

    /* Resolve SNTP server hostname */
    sntpex_resolve_server(
        "pool.ntp.org",
        &serverAddr,
        SLNETSOCK_AF_INET
    );

    /* Configure SNTP server address */
    sntpex_client_set_server_address(&sntpClient, &serverAddr);

    /* Optional: set client timeout (milliseconds) */
    sntpex_SetClientTimeout(&sntpClient, 3000);
}

/*
 * This function MUST be called from the SimpleLink internal Spawn task
 */
void SimpleLinkSpawnHook(void)
{
    sntpex_eventTriggingFromISR();
}
```

After synchronization completes, the current timestamp can be obtained using:

```c
struct xTimestampCtx_t timestampCtx;

if (sntpex_client_timestamp_get(&sntpClient, &timestampCtx) == SNTPEX_SUCCESS)
{
    /* Use synchronized timestamp */
}
```

---

## Build with CMake

This library can be built as a static library using CMake.

```bash
mkdir build
cd build
cmake ..
make
```

You can also include it as a subdirectory in a larger firmware project.

---

## Accuracy Improvements

Compared to the default TI SNTP implementation, this library:

* Uses improved timestamp calculation
* Reduces round-trip error impact
* Allows better control of polling and timeout behavior

---

## Documentation

* 📘 **API Reference**: `docs/api.md`
* 🧱 **Architecture Overview**: `docs/architecture.md`

---

## License

This project is released as **open source**.

Suggested license:

* **MIT License** (simple and permissive)
* or **BSD-3 Clause** (TI-friendly)

You can change this depending on your preference.

---

## Author

**Ridha Mastouri**

---

## Contributing

Contributions are welcome:

* Bug fixes
* Accuracy improvements
* Support for additional SimpleLink devices

Please open an issue or submit a pull request.

---

## Disclaimer

This is **not an official TI library** and is provided *“as-is”* without warranty.


