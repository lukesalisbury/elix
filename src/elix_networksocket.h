/***********************************************************************************************************************
 Copyright (c) Luke Salisbury
 This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held
 liable for any damages arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter
 it and redistribute it freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
	If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is
	not required.
 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
	software.
 3. This notice may not be removed or altered from any source distribution.
***********************************************************************************************************************/
///Note: Requires building with -D_GNU_SOURCE=1 or --std=gnu*

#ifndef ELIX_NETWORKSOCKET_HEADER
#define ELIX_NETWORKSOCKET_HEADER

#include "elix_core.h"
#include "elix_cstring.h"

#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#ifdef PLATFORM_WINDOWS
	#include <winsock2.h>
	#include <ws2ipdef.h>
	#include <ws2tcpip.h>
	#include <iphlpapi.h>

	#define socklen_t size_t
	#define MSG_DONTWAIT 0
	#define NATIVE_BUFFER_TYPE(x) (char*)x
	#define NATIVE_LENGTH_TYPE(x) (int*)x

	#define elix_socket_handle int
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <netdb.h> // NI_MAXHOST and NI_NUMERICHOST are defined here
	#include <unistd.h>
	#include <errno.h>

	#define NATIVE_BUFFER_TYPE(x) x
	#define NATIVE_LENGTH_TYPE(x) x
	
	#define elix_socket_handle int
#endif

#define elix_ip_port uint16_t // Stored in Network byte order


#define ELIX_ALLOCATED_BUFFER_SIZE 1024
#define ELIX_SOCKET_NOTSET -1


#define RESULTS_ERROR_INVALID_PROTOCOL		0xA0
#define RESULTS_ERROR_LOCAL_MESSAGE			0xA1
#define RESULTS_ERROR_LISTEN				0xA2


enum ELIX_NETWORK_PROTOCOL{UDP, TCP};

typedef struct elix_allocated_buffer {
	uint8_t data[ELIX_ALLOCATED_BUFFER_SIZE];
	uint16_t data_size;
	uint16_t actual_size;
} elix_allocated_buffer;

typedef union elix_ip_address {
	struct  {
		union {
			uint8_t byte[16];
			uint16_t word[8];
		};
	} ip6;
	struct {
		uint32_t buffer[3];
		union {
			uint8_t octel[4];
			uint32_t ip;
		};
	} ip4;
	uint64_t raw[2];
} elix_ip_address;

typedef struct elix_network_interface elix_network_interface;
typedef struct elix_network_interface {
	elix_ip_address ip;
	elix_ip_port listening_port;
	elix_network_interface * next;
} elix_network_interface;

typedef struct elix_network_peer {
	elix_socket_handle socket_handle; // Should be set to INVALID_SOCKET
	elix_ip_address ip;
	elix_ip_port port;
} elix_network_peer;


typedef struct elix_networksocket_options {
	uint8_t listening;
	uint8_t public_only;
	uint8_t non_blocking;
} elix_networksocket_options;

typedef struct elix_networksocket {
	elix_socket_handle socket_handle;
	elix_network_interface * local_interfaces;
	uint8_t socket_type;
	uint8_t ignore_local_broadcast;
} elix_networksocket;

#ifdef __cplusplus
extern "C" {
#endif


static inline uint16_t elix_network_port_local(elix_ip_port port) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return ntohs(port);
#else
	return port;
#endif

}

static inline elix_ip_port elix_network_port_network(uint16_t port) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return htons(port);
#else
	return port;
#endif
}

uint8_t elix_mem_compare( const void * p1, const void * p2, size_t size );

size_t elix_mem_copy( const void * dest, const void * src, size_t size );


struct sockaddr_in elix_network_socket_address(elix_network_peer * peer);
elix_network_peer elix_network_ip_address(struct sockaddr_in * peer);
elix_network_peer elix_network_address_info( const char * domain, const char * port );

void elix_network_init();
void elix_network_deinit();

elix_network_interface * elix_network_gather_ip_addresses( uint8_t public_only );
void elix_network_interface_free(elix_network_interface * head);


void elix_networksocket_create(elix_networksocket * networksocket, uint8_t type, elix_network_peer * peer, elix_networksocket_options option);
void elix_networksocket_destroy(elix_networksocket * networksocket );

bool elix_networksocket_from_local_address(elix_networksocket * networksocket, elix_network_peer * remote_peer );
function_results elix_networksocket_listen_for_message(elix_networksocket * networksocket, elix_network_peer * remote_peer);
function_results elix_networksocket_receive_message(elix_networksocket * networksocket, elix_allocated_buffer * buffer, elix_network_peer * remote_peer, bool ignore_local_broadcast );
function_results elix_networksocket_send_message(elix_networksocket * networksocket, elix_network_peer * target, const uint8_t * message, uint64_t message_size);
void elix_networktsocket_close_peer(elix_network_peer * peer);


#ifdef __cplusplus
}
#endif

#endif // ELIX_NETWORKSOCKET_HEADER
