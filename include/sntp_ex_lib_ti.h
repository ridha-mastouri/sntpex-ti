/**
 * @file    sntpex_ti/sntp_ex_lib_ti.h
 *          This file is part of sntpex_ti.
 *
 * @section License
 * @par     COPYRIGHT NOTICE: (c) 2026 Ridha MASTOURI
 *
 * @brief   User Extended library for Simple-Network-Time-Protocol (SNTP) based-on TI SIMPLELINK SOCKET APIs.
 *
 * @note    This file is the SNTP interface implementation for CC3135 wifi adapter, aligned with RFC 4330.
 *
 * @details Definition of SNTP-protocol INTERFACE functions, Client component, including all data types and external references.
 *          It is assumed that simplelink socket file, have already been included.
 *
 * @remark  User must call @ref sntpex_eventTriggingFromISR() APIs on the CC3135 internal Spawn Task to handle library events 
 *          @ref _SlInternalSpawn().
 *          <B>[PATCH] Internal Patch version for CC3135-BSP : 3.0.1.60-rc1.0.0 </B>
 * 
 * @see     Public NTP server is available as gist :
 *          <a href="https://github.com/MSTR-LNX/Public-NTP-Servers.git">GITHUB-GIST</a>
 *          <a href="https://gist.github.com/MSTR-LNX/e40fe73f5a77e268652815a28e0cfc4d">GITHUB-GIST</a>
 *
 * @version V1.0.0
 *
 * @date    Created on :Jan 6, 2026
 * @author  Ridha MASTOURI
 */

#ifndef SNTPEX_LIBRARY_EXTENDED_TI_SIMPLELINK_SNTP_LIB_H_
#define SNTPEX_LIBRARY_EXTENDED_TI_SIMPLELINK_SNTP_LIB_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ----------------------------------------------------------------------*/
#include <ti/net/slnetsock.h>   /* socket APIs file */
#include <ti/net/slnetutils.h>  /* net utility APIs file */
#include <ti/net/slneterr.h>    /* specific error type and status file */

/* Lib configurations ------------------------------------------------------------*/

/**
 * @brief  Non blocking Timeout option, so timeout mecanism is handled on the @ref sntp_ex_ti_lib file
 *  Keep pooling until the return status is different from @ref SLNETERR_BSD_EAGAIN and timeout is not occured.
 * @remark Comment this line, to use the internal socket timeout which is configured on @ref p_client->sock->descriptor.timeout */
#define exlibSNTP_CLIENT_USE_NONBLOCKING_TIMEOUT_OPTION
/**
 * @brief  Default client timeout.
 * @remark This timeout will be used when application ignore the call of @ref sntpex_SetClientTimeout APIs. */
#define exlibSNTP_CLIENT_DEFAULT_TIMEOUT 3000 
/**
 * @brief  Define the maximum size of the packet NTP/SNTP time message
 * @remark includes 20 bytes for optional authentication data. */
#define exlibSNTP_TIME_MESSAGE_MAX_SIZE  68

/* Private macros ----------------------------------------------------------------*/

/* socket poll-in event option  */
#define exlibPOLLIN_EVENT               (0u)

/* socket poll-out event option */
#define exlibPOLLOUT_EVENT              (1u << 0)

/* number of seconds between 1900 and 1970 (MSB=1)*/
#define exlibDIFF_SEC_1900_1970         (2208988800) 

/**
 * @note Reorder the bytes of a 32-bit unsigned value from network
 * order(Big endian) to host order */
#define exlibSLNETUTIL_NTOHL( ulvalue ) SlNetUtil_ntohl( ulvalue )

/**
 * @note Reorder the bytes of a 32-bit unsigned value from host
 * order to network order(Big endian) */
#define exlibSLNETUTIL_HTONL( ulvalue ) SlNetUtil_htonl( ulvalue )

/**
 * @note Reorder the bytes of a 16-bit unsigned value from host
 * order to network order(Big endian) */
#define exlibSLNETUTIL_HTONS( ulvalue ) SlNetUtil_htons( ulvalue )

/**
 * @brief UD config protection */
#ifndef exlibSNTP_CLIENT_DEFAULT_TIMEOUT
#define exlibSNTP_CLIENT_DEFAULT_TIMEOUT  3000
#endif

#ifndef exlibSNTP_TIME_MESSAGE_MAX_SIZE
#define exlibSNTP_TIME_MESSAGE_MAX_SIZE   68
#endif

/* Event bit mask definition */
#define exlibSNTP_SOFTSR_RECV_BIT                  ( 1u << 0 )
#define exlibSNTP_SOFTSR_SEND_BIT                  ( 1u << 1 )

/* event function pointer type definition */
typedef void( * pf_eventCallback )( uint8_t eventFiled );

/* sntpex event structure */
struct ux_sntpAsynchEvent
{
  uint64_t  timestamp;  /* 64-UNIX Time used for T1 and T4 timestamp */

  pf_eventCallback event_cb;
  /**
   * @brief  SNTPEX event flags */
  union uxsntpexEvent
  {
    uint8_t SR; /* SOFT Status register */

    struct x_sntpbitFields
    {
      uint8_t recv_ev       : 1; /* Bit for registering T4 timestamp */
      uint8_t send_ev       : 1; /* Bit for registering T1 timestamp */  
    } bits;

  }event; 
};

/**
 * @brief SNTP Library error code */
typedef enum
{
  SNTPEX_SUCCESS =0,           /* Success code.                                */
  SNTPEX_ERROR,                /* Generic error code.                          */
  SNTPEX_ERR_SOCKET_CREATE,    /* Fail UDP socket creation.                    */
  SNTPEX_ERR_SOCKET_SET_OPT,   /* Fail setting UDP socket options.             */
  SNTPEX_ERR_FAULT_INIT,       /* Memory allocation error.                     */
  SNTPEX_ERR_DNS_RESOLVE,      /* Fail resolve DNS.                            */
  SNTPEX_ERR_NULL_PTR,         /* Ptr arg(s) passed NULL ptr.                  */
  SNTPEX_ERR_RX,               /* Error occured during packet reception.       */
  SNTPEX_ERR_TX,               /* Error occurred during request transmission.  */
  SNTPEX_ERR_REQUEST_REJECTED, /* NTP request rejected.                        */
  SNTPEX_ERR_INVALID_MESSAGE,  /* Invalid NTP message received.                */
  SNTPEX_ERR_TIMEOUT,          /* Timeout occured in RX/TX.                    */
} sntp_ud_t;

/**
 * @brief SNTP Version */
enum
{
  specNTP_VERSION_V1  = 1,
  specNTP_VERSION_V2  = 2,
  specNTP_VERSION_V3  = 3,
  specNTP_VERSION_V4  = 4,
};

/**
 * @brief SNTP Mode enumeration */
enum
{
  specNTP_MODE_RESERVED	   = 0,
  specNTP_MODE_SYM_ACTV	   = 1,
  specNTP_MODE_SYM_PASV	   = 2,
  specNTP_MODE_CLIENT      = 3,
  specNTP_MODE_SERVER      = 4,
  specNTP_MODE_BROADCAST   = 5,
  specNTP_MODE_NTP_CONTROL = 6,
  specNTP_MODE_PRIVATE	   = 7,
};

/**
 * @brief SNTP Stratum enumeration */
enum
{
  specNTP_STRATUM_KISS_O_DEATH	= 0,
  specNTP_STRATUM_PRI           = 1,
  specNTP_STRATUM_SEC_LO        = 2,
  specNTP_STRATUM_SEC_HI        = 15,
  specNTP_STRATUM_UNSYNC        = 16,
  specNTP_STRATUM_RSVD_LO       = 17,
  specNTP_STRATUM_RESERVED_HI   = 255,
} ;

/**
 * @brief state machine enumeration */
typedef enum
{
  UD_SNTP_CLIENT_STATE_OPEN         = 0,
  UD_SNTP_CLIENT_STATE_SENDING      = 1,
  UD_SNTP_CLIENT_STATE_RECEIVING    = 2,
  UD_SNTP_CLIENT_STATE_HANDLING_RSP = 3,
  UD_SNTP_CLIENT_STATE_CLOSE        = 4,
  UD_SNTP_CLIENT_STATE_COMPLETE     = 5
} udSntpClientState;

/**
 * @brief Return current stratum index */
#define	exlibSTRATUM_IDX(x)     ((x >= specNTP_STRATUM_RSVD_LO) ? 4 : (x == specNTP_STRATUM_UNSYNC)	? 3 : (x >= specNTP_STRATUM_SEC_LO)	? 2 : x )

/**
 * @brief SNTP Time structure */
typedef union
{
  /* 64 value format */
  uint64_t    val;
  struct
  {
    /* Seconds, in the 32 bit field of an NTP time data.  */
    uint32_t    seconds;
    /* Fraction of a second, in the 32 bit fraction field of an NTP time data.  */
    uint32_t    fraction;
  };
}NtpTimestamp ;

/**
 * @brief NTP timestamp list */
struct xTimestampCtx_t
{
  NtpTimestamp referenceTimestamp;   /* Time at which the server clock was last set or corrected in a server time message.   */
  NtpTimestamp originateTimestamp;   /* Time at which the Client update request left the Client in a server time message.    */
  NtpTimestamp receiveTimestamp;     /* Time at which the server received the Client request in a server time message.       */
  NtpTimestamp transmitTimestamp;    /* Time at which the server transmitted its reply to the Client in a server time message
                                        (or the time client request was sent in the client request message).  */
  uint64_t     originate64_ts;       /* 64-UNIX Time at which the server clock was last set or corrected in a server time message. */
  uint64_t     reference64_ts;       /* 64-UNIX Time at which the Client update request left the Client in a server time message   */
  uint64_t     receive64_ts;         /* 64-UNIX Time at which the server received the Client request in a server time message.     */
  uint64_t     transmit64_ts;        /* 64-UNIX Time at which the server transmitted its reply to the Client in a server time message
                                        (or the time client request was sent in the client request message).  */
};

/**
 * @brief Interface index type redirect */
typedef uint16_t InterfaceIndex_t;

/**
 * @brief SNTP Header (as specified in RFC 4330)
 * SNTP Time Message structure.  The Client only uses the flags field and the transmit_time_stamp field
 * in time requests it sends to its time server. */
__packed struct x_sntp_request
{

#ifdef _CPU_BIG_ENDIAN
/* These are represented as 8 bit data fields in the time message format.  */
	uint8_t      li   : 2;           /* Field 0     */
	uint8_t      vn   : 3;
	uint8_t      mode : 3;
#else
/* These are represented as 8 bit data fields in the time message format.  */
	uint8_t      mode : 3;           /* Field 0     */
	uint8_t      vn   : 3;
	uint8_t      li   : 2;
#endif

	uint8_t      stratum;            /* Field 1     */
	uint8_t      poll;               /* Field 2     */
	int8_t       precision;          /* Field 3     */

/* These are represented as 32 bit data fields in the time message format*/
	uint32_t     rootDelay;          /* Field 4-7   */
	uint32_t     rootDispersion;     /* Field 8-11  */
	uint32_t     referenceId;        /* Field 13-15 */

/* These are represented as 64 bit data fields in the time message format*/
	NtpTimestamp referenceTimestamp; /* Field 16-23 */
	NtpTimestamp originateTimestamp; /* Field 24-31 */
	NtpTimestamp receiveTimestamp;   /* Field 32-39 */
	NtpTimestamp transmitTimestamp;  /* Field 40-47 */
};

/**
 * @brief  Virtual socket structure
 * @remark Contain the descriptor for simplelink socket used by the @ref sntp_ex_lib_ti module */
struct vsocket
{
  struct
  {
    int                 event;       /* socket event.               */
    int16_t             type;        /* socket type.                */
    int16_t             protocol;    /* socket protocol.            */
    SlNetSock_Timeval_t timeout;     /* socket timeout value.       */
    SlNetSock_Addr_t    SocketAddr;  /* socket net address context. */
    uint16_t            InAddLength; /* socket net address length.  */
  }descriptor;

  int fd;  /* socket handle, (-1) is the initial value */
};

/**
 * @brief operational virtual table (port APIs)
 * @note  local time APIs, which will be linked with the user-space
 */
struct ud_op_vtable
{
  /* get local time format second, fraction */
  int8_t   ( * get_sntp_time )( uint32_t * pulseconds, uint32_t * pulfraction );

  /* get local timestamp format unix64 */
  uint64_t ( * get_unix_timestamp )( void );

  /* get operating system tick in ms */
  uint32_t ( * get_os_tick )( void );
};

/**
 * @brief  SNTP client handle
 * @remark User defined client will be runned with their specific handle, in order to manage multiple client */
typedef struct
{
  struct vsocket    * sock;
  struct ud_op_vtable vtable_api;
  uint32_t            timeout;
  InterfaceIndex_t    interface;
  uint32_t            startTime;
  udSntpClientState   state;
  uint8_t             payload[ exlibSNTP_TIME_MESSAGE_MAX_SIZE ];
  size_t              payloadLen;
  uint32_t            kissCode;
  struct xTimestampCtx_t * xTimestampList;

  /** Timestamp when the request was sent from client to server.
   *  This is used to check if the originated timestamp in the server
   *  reply matches the one in client request.
   */
  uint32_t             expected_orig_ts;
}sntpex_client_handle_t;

/**
 * @brief local_state_api API structure
 */
struct local_state_api_t
{
  sntp_ud_t( *execFuntion)( sntpex_client_handle_t * p_client );    /* function pointer */
};

/** @addtogroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
  * @{
  */
/* Exported function   ------------------------------------------------------------*/
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   Initialize the SNTP Client and set default configurations.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   p_vtable_api: Pointer to the user defined virtual table APIs.
 *          This parameter can be a value of @ref struct ud_op_vtable *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
sntp_ud_t sntpex_clientInitialization( sntpex_client_handle_t * p_client, struct ud_op_vtable * p_vtable_api );
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   Resolve NTP host name.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   pcHostname: Pointer to the NTP Host name server.
 *          This parameter can be a value of @ref const char *.
 * @param   pxhost_address: Pointer to net address structure.
 *          This parameter can be a value of @ref SlNetSock_Addr_t *.
 * @param   pxhost_address: Address family (IPV4/IPV6).
 *          This parameter can be a value of @ref uint8_t.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
sntp_ud_t sntpex_dns_host_by_name_get( InterfaceIndex_t * pxInterfaceIndex ,const char * pcHostname, SlNetSock_Addr_t * pxhost_address, uint8_t Family);
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   set sntp client timeout value.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   pxhost_address: timeout value in ms.
 *          This parameter can be a value of @ref timeout.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
sntp_ud_t sntpex_SetClientTimeout( sntpex_client_handle_t * p_client, uint32_t timeout);
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   bind client to interface.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   interface: Pointer to the interface.
 *          This parameter can be a value of @ref InterfaceIndex_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
sntp_ud_t sntpex_client_bind_to_interface(sntpex_client_handle_t *p_client, InterfaceIndex_t *interface);
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   set server address and port.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   serverIpAddr: Pointer to the net address.
 *          This parameter can be a value of @ref const SlNetSock_Addr_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
sntp_ud_t sntpex_client_set_server_address(sntpex_client_handle_t *p_client, const SlNetSock_Addr_t * serverIpAddr );
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   get timestamp list (format unix64/ format stime).
 *          T1,T2,T3 and T4 will be inserted to the passed list
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   xTimestampCtx: Pointer to timestamp list.
 *          This parameter can be a value of @ref struct xTimestampCtx_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
sntp_ud_t sntpex_client_timestamp_get( sntpex_client_handle_t *p_client, struct xTimestampCtx_t * xTimestampCtx );
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   get kiss of death code (KoD).
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  KoD value, this parameter can be a value ofe @ref uint32_t.
 */
uint32_t  sntpex_client_Kiss_code_get(sntpex_client_handle_t *p_client);
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   sntp client deinitialization.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  None.
 */
void      sntpex_client_deinitialization(sntpex_client_handle_t *p_client);
/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   Handle sntp events from ISR.
 * @param   None.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
sntp_ud_t sntpex_eventTriggingFromISR( void );
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SNTPEX_LIBRARY_EXTENDED_TI_SIMPLELINK_SNTP_LIB_H_ */

/************************ (C) COPYRIGHT RIDHA MASTOURI *****END OF FILE*****************/