#include "elix_networksocket.h"

#if !defined (INVALID_SOCKET)
#define INVALID_SOCKET -1
#endif

void elix_networksocket_create(elix_networksocket * networksocket, uint8_t type, elix_network_peer *peer, uint8_t listening) {
	networksocket->local_interfaces = elix_network_gather_ip_addresses(0);
	struct sockaddr_in address = elix_network_socket_address(peer);

	networksocket->socket_type = type;
	if ( networksocket->socket_type == TCP ) {
		networksocket->socket_handle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if ( listening ) {
			if ( bind(networksocket->socket_handle, (struct sockaddr*)&address, sizeof (address)) < 0 ) {
				LOG_INFO( "IPPROTO_TCP Bind Error: %s", strerror(errno) );
			}
			if ( listen(networksocket->socket_handle, SOMAXCONN) < 0 ) {
				LOG_INFO( "IPPROTO_TCP Listen Error: %s", strerror(errno) );
			}
		} else {
			if ( connect(networksocket->socket_handle, (struct sockaddr*)&address, sizeof (address)) < 0 ) {
				LOG_INFO( "IPPROTO_TCP connect Error: %s", strerror(errno) );
			} else {
				LOG_INFO( "Connected to %d.%d.%d.%d", peer->ip.ip4.octel[0], peer->ip.ip4.octel[1], peer->ip.ip4.octel[2], peer->ip.ip4.octel[3] );
			}
		}
	} else if ( networksocket->socket_type == UDP) {
		networksocket->socket_handle = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if ( bind(networksocket->socket_handle, (struct sockaddr*)&address, sizeof (address)) < 0 ) {
			LOG_INFO( "IPPROTO_UDP Bind Error %s", strerror(errno) );
		}
	}
	#ifndef PLATFORM_WINDOWS
	fcntl(networksocket->socket_handle, F_SETFL, O_NONBLOCK);
	#endif

	if ( listening ) {
		int broadcast = 1;
		#ifdef PLATFORM_WINDOWS
		setsockopt(networksocket->socket_handle, SOL_SOCKET, SO_BROADCAST, (char *) &broadcast, sizeof(broadcast));
		#else
		//setsockopt(networksocket->socket_handle, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
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
		if ( local_if->ip.raw[0] == remote_peer->ip.raw[0] &&
				local_if->ip.raw[1] == remote_peer->ip.raw[1]) {
			return true;
			break;
		}
		local_if = local_if->next;
	}
	return false;
}

uint8_t elix_networksocket_listen_for_message(elix_networksocket * networksocket, elix_network_peer * remote_peer) {
	if ( networksocket->socket_type != TCP ) {
		return 0;
	}
	int read = 0;
	struct sockaddr_in remote = {};
	socklen_t remote_size = sizeof(remote);
	int peer_socket = 0;
	peer_socket = accept(networksocket->socket_handle, (struct sockaddr*)&remote, &remote_size);
	if ( peer_socket != INVALID_SOCKET ) {
		elix_network_ip_address_set(&remote, remote_peer);
		remote_peer->socket_handle = peer_socket;
		return 1;
	}
	return 0;
}

uint8_t elix_networksocket_receive_message(elix_networksocket * networksocket, elix_allocated_buffer * buffer, elix_network_peer * remote_peer ) {

	int read = 0;
	struct sockaddr_in remote;
	socklen_t remote_size = sizeof(remote);

	buffer->actual_size = 0;

	if ( networksocket->socket_type == TCP ) {
		// Server mode, use peer socket handle otherwise
		if ( remote_peer->socket_handle != INVALID_SOCKET ) {
			read = recv(remote_peer->socket_handle, buffer->data, buffer->data_size, 0);
		} else {
			LOG_INFO( "remote_peer socket handle is invalid");
			read = recv(networksocket->socket_handle, buffer->data, buffer->data_size, 0);
		}

		if ( read > 0 ) {
			buffer->actual_size = read;
			LOG_INFO( "Sockect Read: %d Bytes", read);
		} else if ( read < 0 ) {
			LOG_ERROR( "recv Error: %s", strerror(errno));
		}
	} else if ( networksocket->socket_type == UDP) {
		read = recvfrom(networksocket->socket_handle, NATIVE_BUFFER_TYPE(buffer->data), buffer->data_size, 0, (struct sockaddr*)&remote, NATIVE_LENGTH_TYPE(&remote_size));
		buffer->actual_size = read;
		elix_network_ip_address_set(&remote, remote_peer);

	}


	// Ignore Local Broadcast
	///bool local_broadcast = elix_networksocket_from_local_address(networksocket, remote_peer);
	//local_broadcast == ignore_local_broadcast

	if ( read > 0 ) {
		return 1;
	}
	return 0;
}


uint8_t elix_networksocket_send_message(elix_networksocket * networksocket, elix_network_peer * target, const uint8_t * message, uint64_t message_size) {
	struct sockaddr_in address = elix_network_socket_address(target);

	ssize_t results = 0;
	LOG_INFO("Sending Message to: %d.%d.%d.%d", target->ip.ip4.octel[0], target->ip.ip4.octel[1], target->ip.ip4.octel[2], target->ip.ip4.octel[3]);
	if ( networksocket->socket_type == TCP ) {
		results = send(target->socket_handle, NATIVE_BUFFER_TYPE(message), message_size, 0);
	} else if ( networksocket->socket_type == UDP) {
		results = sendto(networksocket->socket_handle, (const char*)message, message_size, 0, (struct sockaddr*)&address, sizeof (address));
	}

	if ( results == -1 ) {
		LOG_INFO( "Send Error: %s", strerror(errno) );
	}

	
	LOG_INFO("Results: %d ", results );
	return 0;
}


void elix_networktsocket_close_peer(elix_network_peer * peer) {
	close(peer->socket_handle);
}