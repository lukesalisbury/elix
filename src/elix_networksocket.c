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

#include "elix_networksocket.h"

#if !defined (INVALID_SOCKET)
#define INVALID_SOCKET -1
#endif


uint8_t elix_mem_compare( const void * p1, const void * p2, size_t size ) {
	uint8_t * a = (uint8_t *)p1, * b = (uint8_t *)p2;
	for (size_t i = 0; i < size; ++i) {
		if ( a[i] != b[i] ) {
			return 0;
		}
	}
	return 1;
}

size_t elix_mem_copy( const void * dest, const void * src, size_t size ) {
	uint8_t * a = (uint8_t *)dest, * b = (uint8_t *)src;
	for (size_t i = 0; i < size; ++i) {
		 a[i] = b[i];
	}
	return size;
}

size_t elix_mem_flipcopy( const void * dest, const void * src, size_t size ) {
	uint8_t * a = (uint8_t *)dest, * b = (uint8_t *)src;
	size_t index = size - 1;
	for (size_t i = 0; i < size; ++i) {
		 a[i] = b[index-i];
	}
	return size;
}

struct sockaddr_in elix_network_socket_address(elix_network_peer * peer) {
	struct sockaddr_in address = {0};
	address.sin_port = htons(peer->port);

	if ( peer->ip.raw[0] ) {
		struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&address;
		address.sin_family = AF_INET6;
		elix_mem_copy( &addr->sin6_addr, &peer->ip.ip6.word, 128);
	} else {
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = peer->ip.ip4.ip;
	}
	return address;
}

elix_network_peer elix_network_ip_address(struct sockaddr_in * peer) {
	elix_network_peer output = {0};
	if ( peer->sin_family == AF_INET ) {
		output.ip.ip4.ip = peer->sin_addr.s_addr;
	} else if ( peer->sin_family == AF_INET6 ) {
		struct sockaddr_in6 * addr = (struct sockaddr_in6 *)peer;
		elix_mem_copy(&output.ip.ip6.word, &addr->sin6_addr, 128);
	}
	output.port = ntohs(peer->sin_port);
	return output;
}

void elix_network_ip_address_set(struct sockaddr_in * sock, elix_network_peer * peer) {
	if ( sock->sin_family == AF_INET ) {
		peer->ip.ip4.ip = sock->sin_addr.s_addr;
	} else if ( sock->sin_family == AF_INET6 ) {
		struct sockaddr_in6 * addr = (struct sockaddr_in6 *)sock;
		elix_mem_copy(&peer->ip.ip6.word, &addr->sin6_addr, 128);
	}
	peer->port = ntohs(sock->sin_port);
	
}

elix_network_peer elix_network_address_info( const char * domain, const char * port ) {
	elix_network_peer peer = {0};
	struct addrinfo * address = nullptr;
	int results = getaddrinfo(domain,port, nullptr, &address);

	if ( address ) {
		peer = elix_network_ip_address((struct sockaddr_in*) address->ai_addr);
		freeaddrinfo(address);
	}

	return peer;
}

void elix_networksocket_create(elix_networksocket * networksocket, uint8_t type, elix_network_peer *peer, elix_networksocket_options option) {
	networksocket->local_interfaces = elix_network_gather_ip_addresses(option.public_only);
	struct sockaddr_in address = elix_network_socket_address(peer);

	networksocket->socket_type = type;
	if ( networksocket->socket_type == TCP ) {
		networksocket->socket_handle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if ( option.listening ) {
			if ( bind(networksocket->socket_handle, (struct sockaddr*)&address, sizeof (address)) < 0 ) {
				LOG_ERROR( "IPPROTO_TCP Bind Error: %s", strerror(errno) );
			}
			if ( listen(networksocket->socket_handle, SOMAXCONN) < 0 ) {
				LOG_ERROR( "IPPROTO_TCP Listen Error: %s", strerror(errno) );
			}
		} else {
			if ( connect(networksocket->socket_handle, (struct sockaddr*)&address, sizeof (address)) < 0 ) {
				LOG_ERROR( "IPPROTO_TCP connect Error: %s", strerror(errno) );
			} else {
				//LOG_INFO( "Connected to %d.%d.%d.%d", peer->ip.ip4.octel[0], peer->ip.ip4.octel[1], peer->ip.ip4.octel[2], peer->ip.ip4.octel[3] );
			}
			peer->socket_handle = networksocket->socket_handle;
		}
	} else if ( networksocket->socket_type == UDP) {
		networksocket->socket_handle = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if ( bind(networksocket->socket_handle, (struct sockaddr*)&address, sizeof (address)) < 0 ) {
			LOG_ERROR( "IPPROTO_UDP Bind Error %s", strerror(errno) );
		}

	}
	#ifndef PLATFORM_WINDOWS
	///TODO: Handle Non blocking sockets
	if ( option.non_blocking ) {
		//fcntl(networksocket->socket_handle, F_SETFL, O_NONBLOCK);
	}
	#endif

	if ( option.listening ) {
		int broadcast = 1;
		#ifdef PLATFORM_WINDOWS
		setsockopt(networksocket->socket_handle, SOL_SOCKET, SO_BROADCAST, (char *) &broadcast, sizeof(broadcast));
		#else
		setsockopt(networksocket->socket_handle, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
		#endif
	}


}

void elix_networksocket_destroy(elix_networksocket * networksocket ) {
	shutdown(networksocket->socket_handle, SHUT_RDWR);
	close(networksocket->socket_handle);
}


bool elix_networksocket_from_local_address(elix_networksocket * networksocket, elix_network_peer * remote_peer ) {
	elix_network_interface * local_if = networksocket->local_interfaces;
	while (local_if) {
		if ( local_if->ip.raw[0] == remote_peer->ip.raw[0] && local_if->ip.raw[1] == remote_peer->ip.raw[1]) {
			return true;
			break;
		}
		local_if = local_if->next;
	}
	return false;
}

function_results elix_networksocket_poll_for_message(elix_networksocket * networksocket, elix_network_peer * remote_peer) {
	if ( networksocket->socket_type != TCP ) {
		return RESULTS_ERROR_INVALID_PROTOCOL;
	}
	int read = 0;
	struct sockaddr_in remote = {0};
	socklen_t remote_size = sizeof(remote);
	int peer_socket = 0;
	peer_socket = accept(networksocket->socket_handle, (struct sockaddr*)&remote, &remote_size);
	if ( peer_socket != INVALID_SOCKET ) {
		elix_network_ip_address_set(&remote, remote_peer);
		remote_peer->socket_handle = peer_socket;
		return RESULTS_SUCCESS;
	} else {
		LOG_ERROR("Listen error: %s", strerror(errno));
	}
	return RESULTS_ERROR_LISTEN;
}

function_results elix_networksocket_listen_for_message(elix_networksocket * networksocket, elix_network_peer * remote_peer) {
	if ( networksocket->socket_type != TCP ) {
		return RESULTS_ERROR_INVALID_PROTOCOL;
	}
	int read = 0;
	struct sockaddr_in remote = {0};
	socklen_t remote_size = sizeof(remote);
	int peer_socket = 0;
	peer_socket = accept(networksocket->socket_handle, (struct sockaddr*)&remote, &remote_size);
	if ( peer_socket != INVALID_SOCKET ) {
		elix_network_ip_address_set(&remote, remote_peer);
		remote_peer->socket_handle = peer_socket;
		return RESULTS_SUCCESS;
	} else {
		LOG_ERROR("Listen error: %s", strerror(errno));
	}
	return RESULTS_ERROR_LISTEN;
}

function_results elix_networksocket_response_message(elix_networksocket * networksocket, elix_allocated_buffer * buffer ) {
	int read = 0;
	buffer->actual_size = 0;

	if ( networksocket->socket_type == TCP ) {
		read = recv(networksocket->socket_handle, buffer->data, buffer->data_size, 0);
		if ( read > 0 ) {
			buffer->actual_size = read;
			//LOG_INFO( "Socket Read: %d Bytes", read);
		} else if ( read < 0 ) {
			LOG_ERROR( "recv Error: %s", strerror(errno));
		}
	} else {
		LOG_ERROR("For non TCP response, user elix_networksocket_receive_message");
	}

	if ( read > 0 ) {
		return RESULTS_SUCCESS;
	}
	return RESULTS_ERROR;
}

function_results elix_networksocket_receive_message2(elix_networksocket * networksocket, elix_network_peer * remote_peer, uint8_t * message, uint64_t message_size, uint64_t * byte_read ) {

	int read = 0;
	struct sockaddr_in remote;
	socklen_t remote_size = sizeof(remote);


	if ( networksocket->socket_type == TCP ) {
		// Server mode, use peer socket handle otherwise
		if ( remote_peer->socket_handle != INVALID_SOCKET ) {
			read = recv(remote_peer->socket_handle, message, message_size, 0);
		} else {
			//LOG_INFO( "remote_peer socket handle is invalid");
			read = recv(networksocket->socket_handle, message, message_size, 0);
		}

		if ( read > 0 ) {
			//LOG_INFO( "Socket Read: %d Bytes", read);
		} else if ( read < 0 ) {
			LOG_ERROR( "recv Error: %s", strerror(errno));
		}
	} else if ( networksocket->socket_type == UDP) {
		read = recvfrom(networksocket->socket_handle, NATIVE_BUFFER_TYPE(message), message_size, 0, (struct sockaddr*)&remote, NATIVE_LENGTH_TYPE(&remote_size));
		elix_network_ip_address_set(&remote, remote_peer);
	}


	if ( read > 0 ) {
		if ( byte_read ) {
			*byte_read = read;
		}
		return RESULTS_SUCCESS;
	}
	return RESULTS_ERROR;
}


function_results elix_networksocket_receive_message(elix_networksocket * networksocket, elix_allocated_buffer * buffer, elix_network_peer * remote_peer, bool ignore_local_broadcast ) {

	int read = 0;
	struct sockaddr_in remote;
	socklen_t remote_size = sizeof(remote);

	buffer->actual_size = 0;

	if ( networksocket->socket_type == TCP ) {
		// Server mode, use peer socket handle otherwise
		if ( remote_peer->socket_handle != INVALID_SOCKET ) {
			read = recv(remote_peer->socket_handle, buffer->data, buffer->data_size, 0);
		} else {
			//LOG_INFO( "remote_peer socket handle is invalid");
			read = recv(networksocket->socket_handle, buffer->data, buffer->data_size, 0);
		}

		if ( read > 0 ) {
			buffer->actual_size = read;
			//LOG_INFO( "Socket Read: %d Bytes", read);
		} else if ( read < 0 ) {
			LOG_ERROR( "recv Error: %s", strerror(errno));
		}
	} else if ( networksocket->socket_type == UDP) {
		read = recvfrom(networksocket->socket_handle, NATIVE_BUFFER_TYPE(buffer->data), buffer->data_size, 0, (struct sockaddr*)&remote, NATIVE_LENGTH_TYPE(&remote_size));
		buffer->actual_size = read;
		elix_network_ip_address_set(&remote, remote_peer);

		// Ignore Local Broadcast
		if ( ignore_local_broadcast ) {
			if (elix_networksocket_from_local_address(networksocket, remote_peer) ) {
				buffer->actual_size = 0;
				return RESULTS_ERROR_LOCAL_MESSAGE;
			}
		}
	}


	if ( read > 0 ) {
		return RESULTS_SUCCESS;
	}
	return RESULTS_ERROR;
}


function_results elix_networksocket_send_message2(elix_networksocket * networksocket, elix_network_peer * target, const uint8_t * message, uint64_t message_size, uint64_t * byte_sent) {
	struct sockaddr_in address = elix_network_socket_address(target);

	ssize_t results = 0;
	//LOG_PRINT("Sending Message to: %d.%d.%d.%d", target->ip.ip4.octel[0], target->ip.ip4.octel[1], target->ip.ip4.octel[2], target->ip.ip4.octel[3]);
	if ( networksocket->socket_type == TCP ) {
		results = send(target->socket_handle, NATIVE_BUFFER_TYPE(message), message_size, 0);
	} else if ( networksocket->socket_type == UDP) {
		results = sendto(networksocket->socket_handle, (const char*)message, message_size, 0, (struct sockaddr*)&address, sizeof (address));
	}

	if ( results < 0 ) {
		LOG_ERROR( "Send Error: %s", strerror(errno) );
		return RESULTS_ERROR;
	}
	
	if ( results != message_size ) {
		LOG_INFO( "Sent reports then message size" );
	}

	if ( byte_sent ) {
		*byte_sent = results;
	}

	return RESULTS_SUCCESS;
}


function_results elix_networksocket_send_message(elix_networksocket * networksocket, elix_network_peer * target, const uint8_t * message, uint64_t message_size) {
	struct sockaddr_in address = elix_network_socket_address(target);

	ssize_t results = 0;
	//LOG_PRINT("Sending Message to: %d.%d.%d.%d", target->ip.ip4.octel[0], target->ip.ip4.octel[1], target->ip.ip4.octel[2], target->ip.ip4.octel[3]);
	if ( networksocket->socket_type == TCP ) {
		results = send(target->socket_handle, NATIVE_BUFFER_TYPE(message), message_size, 0);
	} else if ( networksocket->socket_type == UDP) {
		results = sendto(networksocket->socket_handle, (const char*)message, message_size, 0, (struct sockaddr*)&address, sizeof (address));
	}

	if ( results < 0 ) {
		LOG_ERROR( "Send Error: %s", strerror(errno) );
		return RESULTS_ERROR;
	}
	
	if ( results != message_size ) {
		LOG_INFO( "Sent reports then message size" );
	}

	return RESULTS_SUCCESS;
}

void elix_networktsocket_close_peer(elix_network_peer * peer) {
	close(peer->socket_handle);
}




#ifdef PLATFORM_WINDOWS
void elix_network_init() {
	WSADATA wsaData;
	int err = WSAStartup(0x0202, &wsaData);
}
void elix_network_deinit() {
	WSACleanup();
}
elix_network_interface * elix_network_gather_ip_addresses() {
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
				elix_ip_address current_ip = {};

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
void elix_network_init() {}
void elix_network_deinit() {}

elix_network_interface * elix_network_gather_ip_addresses( uint8_t public_only ) {
	struct ifaddrs * list_if = nullptr, * current_if;
	char addr_buffer[NI_MAXHOST] = "";
	elix_network_interface * interfaces = nullptr, * new_interface = nullptr;
	if ( getifaddrs(&list_if) == 0 ) {
		current_if = list_if;
		while ( current_if != nullptr){
			size_t len = 0;
			
			elix_ip_address current_ip = {};
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

				if ( !elix_mem_compare(&addr->sin6_addr, &in6addr_loopback, sizeof(in6addr_loopback)) ) {
					goto skip;
				}
				elix_mem_copy(&current_ip.ip6.word, &addr->sin6_addr, 128);

			} else {
				goto skip;
			}

			//getnameinfo(current_if->ifa_addr, len, addr_buffer, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
			if ( public_only ) {
				///TODO: Fix quick hack
				/// Check netmask and gateway
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

void elix_network_interface_free(elix_network_interface * head) {
	elix_network_interface * tmp = nullptr;

	while (head != nullptr) {
		tmp = head;
		head = head->next;
		NULLIFY(tmp)
	}

}











