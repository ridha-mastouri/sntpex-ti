/**
 * @file    sntpex_ti/sntp_ex_lib_ti.c
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

/* Includes ----------------------------------------------------------------------*/
#include "sntp_ex_lib_ti.h"

/* Private function   ------------------------------------------------------------*/
/**
 * @defgroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief Private functions used by @ref sntp_ex_lib_ti.c .
 * @note  All these functions is declared as static to avoid application call.
 *       + @ref sFct_sntp_OpenConnection
 *       + @ref sFct_sntp_ReceiveResponse
 *       + @ref sFct_sntp_SendRequest
 *       + @ref sFct_sntp_HandlingResponse
 *       + @ref sFct_sntp_CloseConnection
 * @{
 */
/**
 * @brief Open the sntp connection ( socket creation, set opts...) */
static sntp_ud_t sFct_sntp_OpenConnection  ( sntpex_client_handle_t * p_client );
/**
 * @brief Receive the sntp request */
static sntp_ud_t sFct_sntp_ReceiveResponse ( sntpex_client_handle_t * p_client );
/**
 * @brief Send the sntp request */
static sntp_ud_t sFct_sntp_SendRequest     ( sntpex_client_handle_t * p_client );
/**
 * @brief Handling and verify the sntp response */
static sntp_ud_t sFct_sntp_HandlingResponse( sntpex_client_handle_t * p_client );
/**
 * @brief Close the sntp connection */
static sntp_ud_t sFct_sntp_CloseConnection ( sntpex_client_handle_t * p_client );
/**
 * @}
 */

/* Private variables ---------------------------------------------------------*/
#pragma pack(push)
#pragma pack(1)

/**
 * @brief   virtual socket.
 * @details It contain the handle/descriptor of the udp socket such as: type, event, df ... */ 
static struct vsocket uvirtual_sock;

/**
 * @brief   Asynchrone Event stucture .
 * @details It contain event flag, SR Soft register and callback for user notification */ 
volatile struct ux_sntpAsynchEvent xsntpAsynchEvent;

/* pointer to the virtual table API */
struct ud_op_vtable * pg_vtable_api;

/**
 * @brief   local state api table.
 * @details It contain the function pointer of every defined state */ 
static const struct local_state_api_t sntp_sapi[ UD_SNTP_CLIENT_STATE_COMPLETE ] =
{
  [UD_SNTP_CLIENT_STATE_OPEN] =
  {
    .execFuntion = sFct_sntp_OpenConnection,   /* open connection p_api */
  },
  [UD_SNTP_CLIENT_STATE_SENDING] =
  {
    .execFuntion = sFct_sntp_SendRequest,      /* send resuest p_api */
  },
  [UD_SNTP_CLIENT_STATE_RECEIVING] =
  {
    .execFuntion = sFct_sntp_ReceiveResponse,  /* receive response p_api */
  },
  [UD_SNTP_CLIENT_STATE_HANDLING_RSP] =
  {
    .execFuntion = sFct_sntp_HandlingResponse, /* handling response p_api */
  },
  [UD_SNTP_CLIENT_STATE_CLOSE] =
  {
    .execFuntion = sFct_sntp_CloseConnection,  /* close connection p_api */
  },
};

#pragma pack(pop)


/* Private function   ------------------------------------------------------------*/
/**
 * @defgroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief Utility APIs
 *        Private functions used by @ref sntp_ex_lib_ti.c .
 * @note  All these functions is declared as __STATIC_INLINE to force compiler inligning.
 *       + @ref prv_utility_ntp_to_epoch
 *       + @ref prv_utility_build_request
 *       + @ref prv_utility_frac_to_usecs
 * @{
 */
/**
 * @brief Convert Fraction part from NTP fraction to EPOCH subsecond-fraction */
__STATIC_INLINE uint64_t  prv_utility_ntp_to_epoch ( uint32_t ulSeconds, uint32_t ulFraction );
/**
 * @brief Build the sntp request. */
__STATIC_INLINE sntp_ud_t prv_utility_build_request( sntpex_client_handle_t *p_client, void * p_payload );
/**
 * @brief Convert fraction to microseconds (Formula is inspired from NETX duo stack). */
__STATIC_INLINE uint32_t  prv_utility_frac_to_usecs( uint32_t fraction );
/**
 * @brief Register event and callback using @ref exlibSNTP_SOFTSR_RECV_BIT and @ref exlibSNTP_SOFTSR_SEND_BIT */
__STATIC_INLINE void      prv_utility_register_event  ( uint8_t eventbitField, pf_eventCallback  cb );
/**
 * @brief Unregister event and callback using @ref exlibSNTP_SOFTSR_RECV_BIT and @ref exlibSNTP_SOFTSR_SEND_BIT */
__STATIC_INLINE void      prv_utility_unregister_event( uint8_t eventbitField );
/**
 * @}
 */

/** @addtogroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
  * @{
  */
/* Exported function   ------------------------------------------------------------*/
/**
 * @defgroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief Exported functions used by @ref pal_os_ti_sntp.c .
 *       + @ref sntpex_clientInitialization
 *       + @ref sntpex_client_deinitialization
 *       + @ref sntpex_dns_host_by_name_get
 *       + @ref sntpex_SetClientTimeout
 *       + @ref sntpex_client_bind_to_interface
 *       + @ref sntpex_client_set_server_address
 *       + @ref sntpex_client_timestamp_get
 *       + @ref sntpex_client_Kiss_code_get
 *       + @ref sntpex_eventTriggingFromISR
 * @{
 */

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   Initialize the SNTP Client and set default configurations.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   p_vtable_api: Pointer to the user defined virtual table APIs.
 *          This parameter can be a value of @ref struct ud_op_vtable *.    
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
sntp_ud_t sntpex_clientInitialization( sntpex_client_handle_t * p_client, struct ud_op_vtable * p_vtable_api )
{
  /* Make sure the SNTP client context, virtual table APIs are valid */
  if( ( NULL == p_client ) || ( NULL == p_vtable_api ))
  {
    /* Return the error status. */
    return SNTPEX_ERR_NULL_PTR;
  }

  /* Clear SNTP client context and virtual socket handle */
  ( void )memset( p_client, 0, sizeof( sntpex_client_handle_t ) );
  ( void )memset( &uvirtual_sock, 0, sizeof( struct vsocket ) );

  /* Initialize simplelink virtual socket */
  uvirtual_sock.descriptor.event          = exlibPOLLIN_EVENT;
  uvirtual_sock.descriptor.type           = SLNETSOCK_SOCK_DGRAM;
  uvirtual_sock.descriptor.protocol       = SLNETSOCK_PROTO_UDP;
  uvirtual_sock.descriptor.InAddLength    =  0;
  uvirtual_sock.fd                        = -1;

  /* configure virtual socket Default timeout */
  uvirtual_sock.descriptor.timeout.tv_sec  = ( exlibSNTP_CLIENT_DEFAULT_TIMEOUT/1000 );
  uvirtual_sock.descriptor.timeout.tv_usec = ( exlibSNTP_CLIENT_DEFAULT_TIMEOUT%1000 )*1000;

  /* Normalize time stamp value */
  if(uvirtual_sock.descriptor.timeout.tv_usec >= 1000000)
  {
    uvirtual_sock.descriptor.timeout.tv_sec += 1;
    uvirtual_sock.descriptor.timeout.tv_usec -= 1000000;
  }
  else
  {
    /* Do Nothink. MISRA 15.7 */
  }
  
  p_client->timeout    = exlibSNTP_CLIENT_DEFAULT_TIMEOUT;

  /* Initialize pointers */
  p_client->sock       = &uvirtual_sock;
  p_client->vtable_api = *p_vtable_api;
  pg_vtable_api        = p_vtable_api;

  /* unregister receive event from ISR */
  prv_utility_unregister_event( exlibSNTP_SOFTSR_RECV_BIT );
  
  /* unregister receive event from ISR */
  prv_utility_unregister_event( exlibSNTP_SOFTSR_SEND_BIT );

  /* Change library state to opened */
  p_client->state = UD_SNTP_CLIENT_STATE_OPEN;

  /* Return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   Handle sntp events from ISR.
 * @param   None.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
sntp_ud_t sntpex_eventTriggingFromISR( void )
{
  /* Make sure the virtual table APIs and event register from ISR are valid */
  if( ( NULL == pg_vtable_api ) ||( xsntpAsynchEvent.event.SR == 0 ) )
  {
    /* Exit. event is not registred or lib is not initialized yet */
    return SNTPEX_ERR_FAULT_INIT;
  }

  if( xsntpAsynchEvent.event.SR && exlibSNTP_SOFTSR_RECV_BIT )
  {
    /* Clear event from SR Soft register */
    xsntpAsynchEvent.event.SR  &= ~(exlibSNTP_SOFTSR_RECV_BIT); 

    /* save unix 64 timestamp, method is a pointer to the user get_timestamp APIs @ref get_unix_timestamp */  
    xsntpAsynchEvent.timestamp = pg_vtable_api->get_unix_timestamp();

    if( xsntpAsynchEvent.event_cb != NULL )
    {
      /* execute registred library callback */
      xsntpAsynchEvent.event_cb( exlibSNTP_SOFTSR_RECV_BIT ); 
    }
  }
  else if( xsntpAsynchEvent.event.SR && exlibSNTP_SOFTSR_SEND_BIT )
  {
    /* Clear event from SR Soft register */
    xsntpAsynchEvent.event.SR  &= ~(exlibSNTP_SOFTSR_SEND_BIT); 

    /* save unix 64 timestamp, method is a pointer to the user get_timestamp APIs @ref get_unix_timestamp */  
    xsntpAsynchEvent.timestamp = pg_vtable_api->get_unix_timestamp();

    if( xsntpAsynchEvent.event_cb != NULL )
    {
      /* execute registred library callback */
      xsntpAsynchEvent.event_cb( exlibSNTP_SOFTSR_SEND_BIT );
    }
  }
  else
  {
    /* Do Nothing : MISRA 15.7 */
  }

  /* return status code*/
  return SNTPEX_SUCCESS;
}

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
#pragma optimize=speed
sntp_ud_t sntpex_dns_host_by_name_get( InterfaceIndex_t * pxInterfaceIndex ,const char *pcHostname, SlNetSock_Addr_t * pxhost_address, uint8_t Family)
{
  /* Make sure that the host-name and address structure are valid */
  if( ( NULL == pcHostname ) || ( NULL == pxhost_address ))
  {
    /* Return the error status. */
    return SNTPEX_ERR_NULL_PTR;
  }

  sntp_ud_t xLibReturnCode = SNTPEX_SUCCESS;
  uint32_t aHostAddress[4] = { 0, };
  uint16_t usAddressLen    = 1;

  /* Obtain the IP Address of machine on network, by machine name */
  *pxInterfaceIndex = SlNetUtil_getHostByName( 0, ( char * )pcHostname, strlen( pcHostname ), aHostAddress, &usAddressLen, ( uint8_t )Family );

  if( *pxInterfaceIndex > 0 )
  {
    switch ( Family )
    {
    case SLNETSOCK_AF_INET: /* V4 host address */
    {
      SlNetSock_AddrIn_t * pxhost_address_v4  = ( SlNetSock_AddrIn_t * )pxhost_address;

      pxhost_address_v4->sin_family      = SLNETSOCK_AF_INET;
      pxhost_address_v4->sin_addr.s_addr = exlibSLNETUTIL_HTONL(aHostAddress[0]);
    }
    break;

    case SLNETSOCK_AF_INET6: /* V4 host address */
    {
      SlNetSock_AddrIn6_t * pxhost_address_v6  = ( SlNetSock_AddrIn6_t * )&pxhost_address;

      pxhost_address_v6->sin6_family                 = SLNETSOCK_AF_INET6;
      pxhost_address_v6->sin6_addr._S6_un._S6_u32[0] = exlibSLNETUTIL_HTONL(aHostAddress[0]);
      pxhost_address_v6->sin6_addr._S6_un._S6_u32[1] = exlibSLNETUTIL_HTONL(aHostAddress[1]);
      pxhost_address_v6->sin6_addr._S6_un._S6_u32[2] = exlibSLNETUTIL_HTONL(aHostAddress[2]);
      pxhost_address_v6->sin6_addr._S6_un._S6_u32[3] = exlibSLNETUTIL_HTONL(aHostAddress[3]);
    }
    break;

    default:
      /* Return the error status. */
      xLibReturnCode = SNTPEX_ERR_DNS_RESOLVE;
      break;
    }
  }
  else
  {
    /* Return the error status. */
    xLibReturnCode = SNTPEX_ERR_DNS_RESOLVE;
  }

  /* Return the error status. */
  return xLibReturnCode;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   set sntp client timeout value.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   pxhost_address: timeout value in ms.
 *          This parameter can be a value of @ref timeout. 
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
sntp_ud_t sntpex_SetClientTimeout( sntpex_client_handle_t * p_client, uint32_t timeout)
{
  /* Make sure the SNTP client context is valid */
  if( NULL == p_client )
  {
    /* Return the error status. */
    return SNTPEX_ERR_NULL_PTR;
  }

  /* Save client timeout value */
  p_client->timeout = timeout;

  if ( NULL != p_client->sock )
  {
    /* Save virtual socket timeout */
    p_client->sock->descriptor.timeout.tv_sec = ( timeout/1000 );
    p_client->sock->descriptor.timeout.tv_usec = ( timeout%1000 )*1000;
    
    /* Normalize time stamp value */
    if(uvirtual_sock.descriptor.timeout.tv_usec >= 1000000)
    {
      uvirtual_sock.descriptor.timeout.tv_sec += 1;
      uvirtual_sock.descriptor.timeout.tv_usec -= 1000000;
    }
    else
    {
      /* Do Nothink. MISRA 15.7 */
    }

  }

  /* Return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   bind client to interface.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   interface: Pointer to the interface.
 *          This parameter can be a value of @ref InterfaceIndex_t *. 
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
sntp_ud_t sntpex_client_bind_to_interface( sntpex_client_handle_t *p_client, InterfaceIndex_t *pxInterfaceIndex )
{
  /* Make sure the SNTP client context is valid */
  if( NULL == p_client )
  {
    /* Return the error status. */
    return SNTPEX_ERR_NULL_PTR;
  }

  /** @remark FUTURE IMPLEMENTATION
   *  Bind the SNTP client to a particular network interface, to make sure the socket is in an unbound state.*/

  /* Avoid compilation warnings with restricted compiler */
  ( void )( p_client );
  ( void )( pxInterfaceIndex );

  /* Return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   set server address and port.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   serverIpAddr: Pointer to the net address.
 *          This parameter can be a value of @ref const SlNetSock_Addr_t *. 
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
sntp_ud_t sntpex_client_set_server_address(sntpex_client_handle_t *p_client, const SlNetSock_Addr_t *serverIpAddr )
{
  /* Make sure that the SNTP client context and server IP Address are valid */
  if( ( NULL == p_client ) || ( NULL == serverIpAddr ))
  {
    /* Return the error status. */
    return SNTPEX_ERR_NULL_PTR;
  }

  ( void )memcpy( &p_client->sock->descriptor.SocketAddr , serverIpAddr, sizeof( SlNetSock_Addr_t ) );

  if ( serverIpAddr->sa_family == SLNETSOCK_AF_INET)
  {
    /* setting v4 address length */
    p_client->sock->descriptor.InAddLength = sizeof( SlNetSock_AddrIn_t );
  }
  else if ( serverIpAddr->sa_family  == SLNETSOCK_AF_INET6 )
  {
    /* setting v6 address length */
    p_client->sock->descriptor.InAddLength = sizeof( SlNetSock_AddrIn6_t );
  }
  else
  {
    /* Return the error status. */
    return SNTPEX_ERROR;
  }

  /* Change library state to opened */
  p_client->state = UD_SNTP_CLIENT_STATE_OPEN;

  /* Return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   get kiss of death code (KoD).
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  KoD value, this parameter can be a value ofe @ref uint32_t.
 */
#pragma optimize=speed
uint32_t sntpex_client_Kiss_code_get( sntpex_client_handle_t *p_client )
{
  /* Return the specific kiss Kod, 0 is returned when SNTP client context is valid */
  return ( (p_client != NULL) ? p_client->kissCode : 0 ) ;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PUBLIC_APIS
 * @brief   sntp client deinitialization.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  None.
 */
#pragma optimize=speed
void sntpex_client_deinitialization(sntpex_client_handle_t *p_client)
{
  /* Make sure the SNTP client context is valid */
  if( p_client != NULL )
  {
    /* Make sure the virtual socket handle is valid */
    if ( NULL != p_client->sock )
    {
      /* Close previous connection; destroy udp socket, clear virtual socket */
      sntp_sapi[ UD_SNTP_CLIENT_STATE_CLOSE ].execFuntion( p_client );

      /* Remove virtual socket handle */
      p_client->sock = NULL;
    }
    else
    {
      /* Socket is not initialized yet, Do Nothing : MISRA 15.7 */
    }

    /* Clear SNTP client context */
    memset(p_client, 0, sizeof(sntpex_client_handle_t));

    /* Change library state to opened */
    p_client->state = UD_SNTP_CLIENT_STATE_OPEN;
  }
}

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
#pragma optimize=speed
sntp_ud_t sntpex_client_timestamp_get( sntpex_client_handle_t *p_client, struct xTimestampCtx_t * xTimestampCtx )
{
  /* Make sure that the SNTP client context and timestamp context are valid */
  if( ( NULL == p_client ) || ( NULL == xTimestampCtx ))
  {
    /* Return the error status. */
    return SNTPEX_ERR_NULL_PTR;
  }

  sntp_ud_t xLibReturnCode = SNTPEX_SUCCESS;

  /* set entry tick for global timeout generation */
  p_client->startTime = p_client->vtable_api.get_os_tick();

  /* Initialize timestamp list pointer */
  p_client->xTimestampList = xTimestampCtx;

  /* run finite state machine */
  while( ( xLibReturnCode == SNTPEX_SUCCESS ) && ( p_client->state != UD_SNTP_CLIENT_STATE_COMPLETE ) )
  {
    /* execute state function */
    xLibReturnCode = sntp_sapi[ p_client->state ].execFuntion( p_client );
  }

  if ( xLibReturnCode == SNTPEX_SUCCESS )
  {
    /* Change library state to sending */
    p_client->state = UD_SNTP_CLIENT_STATE_SENDING;
  }
  else
  {
    /* unregister receive event from ISR */
    prv_utility_unregister_event( exlibSNTP_SOFTSR_RECV_BIT );
    
    /* close previous connection and change library state to opened */
    sntp_sapi[ UD_SNTP_CLIENT_STATE_CLOSE ].execFuntion( p_client );
  }

  /* Return the error status. */
  return xLibReturnCode;
}
/** @} */
/** @} */

/** @addtogroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
  * @{
  */
/* Private function   ------------------------------------------------------------*/
/**
 * @defgroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief Private functions used by @ref sntp_ex_lib_ti.c .
 * @note  All these functions is declared as static to avoid application call.
 *       + @ref sFct_sntp_OpenConnection
 *       + @ref sFct_sntp_ReceiveResponse
 *       + @ref sFct_sntp_SendRequest
 *       + @ref sFct_sntp_HandlingResponse
 *       + @ref sFct_sntp_CloseConnection
 *       + @ref prv_utility_build_request
 *       + @ref prv_utility_ntp_to_epoch
 *       + @ref prv_utility_register_event 
 *       + @ref prv_utility_unregister_event 
 * @{
 */

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   Build the sntp request.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @param   p_payload: Pointer to the payload.
 *          This parameter can be a value of @ref void *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
__STATIC_INLINE sntp_ud_t prv_utility_build_request( sntpex_client_handle_t * p_client, void * p_payload )
{
  /* Point to the buffer where to format the NTP payload */
  struct x_sntp_request * request = ( struct x_sntp_request * ) p_payload;

  /** @remark The client initializes the NTP payload header. For this purpose, all the NTP 
   *  header fields are set to 0, except the Mode, VN, and optional Transmit Timestamp fields*/
  memset( request, 0, sizeof( struct x_sntp_request ));

  /* Format the NTP request */
  request->vn        = specNTP_VERSION_V4;
  request->mode      = specNTP_MODE_CLIENT;
  request->stratum   = 2;
  request->poll      = 0x06;
  request->precision = 0xec; /* -6 */

  /** @remark The Transmit Timestamp allows a simple calculation to determine the
   *  propagation delay between the server and client and to align the system
   *  clock generally within a few tens of milliseconds relative to the server */

  /* Time at which the NTP request was sent */
  p_client->expected_orig_ts = p_client->vtable_api.get_os_tick();
  request->transmitTimestamp.seconds = 0;
  request->transmitTimestamp.fraction = exlibSLNETUTIL_HTONL(p_client->expected_orig_ts) ;

  /* save the payload length */
  p_client->payloadLen = sizeof( struct x_sntp_request);

  /* Return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   Open the sntp connection ( socket creation, set opts...)
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
static sntp_ud_t sFct_sntp_OpenConnection( sntpex_client_handle_t * p_client )
{
  /* Point to the socket extended Library structure */
  struct vsocket * p_socket   = ( struct vsocket *)p_client->sock;

  void           * socket_opt = NULL;
  uint32_t         opt_length = 0;
  int16_t          opt_name   = 0;
  int32_t          SLReturnCode = SLNETERR_RET_CODE_OK;

  if ( NULL == p_socket )
  {
    /* sntpex_client is not initialized yet, Return the error status*/
    return SNTPEX_ERR_NULL_PTR;
  }

  /* close previous UDP socket only when it is opened */
  if ( p_socket->fd != -1 )
  {
    /* close previous UDP socket */
    SlNetSock_close( p_socket->fd );

    /* reset virtual socket descriptor */
    p_socket->fd = -1;
  }

  /* Create a UDP socket to communicate with NTP server */
  p_socket->fd = SlNetSock_create( p_socket->descriptor.SocketAddr.sa_family,
                                   p_socket->descriptor.type,
                                   p_socket->descriptor.protocol,
                                   p_client->interface,
                                   0 );
  if ( p_socket->fd < 0 )
  {
    /* Socket cannot be created, return the error status. */
    return SNTPEX_ERR_SOCKET_CREATE;
  }

#ifdef exlibSNTP_CLIENT_USE_NONBLOCKING_TIMEOUT_OPTION  

  /** @remark Non blocking Timeout option, so timeout mecanism is handled on the @ref sntp_ex_ti_lib file
   *  Keep pooling until the return status is different from @ref SLNETERR_BSD_EAGAIN and timeout is not occured */
  SlNetSock_Nonblocking_t enableOption =
  {
    .nonBlockingEnabled = 1, 
  };

  socket_opt = &enableOption;
  opt_name   = SLNETSOCK_OPSOCK_NON_BLOCKING;
  opt_length = sizeof( enableOption );

#else

  /** @remark blocking Timeout option, so timeout mecanism is handled on the TI TCP Stack.
   *  The timeout value is @ref p_client->sock->descriptor.timeout */
  socket_opt = &p_client->sock->descriptor.timeout;
  opt_name   = SLNETSOCK_OPSOCK_RCV_TIMEO;
  opt_length = sizeof( SlNetSock_Timeval_t );

#endif /* exlibSNTP_CLIENT_USE_NONBLOCKING_TIMEOUT_OPTION */
  
  /* Setting socket options */
  SLReturnCode = SlNetSock_setOpt( p_client->sock->fd,
                                   SLNETSOCK_LVL_SOCKET,
                                   opt_name,
                                   socket_opt,
                                   opt_length);
  if ( SLReturnCode < 0 )
  {
    /* Setting socket options is failed, return the error status. */
    return SNTPEX_ERR_SOCKET_SET_OPT;
  }

  /* Everything is OK, change library state to sending */
  p_client->state = UD_SNTP_CLIENT_STATE_SENDING;

  /* return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   send the sntp request.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
static sntp_ud_t sFct_sntp_SendRequest( sntpex_client_handle_t * p_client )
{

  /* Point to the socket extended Library structure */
  struct vsocket * p_socket   = ( struct vsocket *)p_client->sock;

  int32_t          SLReturnCode = SLNETERR_RET_CODE_OK;

  /* Create NTP request which will be stored on the @ref p_client->payload with size @ref p_client->payloadLen */
  prv_utility_build_request( p_client, p_client->payload );

  /* save the originate unix 64 timestamp T1, method is a pointer to the user get_timestamp APIs @ref get_unix_timestamp */
  p_client->xTimestampList->originate64_ts = p_client->vtable_api.get_unix_timestamp();

#ifdef exlibSNTP_CLIENT_USE_NONBLOCKING_TIMEOUT_OPTION

  /** @remark Non blocking Timeout option, so timeout mecanism is handled on the @ref sntp_ex_ti_lib file
   *  Keep pooling until the return status is different from @ref SLNETERR_BSD_EAGAIN and timeout is not occured */

  /* save the initial tick, used for timeout trigging */
  uint32_t ulTickStart = p_client->vtable_api.get_os_tick();

  do
  {
    /*  Write data to UDP socket, in order to will be sended to the configured server 
        SLReturnCode will return the length of sended payload */
    SLReturnCode =  SlNetSock_sendTo( p_socket->fd,
                                      &p_client->payload,
                                      p_client->payloadLen,
                                      0,
                                      &p_socket->descriptor.SocketAddr,
                                      p_socket->descriptor.InAddLength );
  }
  while( ( SLReturnCode == ( SLNETERR_BSD_EAGAIN ) ) && ( ( p_client->vtable_api.get_os_tick() - ulTickStart ) < p_client->timeout ) );

#else

  /** @remark blocking Timeout option, so timeout mecanism is handled on the TI TCP Stack.
   *  The timeout value is @ref p_client->sock->descriptor.timeout */

  /*  Write data to UDP socket, in order to will be sended to the configured server 
      SLReturnCode will return the length of sended payload */
  SLReturnCode =  SlNetSock_sendTo( p_socket->fd,
                                    &p_client->payload,
                                    p_client->payloadLen,
                                    0,
                                    &p_socket->descriptor.SocketAddr,
                                    p_socket->descriptor.InAddLength );

#endif /* exlibSNTP_CLIENT_USE_NONBLOCKING_TIMEOUT_OPTION */

  if ( SLNETERR_BSD_EAGAIN == SLReturnCode )
  {
    /* Timeout is occured, return the error status. */
    return SNTPEX_ERR_TIMEOUT;
  }
  else if ( ( SLReturnCode < 0 ) || ( SLReturnCode != p_client->payloadLen ) )
  {
    /* Error send NTP request, return the error status. */
    return SNTPEX_ERR_TX;
  }
  else
  {
    /* clear the payload, to will be used on the receive state */
    ( void ) memset( &p_client->payload, 0, p_client->payloadLen );

    /* Everything is OK, change library state to receiving */
    p_client->state = UD_SNTP_CLIENT_STATE_RECEIVING;

    /* return the error status. */
    return SNTPEX_SUCCESS;
  }
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   receive the sntp request
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
static sntp_ud_t sFct_sntp_ReceiveResponse( sntpex_client_handle_t *p_client )
{

  /* Point to the socket extended Library structure */
  struct vsocket * p_socket   = ( struct vsocket *)p_client->sock;

  int32_t          SLReturnCode = SLNETERR_RET_CODE_OK;

  /* register receive event from ISR */
  prv_utility_register_event( exlibSNTP_SOFTSR_RECV_BIT, NULL );

#ifdef exlibSNTP_CLIENT_USE_NONBLOCKING_TIMEOUT_OPTION

  /** @remark Non blocking Timeout option, so timeout mecanism is handled on the @ref sntp_ex_ti_lib file
   *  Keep pooling until the return status is different from @ref SLNETERR_BSD_EAGAIN and timeout is not occured */

  /* save the initial tick, used for timeout trigging */
  uint32_t ulTickStart = p_client->vtable_api.get_os_tick();
  do
  {
    /* Read data from socket, SLReturnCode will return the length of received payload */
    SLReturnCode =  SlNetSock_recvFrom( p_socket->fd,
                                        &p_client->payload,
                                        p_client->payloadLen,
                                        0,
                                        &p_socket->descriptor.SocketAddr,
                                        &p_socket->descriptor.InAddLength );
  }
  while( ( SLReturnCode == ( SLNETERR_BSD_EAGAIN ) ) && ( ( p_client->vtable_api.get_os_tick() - ulTickStart ) < p_client->timeout ) );
#else

  /** @remark blocking Timeout option, so timeout mecanism is handled on the TI TCP Stack.
   *  The timeout value is @ref p_client->sock->descriptor.timeout */

  /* Read data from socket, SLReturnCode will return the length of received payload */
  SLReturnCode =  SlNetSock_recvFrom( p_socket->fd,
                                      &p_client->payload,
                                      p_client->payloadLen,
                                      0,
                                      &p_socket->descriptor.SocketAddr,
                                      &p_socket->descriptor.InAddLength );

#endif /* exlibSNTP_CLIENT_USE_NONBLOCKING_TIMEOUT_OPTION */

  if ( SLNETERR_BSD_EAGAIN == SLReturnCode )
  {
    /* Timeout is occured, return the error status. */
    return SNTPEX_ERR_TIMEOUT;
  }
  else if ( SLReturnCode < 0 )
  {
    /* Error receive NTP request, return the error status. */
    return SNTPEX_ERR_RX;
  }
  else if ( SLReturnCode != p_client->payloadLen )
  {
    /* invalid payload length, return the error status. */
    return SNTPEX_ERR_INVALID_MESSAGE;
  }
  else
  {
    /* Error occured on EVENT ISR*/
    if( xsntpAsynchEvent.timestamp == ( uint64_t )0 )
    {
      return SNTPEX_ERR_INVALID_MESSAGE;
    }

    /* save the reference unix 64 timestamp T4, method is a pointer to the user get_timestamp APIs @ref get_unix_timestamp */  
    p_client->xTimestampList->reference64_ts = xsntpAsynchEvent.timestamp;
    
    /* unregister receive event from ISR */
    prv_utility_unregister_event( exlibSNTP_SOFTSR_RECV_BIT );
  
    /* Everything is OK, change library state to handling response */
    p_client->state = UD_SNTP_CLIENT_STATE_HANDLING_RSP;

    /* return the error status. */
    return SNTPEX_SUCCESS;
  }
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   close the sntp connection
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
static sntp_ud_t sFct_sntp_CloseConnection( sntpex_client_handle_t * p_client )
{
  /* Point to the socket extended Library structure */
  struct vsocket * p_socket   = ( struct vsocket * )p_client->sock;

  if ( NULL == p_socket )
  {
    /* sntpex_client is not initialized yet, Return the error status*/
    return SNTPEX_ERR_NULL_PTR;
  }
  
  /* close UDP socket only when it is opened */
  if ( p_client->sock->fd != -1 )
  {
    /* close previous UDP socket */
    SlNetSock_close( p_client->sock->fd );

    /* reset virtual socket descriptor */
    p_client->sock->fd = -1;
  }

  /* Everything is OK, change library state to open connection */
  p_client->state = UD_SNTP_CLIENT_STATE_OPEN;

  /* return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   Handling and verify the sntp response.
 * @param   p_client: Pointer to the sntp client handle
 *          This parameter can be a value of @ref sntpex_client_handle_t *.
 * @retval  SNTPEX_SUCCESS if successful, A specific error type @ref sntp_ud_t otherwise.
 */
#pragma optimize=speed
static sntp_ud_t sFct_sntp_HandlingResponse( sntpex_client_handle_t * p_client )
{
  /* casting payload to request structure */
  struct x_sntp_request * responce = ( struct x_sntp_request * ) p_client->payload;
  
  /* Ensure the NTP packet is valid */
  if( p_client->payloadLen < sizeof( struct x_sntp_request ) )
  {
    /* return the error status. */
    return SNTPEX_ERR_INVALID_MESSAGE;
  }
    
  /* The server reply should be discarded if the VN field is 0 */
  if( responce->vn == 0 )
  {
    /* return the error status. */
    return SNTPEX_ERR_INVALID_MESSAGE;
  }

  /* The server reply should be discarded if the Transmit Timestamp fields is 0 */
  if( responce->transmitTimestamp.val == 0 )
  {
    /* return the error status. */
    return SNTPEX_ERR_INVALID_MESSAGE;
  }

  /* The server reply should be discarded if the Mode field is not 4 (unicast) or 5 (broadcast) */
  if(responce->mode != specNTP_MODE_SERVER && responce->mode != specNTP_MODE_BROADCAST)
  {
    /* return the error status. */
    return SNTPEX_ERR_INVALID_MESSAGE;
  }

  /** Timestamp when the request was sent from client to server.
   *  This is used to check if the originated timestamp in the server
   *  reply matches the one in client request.
   */
  if ( ( responce->originateTimestamp.seconds  != 0 ) ||
       ( responce->originateTimestamp.fraction != exlibSLNETUTIL_NTOHL(p_client->expected_orig_ts) ) )
  {
    /* return the error status. */
    return SNTPEX_ERR_INVALID_MESSAGE;
  }

  /* Clear kiss code */
  p_client->kissCode = 0;

  /* Kiss-of-Death packet received? */
  if(responce->stratum == 0)
  {
    /* The kiss code is encoded in four-character ASCII strings left justified and zero filled */
    p_client->kissCode = exlibSLNETUTIL_NTOHL(responce->referenceId);

    /* An SNTP client should stop sending to a particular server if that server returns a reply with a Stratum field of 0 */
    return SNTPEX_ERR_REQUEST_REJECTED;
  }

  /* export transmit timestamp ( 64 bit unix format ) */
  p_client->xTimestampList->transmit64_ts = prv_utility_ntp_to_epoch( exlibSLNETUTIL_NTOHL(responce->transmitTimestamp.seconds  ),
                                                                      exlibSLNETUTIL_NTOHL(responce->transmitTimestamp.fraction ) );

  /* export receive timestamp ( 64 bit unix format ) */
  p_client->xTimestampList->receive64_ts  = prv_utility_ntp_to_epoch( exlibSLNETUTIL_NTOHL(responce->receiveTimestamp.seconds ),
                                                                      exlibSLNETUTIL_NTOHL(responce->receiveTimestamp.fraction ) );

  /* export reference timestamp ( 32 bit sec, 32 bit frac ) */
  p_client->xTimestampList->referenceTimestamp.seconds  = exlibSLNETUTIL_NTOHL( responce->referenceTimestamp.seconds );
  p_client->xTimestampList->referenceTimestamp.fraction = exlibSLNETUTIL_NTOHL( responce->referenceTimestamp.fraction );

  /* export receive timestamp ( 32 bit sec, 32 bit frac ) */
  p_client->xTimestampList->receiveTimestamp.seconds    = exlibSLNETUTIL_NTOHL( responce->receiveTimestamp.seconds );
  p_client->xTimestampList->receiveTimestamp.fraction   = exlibSLNETUTIL_NTOHL( responce->receiveTimestamp.fraction );

  /* export transmit timestamp ( 32 bit sec, 32 bit frac ) */
  p_client->xTimestampList->transmitTimestamp.seconds   = exlibSLNETUTIL_NTOHL( responce->transmitTimestamp.seconds );
  p_client->xTimestampList->transmitTimestamp.fraction  = exlibSLNETUTIL_NTOHL( responce->transmitTimestamp.fraction );

  /* Everything is OK, change library state to complete */
  p_client->state = UD_SNTP_CLIENT_STATE_COMPLETE;

  /* return the error status. */
  return SNTPEX_SUCCESS;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   Convert fraction to microseconds (Formula is inspired from NETX duo stack)
 * @param   fraction: Number of fractions
 *          This parameter can be a value of @ref uint32_t.
 * @retval  Microseconds value, this parameter can be a value of @ref uint32_t.
 */
#pragma optimize=speed
__STATIC_INLINE uint32_t prv_utility_frac_to_usecs( uint32_t fraction )
{
  uint32_t value, segment;
  int index;

  value = 0;

  for( index = 2; index < 32; index+=6)
  {
    segment = (fraction >> index ) & 0x3F;

    value = ( ( value & 0x3F ) >= 32) ? (value  >> 6) + segment * 15625 + 1 : (value  >> 6) + segment * 15625 ;
  }

  return value;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   Convert Fraction part from NTP fraction to EPOCH subsecond-fraction
 * @param   ulSeconds: Number of seconds
 *          This parameter can be a value of @ref uint32_t.
 * @param   fraction: Number of fractions
 *          This parameter can be a value of @ref uint32_t.
 * @retval  unix-64 timestamp, this parameter can be a value of @ref uint64_t.
 */
#pragma optimize=speed
__STATIC_INLINE uint64_t prv_utility_ntp_to_epoch( uint32_t ulSeconds, uint32_t ulFraction )
{
  /** @remark Return the following calculated process :
   *  Convert Fraction part from NTP fraction(1/2^32) to EPOCH subsecond-fraction( 1/10^6 ) by dividing by (2^32) then multiplying by (10^6) 
   *  Deduct the time difference in seconds between NTP 1900 and EPOCH 1970, then add the microseconds from the fraction part*/
  return ( ( uint64_t )( ( ulSeconds - exlibDIFF_SEC_1900_1970) * 1000000 ) + ( uint64_t )( prv_utility_frac_to_usecs( ulFraction ) ) );
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   Register event and callback using @ref exlibSNTP_SOFTSR_RECV_BIT and @ref exlibSNTP_SOFTSR_SEND_BIT
 * @param   eventbitField: event field 
 *          This parameter can be a value of @ref uint8_t.
 * @param   cb: callback function
 *          This parameter can be a value of @ref pf_eventCallback.
 * @retval  None.
 */
#pragma optimize=speed
__STATIC_INLINE void prv_utility_register_event( uint8_t eventbitField, pf_eventCallback cb )
{
  /* register callback */
  xsntpAsynchEvent.event_cb  = cb;
  
  /* register event */
  xsntpAsynchEvent.event.SR |= eventbitField;
}

/**
 * @ingroup EXTENDED_SNTP_LIBRARY_TI_STACK_PRIVATE_APIS
 * @brief   Unregister event and callback using @ref exlibSNTP_SOFTSR_RECV_BIT and @ref exlibSNTP_SOFTSR_SEND_BIT
 * @param   eventbitField: event field 
 *          This parameter can be a value of @ref uint8_t.
 * @retval  None.
 */
#pragma optimize=speed
__STATIC_INLINE void prv_utility_unregister_event( uint8_t eventbitField )
{
  /* Clear previous timestamp */
  xsntpAsynchEvent.timestamp = ( uint64_t )0;

  /* Clear callback */
  xsntpAsynchEvent.event_cb  = NULL;

  /* Clear event from SR Soft register */
  xsntpAsynchEvent.event.SR &= ~(eventbitField);
}
/** @} */
/** @} */

/************************************* NTP packet structure ************************************
 
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                  |MSb		 LSb|MSb         LSb|MSb         LSb|MSb         LSb|
                +-+-+-+-+-+-+-+-+-+-+-+- NTP Packet Header -+-+-+-+-+-+-+-+-+-+-+
                  |0 1|2 3 4|5 6 7|8 9 0 1 2 3 4 5|6 7 8 9 0 1 2 3|4 5 6 7 8 9 0 1|
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x00|L I| V N |Mode |    Stratum    |     Poll      |   Precision   |
                  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x04|                          Root Delay                           |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x08|                        Root Dispersion                        |
                  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x0C|                     Reference Identifier                      |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x10|                    Reference Timestamp (64)                   |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x18|                    Originate Timestamp (64)                   |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x20|                     Receive Timestamp (64)                    |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x28|                     Transmit Timestamp (64)                   |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x30|                 Key Identifier (optional) (32)                |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              0x34|                 Message Digest (optional) (128)               |
                +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                Getting the data from the Transmit Timestamp (seconds) field
                This is the time at which the reply departed the server for the client
                https://www.meinbergglobal.com/english/info/ntp-packet.htm
                https://tools.ietf.org/html/rfc5905									

 **********************************************************************************************/


/************************ (C) COPYRIGHT RIDHA MASTOURI *****END OF FILE*****************/