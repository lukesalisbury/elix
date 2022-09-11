#include "elix_core.h"
#include "elix_cstring.hpp"

#include <sys/types.h>
#include <fcntl.h>
#include <cstring>
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
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
	#define NATIVE_BUFFER_TYPE(x) x
	#define NATIVE_LENGTH_TYPE(x) x
	#define elix_socket_handle int
#endif

//#define IPOCTALTOLONG(a,b,c,d) d | (c << 8) | (b << 16) | (a << 24)


union elix_ipaddress {
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
};

struct elix_network_interface {
	elix_ipaddress ip;
	uint16_t listening_port;
	elix_network_interface * next;
};

struct elix_network_peer {
	elix_ipaddress ip;
	uint16_t port;
	elix_socket_handle socket_handle;
};

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

inline sockaddr_in elix_network_socket_address(elix_network_peer & peer) {
	sockaddr_in address = {};
	address.sin_port = htons(peer.port);

	if ( peer.ip.raw[0] ) {
		sockaddr_in6 * addr = (sockaddr_in6 *)&address;
		address.sin_family = AF_INET6;
		elix_memcopy( &addr->sin6_addr, &peer.ip.ip6.word, 128);
	} else {
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = peer.ip.ip4.ip;
	}
	return address;
}

inline elix_network_peer elix_network_ip_address(sockaddr_in & peer) {
	elix_network_peer output = {};
	if ( peer.sin_family == AF_INET ) {
		output.ip.ip4.ip = peer.sin_addr.s_addr;
	} else if ( peer.sin_family == AF_INET6 ) {
		sockaddr_in6 * addr = (sockaddr_in6 *)&peer;
		elix_memcopy(&output.ip.ip6.word, &addr->sin6_addr, 128);
	}
	output.port = peer.sin_port;
	return output;
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

inline elix_network_interface * elix_network_gather_ip_addresses( uint8_t public_only = 0) {
	ifaddrs * list_if = nullptr, * current_if;
	char addr_buffer[NI_MAXHOST] = "";
	elix_network_interface * interfaces = nullptr, * new_interface = nullptr;
	if ( getifaddrs(&list_if) == 0 ) {
		current_if = list_if;
		while ( current_if != nullptr){
			size_t len = 0;
			elix_ipaddress current_ip = {};
			if ( current_if->ifa_addr->sa_family == AF_INET ) {
				len = sizeof (sockaddr_in);
				sockaddr_in * addr = (sockaddr_in*)current_if->ifa_addr;

				if ( addr->sin_addr.s_addr == htonl(INADDR_LOOPBACK) ) {
					goto skip;
				}
				
				current_ip.ip4.ip = addr->sin_addr.s_addr;
			} else if ( current_if->ifa_addr->sa_family == AF_INET6 ) {
				len = sizeof (sockaddr_in6);
				sockaddr_in6 * addr = (sockaddr_in6*)current_if->ifa_addr;

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


			new_interface = new elix_network_interface();
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


class elix_networksocket {
	public:
	enum {UDP, TCP};

	uint8_t socket_type = TCP;
	elix_network_interface * local_interfaces = nullptr;

	elix_networksocket(uint8_t type, elix_network_peer &peer, uint8_t listening){
		local_interfaces = elix_network_gather_ip_addresses();
		sockaddr_in address = elix_network_socket_address(peer);

		socket_type = type;
		if ( type == TCP ) {
			socket_handle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if ( listening ) {
				if ( ::bind(socket_handle, (sockaddr*)&address, sizeof (address)) < 0 ) {
					LOG_INFO( "IPPROTO_TCP Bind Error: %s", std::strerror(errno) );
				}
				if ( ::listen(socket_handle, 10) < 0 ) {
					LOG_INFO( "IPPROTO_TCP Listen Error: %s", std::strerror(errno) );
				}
			} else {
				if ( ::connect(socket_handle, (sockaddr*)&address, sizeof (address)) < 0 ) {
					LOG_INFO( "IPPROTO_TCP connect Error: %s", std::strerror(errno) );
				} else {
					LOG_INFO( "Connected to %d.%d.%d.%d", peer.ip.ip4.octel[0], peer.ip.ip4.octel[1], peer.ip.ip4.octel[2], peer.ip.ip4.octel[3] );
				}
			}
		} else if ( type == UDP) {
			socket_handle = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if ( ::bind(socket_handle, (sockaddr*)&address, sizeof (address)) < 0 ) {
				LOG_INFO( "IPPROTO_UDP Bind Error %s", std::strerror(errno) );
			}
		}
		#ifndef PLATFORM_WINDOWS
		fcntl(socket_handle, F_SETFL, O_NONBLOCK);
		#endif

		if ( listening ) {
			int broadcast = 1;
			#ifdef PLATFORM_WINDOWS
			setsockopt(socket_handle, SOL_SOCKET, SO_BROADCAST, (char *) &broadcast, sizeof(broadcast));
			#else
			setsockopt(socket_handle, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
			#endif
		}

	}
	~elix_networksocket() {
		close_socket();
	}

	void close_socket() {
		LOG_INFO( "close_socket");
		::close(socket_handle);
	}

	bool from_local_address(elix_network_peer & remote_peer ) {
		elix_network_interface * local_if = local_interfaces;
		while (local_if) {
			if ( local_if->ip.raw[0] == remote_peer.ip.raw[0] &&
				 local_if->ip.raw[1] == remote_peer.ip.raw[1]) {
				return true;
				break;
			}
			local_if = local_if->next;
		}
		return false;
	}
	#if !defined (INVALID_SOCKET)
	#define INVALID_SOCKET -1
	#endif
	uint8_t listen_for_message(elix_network_peer & remote_peer) {
		if ( socket_type != TCP ) {
			return 0;
		}
		int read = 0;
		sockaddr_in remote = {};
		socklen_t remote_size = sizeof(remote);
		int peer_socket = 0;
		peer_socket = ::accept(socket_handle, (sockaddr*)&remote, &remote_size);
		if ( peer_socket != INVALID_SOCKET ) {

			remote_peer = elix_network_ip_address(remote);
			remote_peer.socket_handle = peer_socket;
			return 1;
		}
		return 0;
	}

	uint8_t receive_message(elix_allocated_buffer & buffer, elix_network_peer & remote_peer ) {

		int read = 0;
		sockaddr_in remote;
		socklen_t remote_size = sizeof(remote);

		buffer.actual_size = 0;

		if ( socket_type == TCP ) {
			//read = recvfrom(socket_handle, NATIVE_BUFFER_TYPE(buffer.data), buffer.data_size, MSG_DONTWAIT, (sockaddr*)&remote, NATIVE_LENGTH_TYPE(&remote_size));

			// Server mode, use peer socket handle otherwise
			if ( remote_peer.socket_handle != INVALID_SOCKET ) {
				read = recv(remote_peer.socket_handle, buffer.data, buffer.data_size, MSG_DONTWAIT);
			} else {
				read = recv(socket_handle, buffer.data, buffer.data_size, MSG_DONTWAIT);
			}

			if (read > 0) {
				buffer.actual_size = read;
				//LOG_INFO( "Sockect Read: %d Bytes", read);
			} else if ( read < 0 ) {
				//LOG_INFO( "IPPROTO_UDP Bind Error: %s", std::strerror(errno) <<  std::endl;
			}
		} else if ( socket_type == UDP) {
			read = recvfrom(socket_handle, NATIVE_BUFFER_TYPE(buffer.data), buffer.data_size, 0, (sockaddr*)&remote, NATIVE_LENGTH_TYPE(&remote_size));
			buffer.actual_size = read;
			remote_peer = elix_network_ip_address(remote);
		}

		remote_peer = elix_network_ip_address(remote);
		// Ignore Local Broadcast
		bool local_broadcast = from_local_address(remote_peer);


		if ( read > 0 && !local_broadcast) {
			return 1;
		} else {

		}
		return 0;
	}


	uint8_t send_message(elix_network_peer & target, const uint8_t * message, uint64_t message_size) {
		sockaddr_in address = elix_network_socket_address(target);

		int results = 0;
		if ( socket_type == TCP ) {
			results = ::send(socket_handle, NATIVE_BUFFER_TYPE(message), message_size, 0);
		} else if ( socket_type == UDP) {
			results = ::sendto(socket_handle, (const char*)message, message_size, 0, (sockaddr*)&address, sizeof (address));
		}

		if ( results == -1 ) {
			LOG_INFO( "Send Error: %s", std::strerror(errno) );
		}

		//LOG_INFO("Sending Message to: %d.%d.%d.%d", target.ip.ip4.octel[0], target.ip.ip4.octel[1], target.ip.ip4.octel[2], target.ip.ip4.octel[3]);
		//LOG_INFO("Results: %d ", results );
		return 0;
	}

private:
	int socket_handle;

};
