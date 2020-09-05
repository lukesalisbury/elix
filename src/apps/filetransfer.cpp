#include "elix_core.h"

#include "elix_file.hpp"
#include "elix_networksocket.hpp"
#include <thread>

#define IPOCTALTOLONG(a,b,c,d) d | (c << 8) | (b << 16) | (a << 24)

struct elix_filetranfer_settings {
	uint16_t udp;
	uint16_t tcp;
};



class elix_network_peer_list {
public:
	elix_network_peer peers[8] = {};
	char peers_names[8][16] = {{}};
	uint8_t counter;
	uint8_t add( elix_network_peer peer) {
		for ( uint8_t c; c < 8; c++) {
			if (!peers[c].ip.raw[1]) {
				peers[c] = peer;
				return c;
			}
		}
		return 0xFF;

	}
	uint8_t remove( elix_network_peer peer) {
		for ( uint8_t c; c < 8; c++) {
			if (peers[c].ip.raw[0] == peer.ip.raw[0] && peers[c].ip.raw[1] == peer.ip.raw[1]) {
				peers[c] = { 0x00000000, 0};
				return c;
			}
		}
		return 0xFF;

	}
};



bool elix_filetransfer_init(elix_networksocket &socket) {
	return false;
}

bool elix_filetransfer_close(elix_networksocket &socket) {
	return false;
}

bool elix_filetransfer_listenudp(elix_network_peer_list & peers, elix_networksocket &socket) {
	elix_allocated_buffer buffer = {};
	elix_network_peer remote_peer = {};
	if ( socket.receive_message(buffer, remote_peer) ) {
		LOG_INFO( "Receive from %u via %d", remote_peer.ip.ip4.ip, socket.socket_type);
		LOG_INFO( "Size: %u bytes", buffer.actual_size);
		switch (buffer.data[0]) {
			case 0x01: {
				LOG_INFO("Hello MSG Broadcast");
				uint8_t broadcast_hello[] = {0x01, 'T', ' ', 'a', 't', ' '};
				socket.send_message(remote_peer, broadcast_hello, 6);
				break;
			}
			case 0x02: {
				LOG_INFO("Hello MSG Unicast");
				break;
			}
			case 0x03: {
				LOG_INFO("Good Bye MSG");
				break;
			}
			case 0x04: {
				LOG_INFO("Hello MSG with port Broadcast");
				break;
			}
			case 0x05: {
				LOG_INFO("Hello MSG  with port Unicast");
				break;
			}
		}
	}
	return false;
}

bool elix_filetransfer_listentcp(elix_network_peer_list & peers, elix_networksocket &socket) {
	elix_allocated_buffer buffer = {};
	elix_network_peer remote_peer = {};
	if ( socket.receive_message(buffer, remote_peer) ) {
		LOG_INFO("Receive from %d.%d.%d.%d", remote_peer.ip.ip4.octel[0], remote_peer.ip.ip4.octel[1], remote_peer.ip.ip4.octel[2], remote_peer.ip.ip4.octel[3] );
		LOG_INFO("Size: %d - %s", buffer.actual_size, buffer.data);
	}
	return false;
}

bool elix_filetransfer_listen(elix_network_peer_list & peers, elix_networksocket &socket) {
	if ( socket.socket_type == elix_networksocket::UDP ) {
		return elix_filetransfer_listenudp(peers, socket);
	} else if ( socket.socket_type == elix_networksocket::TCP ) {
		return elix_filetransfer_listentcp(peers, socket);
	}
	return false;
}




bool elix_filetranfer_sendviadukto(elix_network_peer & peer, uint8_t * name, uint8_t * text_message) {
	// dukto header
	// Entities
	// string (nul term) - file
	// int64 - data size
	// data
	int64_t data_size = 0;
	int64_t entities = 1;
	elix_file input_file;
	static uint8_t text_message_filename[] = {
		'_', '_', '_', 'D', 'U', 'K', 'T', 'O', '_', '_', '_', 'T', 'E', 'X', 'T', '_', '_', '_', '\0', // 19
	};

	if ( text_message == nullptr ) {
		//Send File
		elix_file_open(&input_file, (char*)name);
		data_size = input_file.length;


	} else {
		data_size = elix_ucstring_length(text_message,1);
	}

	elix_networksocket tcp_sender = elix_networksocket(elix_networksocket::TCP, peer, false);
	tcp_sender.send_message(peer, (uint8_t*) &entities, 8);
	tcp_sender.send_message(peer, (uint8_t*) &data_size, 8);

	if ( text_message ) {
		tcp_sender.send_message(peer, text_message_filename, 19);
		tcp_sender.send_message(peer, (uint8_t*) &data_size, 8);
		tcp_sender.send_message(peer, text_message, 17);
	} else {
		//Read thought file
		tcp_sender.send_message(peer, name, elix_ucstring_length(name,1));
		tcp_sender.send_message(peer, (uint8_t*) &data_size, 8);

		uint8_t buffer[512] = {};
		int64_t buffer_size = 0;
		do {
			buffer_size = elix_file_read(&input_file, buffer, 1, 512);
			tcp_sender.send_message(peer, buffer, buffer_size);
		} while (elix_file_at_end(&input_file));
	}

	tcp_sender.close_socket();




	return true;
}



int main()
{
	elix_network_init();
	LOG_MESSAGE("Testing File Tranfer");
	elix_network_peer broadcast_peer = { {.ip4={.ip=0xFFFFFFFF} }, 4644};
	elix_network_peer any_peer = { {0x00000000, 0x00000000}, 4644};

	//uint8_t broadcast_hello[] = {0x01, 'T', ' ', 'a', 't', ' '};
	uint8_t broadcast_hello[] = "\1TestBot at this address";
	uint8_t broadcast_bye[] = {0x04, 'B', 'y', 'e', ' '};

/*
	elix_network_interface * local_interfaces = elix_network_gather_ip_addresses(), * next_local_interfaces = nullptr;
	next_local_interfaces = local_interfaces;
	while ( next_local_interfaces ) {
		LOG_MESSAGE("IP: %d.%d.%d.%d", next_local_interfaces->ip.ip4.octel[0], next_local_interfaces->ip.ip4.octel[1], next_local_interfaces->ip.ip4.octel[2], next_local_interfaces->ip.ip4.octel[3]);

		next_local_interfaces = next_local_interfaces->next;
	}
	elix_network_interface_free(local_interfaces);
*/
	elix_network_peer_list peers_list;
	
	elix_networksocket udp = elix_networksocket(elix_networksocket::UDP, any_peer, true);
	elix_networksocket tcp = elix_networksocket(elix_networksocket::TCP, any_peer, true);


	elix_network_peer test_peer = { {0x00000000, 0x00000000}, 4644};
	test_peer.ip.ip4.ip = 0x5110A8C0;

	elix_filetranfer_sendviadukto(test_peer, nullptr, (uint8_t*)"Hello world sdaf");


	udp.send_message(broadcast_peer, broadcast_hello, 25);

	while (true) {
		elix_filetransfer_listen(peers_list, udp);
		elix_filetransfer_listen(peers_list, tcp);
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}

	elix_network_deinit(); 
	return 0;
}
