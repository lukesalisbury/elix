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

#ifndef ELIX_NETWORKSOCKET_HPP
#define ELIX_NETWORKSOCKET_HPP

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


#define ELIX_ALLOCATED_BUFFER_SIZE 1024

enum ELIX_SOCKETS{UDP, TCP};

typedef struct elix_allocated_buffer {
	uint8_t data[ELIX_ALLOCATED_BUFFER_SIZE];
	uint16_t data_size;
	uint16_t actual_size;
} elix_allocated_buffer;

typedef union elix_ipaddress {
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
} elix_ipaddress;

typedef struct elix_network_interface elix_network_interface;
typedef struct elix_network_interface {
	elix_ipaddress ip;
	uint16_t listening_port;
	elix_network_interface * next;
} elix_network_interface;

typedef struct elix_network_peer {
	elix_ipaddress ip;
	uint16_t port;
	elix_socket_handle socket_handle;
} elix_network_peer;


typedef struct elix_networksocket {
	uint8_t socket_type;
	elix_network_interface * local_interfaces;
	elix_socket_handle socket_handle;
	uint8_t ignore_local_broadcast;
} elix_networksocket;

#ifdef __cplusplus
extern "C" {
#endif

inline uint8_t elix_compare( const void * p1, const void * p2, size_t size ) {
	uint8_t * a = (uint8_t *)p1, * b = (uint8_t *)p2;
	for (size_t i = 0; i < size; ++i) {
		if ( a[i] != b[i] ) {
			return 0;
		}
	}
	return 1;
}

inline size_t elix_memcopy( const void * dest, const void * src, size_t size ) {
	uint8_t * a = (uint8_t *)dest, * b = (uint8_t *)src;
	for (size_t i = 0; i < size; ++i) {
		 a[i] = b[i];
	}
	return size;
}

static inline struct sockaddr_in elix_network_socket_address(elix_network_peer * peer) {
	struct sockaddr_in address = {};
	address.sin_port = htons(peer->port);

	if ( peer->ip.raw[0] ) {
		struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&address;
		address.sin_family = AF_INET6;
		elix_memcopy( &addr->sin6_addr, &peer->ip.ip6.word, 128);
	} else {
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = peer->ip.ip4.ip;
	}
	return address;
}

static inline elix_network_peer elix_network_ip_address(struct sockaddr_in * peer) {
	elix_network_peer output = {};
	if ( peer->sin_family == AF_INET ) {
		output.ip.ip4.ip = peer->sin_addr.s_addr;
	} else if ( peer->sin_family == AF_INET6 ) {
		struct sockaddr_in6 * addr = (struct sockaddr_in6 *)peer;
		elix_memcopy(&output.ip.ip6.word, &addr->sin6_addr, 128);
	}
	output.port = peer->sin_port;
	return output;
}

static inline void elix_network_ip_address_set(struct sockaddr_in * sock, elix_network_peer * peer) {
	if ( sock->sin_family == AF_INET ) {
		peer->ip.ip4.ip = sock->sin_addr.s_addr;
	} else if ( sock->sin_family == AF_INET6 ) {
		struct sockaddr_in6 * addr = (struct sockaddr_in6 *)sock;
		elix_memcopy(&peer->ip.ip6.word, &addr->sin6_addr, 128);
	}
	peer->port = sock->sin_port;
	
}

#ifdef PLATFORM_WINDOWS
inline void elix_network_init() {
	WSADATA wsaData;
	int err = WSAStartup(0x0202, &wsaData);
}
inline void elix_network_deinit() {
	WSACleanup();
}
inline elix_network_interface * elix_network_gather_ip_addresses() {
	elix_network_interface * interfaces = nullptr, * new_interface = nullptr;
	IP_ADAPTER_ADDRESSES peek_address;
	IP_ADAPTER_ADDRESSES * list_address = nullptr, * current_address = nullptr;
	unsigned long outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
	

	int8_t retries_counter = 8;
	DWORD adapters_results = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER, nullptr, &peek_address, &outBufLen );
	while ( adapters_results == ERROR_BUFFER_OVERFLOW && retries_counter-- > 0 ) {
		list_address = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
		adapters_results = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER, nullptr, list_address, &outBufLen );
		if (adapters_results == ERROR_BUFFER_OVERFLOW) {
			NULLIFY(list_address)
		}
	}

	if ( adapters_results == NO_ERROR )	{
		current_address = list_address;
		while (current_address != nullptr) {
			PIP_ADAPTER_UNICAST_ADDRESS pUnicast = current_address->FirstUnicastAddress;
			if (pUnicast != nullptr) {
				size_t len = 0;
				elix_ipaddress current_ip = {};

				for (uint8_t q = 0; pUnicast != nullptr; q++) {
					SOCKADDR *pSockAddr = pUnicast->Address.lpSockaddr;
					if ( pSockAddr->sa_family == AF_INET ) {
						len = sizeof (sockaddr_in);
						sockaddr_in * addr = (sockaddr_in*)pSockAddr;

						if ( addr->sin_addr.s_addr == INADDR_LOOPBACK ) {
							goto skip;
						}
						current_ip.ip4.ip = addr->sin_addr.s_addr;
					} else if ( pSockAddr->sa_family == AF_INET6 ) {
						len = sizeof (sockaddr_in6);
						sockaddr_in6 * addr = (sockaddr_in6*)pSockAddr;

						if ( !elix_compare(&addr->sin6_addr, &in6addr_loopback, sizeof(in6addr_loopback)) ) {
							goto skip;
						}
						elix_memcopy(&current_ip.ip6.word, &addr->sin6_addr, 128);

					} else {
						goto skip;
					}

					new_interface = new elix_network_interface();
					new_interface->next = interfaces;
					new_interface->ip = current_ip;

					interfaces = new_interface;

					skip:
					pUnicast = pUnicast->Next;
				}
			}


			current_address = current_address->Next;
		}
	}

	NULLIFY(list_address)

	return interfaces;
}
#else
inline void elix_network_init() {}
inline void elix_network_deinit() {}

inline elix_network_interface * elix_network_gather_ip_addresses( uint8_t public_only ) {
	struct ifaddrs * list_if = nullptr, * current_if;
	char addr_buffer[NI_MAXHOST] = "";
	elix_network_interface * interfaces = nullptr, * new_interface = nullptr;
	if ( getifaddrs(&list_if) == 0 ) {
		current_if = list_if;
		while ( current_if != nullptr){
			size_t len = 0;
			elix_ipaddress current_ip = {};
			if ( current_if->ifa_addr->sa_family == AF_INET ) {
				len = sizeof (struct sockaddr_in);
				struct sockaddr_in * addr = (struct sockaddr_in*)current_if->ifa_addr;

				if ( addr->sin_addr.s_addr == htonl(INADDR_LOOPBACK) ) {
					goto skip;
				}
				
				current_ip.ip4.ip = addr->sin_addr.s_addr;
			} else if ( current_if->ifa_addr->sa_family == AF_INET6 ) {
				len = sizeof (struct sockaddr_in6);
				struct sockaddr_in6 * addr = (struct sockaddr_in6*)current_if->ifa_addr;

				if ( !elix_compare(&addr->sin6_addr, &in6addr_loopback, sizeof(in6addr_loopback)) ) {
					goto skip;
				}
				elix_memcopy(&current_ip.ip6.word, &addr->sin6_addr, 128);

			} else {
				goto skip;
			}

			getnameinfo(current_if->ifa_addr, len, addr_buffer, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);

			if ( public_only ) {
				///TODO: Fix quick hack
				if ( current_ip.ip4.octel[0] == 0 ) {
					//LOG_INFO("Skipping: %d.%d.%d.%d", current_ip.ip4.octel[0], current_ip.ip4.octel[1], current_ip.ip4.octel[2], current_ip.ip4.octel[3], current_ip.ip4.ip, INADDR_LOOPBACK);
					goto skip;
				}
			}


			new_interface = ALLOCATE(elix_network_interface, 1);
			new_interface->next = interfaces;
			new_interface->ip = current_ip;

			interfaces = new_interface;

			skip:
			current_if = current_if->ifa_next;
		}
		freeifaddrs(list_if);
	}

	return interfaces;
}
#endif

inline void elix_network_interface_free(elix_network_interface * head) {
	elix_network_interface * tmp = nullptr;

	while (head != nullptr) {
		tmp = head;
		head = head->next;
		NULLIFY(tmp)
	}

}


void elix_networksocket_create(elix_networksocket * networksocket, uint8_t type, elix_network_peer * peer, uint8_t listening);
void elix_networksocket_destroy(elix_networksocket * networksocket );

bool elix_networksocket_from_local_address(elix_networksocket * networksocket, elix_network_peer * remote_peer );
uint8_t elix_networksocket_listen_for_message(elix_networksocket * networksocket, elix_network_peer * remote_peer);
uint8_t elix_networksocket_receive_message(elix_networksocket * networksocket, elix_allocated_buffer * buffer, elix_network_peer * remote_peer );
uint8_t elix_networksocket_send_message(elix_networksocket * networksocket, elix_network_peer * target, const uint8_t * message, uint64_t message_size);
void elix_networktsocket_close_peer(elix_network_peer * peer);


#ifdef __cplusplus
}
#endif

#endif // ELIX_NETWORKSOCKET_HPP
