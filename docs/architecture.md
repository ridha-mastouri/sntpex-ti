# Architecture Overview

**Author:** Ridha MASTOURI  
**Date:** Jan 6, 2026
**Version:** 1.0.0

This document describes the internal architecture and execution model of the
**Extended SNTP library for TI SimpleLink**.

The library implements a **client-side SNTP protocol** compliant with **RFC 4330**
and is designed specifically for **TI SimpleLink Wi-Fi devices** using the
SimpleLink socket and Spawn-task model.

---

## Design Goals

- Higher time accuracy than the default TI SNTP implementation
- Deterministic and predictable execution
- Fully event-driven (no blocking calls)
- Safe interaction with ISR and networking stack
- Low memory and CPU footprint
- Portability across SimpleLink Wi-Fi devices

---

## High-Level Architecture

The SNTP extended library is structured around four main components:

1. **Client State Machine**
2. **Network Interface Layer**
3. **Timestamp Processing Engine**
4. **Event & Execution Controller**

All SNTP protocol logic executes inside the **SimpleLink Spawn task context**.

---

## Client State Machine

The SNTP client operates as a state machine with the following logical states:

- **UNINITIALIZED**  
  Client is not configured or active.

- **INITIALIZED**  
  Client handle and platform abstraction are registered.

- **CONFIGURED**  
  Server address, timeout, and network interface are configured.

- **REQUEST_SENT**  
  SNTP request packet has been transmitted.

- **WAITING_RESPONSE**  
  Client is waiting for a server response.

- **RESPONSE_RECEIVED**  
  SNTP response packet received and validated.

- **TIME_UPDATED**  
  Local time is updated based on computed offset.

- **ERROR**  
  Error condition or Kiss-of-Death received.

State transitions are triggered exclusively by **events**, never by blocking waits.

---

## Execution Model

### Event-Driven Processing

- No busy-wait loops
- No blocking socket calls
- All processing is triggered by events

The application must periodically call:

```c
sntpex_eventTriggingFromISR();
````

from the **SimpleLink internal Spawn task** (e.g. `_SlInternalSpawn()`).

This function advances the SNTP state machine and performs:

* Packet transmission
* Packet reception
* Timeout handling
* Timestamp extraction
* Time correction

---

## Network Communication

* Uses **UDP sockets**
* Supports IPv4 and IPv6
* Server address can be configured directly or resolved via DNS
* Socket is optionally bound to a specific network interface

The library does not manage Wi-Fi connection or IP acquisition; these must be
handled by the application.

---

## Timestamp Processing Engine

The library implements full SNTP timestamp handling using four timestamps:

* **T1** – Client transmit time
* **T2** – Server receive time
* **T3** – Server transmit time
* **T4** – Client receive time

From these timestamps, the library computes:

* **Round-trip delay**
* **Clock offset**

These calculations reduce network-induced jitter and improve synchronization accuracy.

---

## Time Representation

* SNTP timestamps are handled in **64-bit format**
* Fractional seconds are preserved for higher precision
* Unix timestamp conversion is supported

The platform-specific time source is provided via the **ud_op_vtable** abstraction.

---

## Platform Abstraction Layer

To remain OS- and hardware-agnostic, the library relies on a user-provided
`ud_op_vtable` structure that supplies:

* Current SNTP time
* Current Unix timestamp
* OS tick count
* Deferred execution callback

This design allows integration with:

* Bare-metal systems
* TI-RTOS
* FreeRTOS

---

## Kiss-of-Death (KoD) Handling

* SNTP Kiss-of-Death packets are detected and parsed
* The KoD code is stored internally
* The application can retrieve the code using:

  ```c
  sntpex_client_Kiss_code_get()
  ```

Upon receiving a KoD, the client enters an error state and stops synchronization.

---

## Error Handling Strategy

* All public APIs return `sntp_ud_t` error codes
* Network, protocol, and timeout errors are explicitly reported
* Invalid packets are discarded safely
* No undefined behavior on malformed input

---

## Thread Safety & ISR Safety

* No dynamic memory allocation in ISR context
* No blocking calls in ISR or Spawn task
* All shared state is accessed deterministically

---

## Why This Design

Compared to the default TI SNTP implementation, this architecture provides:

* Better control over execution timing
* Improved timestamp accuracy
* Clear separation of concerns
* Easier debugging and maintenance
* Safer integration with real-time systems

---

## Notes

* This library is **not an official Texas Instruments product**
* Designed for **TI SimpleLink Wi-Fi devices**
* Intended for developers needing **precise time synchronization**

