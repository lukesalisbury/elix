/*
 Simple local network file transfer, compatible with Dukto https://sourceforge.net/projects/dukto/

*/

#include <csignal>
#include <cstdio>
#include <thread>

#include "elix_networksocket.h"
#include "window/elix_os_window.hpp"
#include "window/elix_os_notification.hpp"

#include "elix_os.h"
#include "elix_file.h"

elix_window_notification notify_handler = {};

void programSignalHandler(int signal) {
	notify_handler.exiting = true;
	LOG_MESSAGE("Quitting");
}

void write_clipboard( elix_window_notification_message * notification ) {
	char * text = (char*)notification->action_data;
	LOG_MESSAGE("write_clipboard: %s", text);
	elix_os_clipboard_put(text);
	LOG_MESSAGE("on_clipboard: %s", elix_os_clipboard_get());

	if ( notification->action_data ) {
		elix_file * file = elix_file_new("~/clipboard.txt");
		elix_file_write_string(file, text, 0);
		elix_file_close(file);
	}
}

void rename_tempoutput( elix_window_notification_message * notification ) {
	LOG_MESSAGE("rename_tempoutput:");
};

elix_window_notification_settings notify_incomingfile;


inline size_t elix_cstring_length(const uint8_t * string, uint8_t include_terminator = 0 ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}

class elix_filetranfer_peer_list {
public:
	elix_network_peer peers[8] = {};
	char peers_names[8][16] = {{}};
	uint8_t counter = 0;
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
				peers[c] = { 0x00000000, 0, 0};
				return c;
			}
		}
		return 0xFF;

	}
};


bool elix_filetransfer_init(elix_networksocket &socket) {
	elix_network_init();
	return false;
}

bool elix_filetransfer_close(elix_networksocket &socket) {
	elix_network_deinit();
	return false;
}


bool elix_filetransfer_listenudp(elix_filetranfer_peer_list & peers, elix_networksocket &socket) {
	elix_allocated_buffer buffer;
	elix_network_peer remote_peer = {};
	if ( elix_networksocket_receive_message(&socket, &buffer, &remote_peer) ) {
		/*
		std::cout << "Receive from ";
		std::cout << +remote_peer.ip.ip4.octel[0] << ".";
		std::cout << +remote_peer.ip.ip4.octel[1] << ".";
		std::cout << +remote_peer.ip.ip4.octel[2] << ".";
		std::cout << +remote_peer.ip.ip4.octel[3] << " via " << socket.socket_type << std::endl;
		std::cout << "Size: " << buffer.actual_size << " - " << buffer.data << std::endl;
		*/
		switch (buffer.data[0]) {
			case 0x01: {
				//std::cout << "Hello MSG Broadcast" << std::endl;
				uint8_t broadcast_hello[] = "\2TestBot at this address (CLI)";
				elix_networksocket_send_message(&socket, &remote_peer, broadcast_hello, 31);
				break;
			}
			case 0x02: {
				//std::cout << "Hello MSG Unicast" << std::endl;
				peers.add(remote_peer);
				break;
			}
			case 0x03: {
				//std::cout << "Good Bye MSG" << std::endl;
				peers.remove(remote_peer);
				break;
			}
			case 0x04: {
				//std::cout << "Hello MSG with port Broadcast" << std::endl;
				break;
			}
			case 0x05: {
				//std::cout << "Hello MSG  with port Unicast" << std::endl;
				break;
			}
		}
		return true;
	}
	return false;
}

bool elix_filetransfer_recieveviadukto(elix_networksocket & socket, elix_network_peer & peer) {
	// dukto header
	// Entities
	// total size
	// string (nul term) - file
	// int64 - data size
	// data
	static uint8_t dukto_clipboard[] = "___DUKTO___TEXT___";
	elix_allocated_buffer buffer;

	int64_t data_size = 0;
	int64_t file_size = 0;
	int64_t file_left = 0;
	int64_t entities = 1, entities_count = 0;
	uint8_t state = 0;
	bool reading_loop = 0;
	char filename[FILENAME_MAX] = {};
	while ( elix_networksocket_receive_message(&socket, &buffer, &peer) > 0 ) {
		uint8_t * buffer_read = buffer.data;
		size_t filename_offset = 0;
		size_t buffer_offset = 0;
		size_t data_offset = 0;

		while (buffer_offset < buffer.actual_size ) {
			switch (state) {
				case 0: //Entity Count
					buffer_offset += elix_memcopy(&entities, buffer_read, sizeof(int64_t));
					if ( buffer_offset < buffer.actual_size ) {
						buffer_read =  buffer.data + buffer_offset;
					}
					state = 1;
				break;
			case 1: //Entity Count
				buffer_offset += elix_memcopy(&data_size, buffer_read, sizeof(int64_t));
				if ( buffer_offset < buffer.actual_size ) {
					buffer_read =  buffer.data + buffer_offset;
				}
				state = 2;
			break;
			case 2: //Filename
				filename_offset = 0;

				while( (filename_offset < FILENAME_MAX - 1) && *buffer_read != 0 ) {
						filename[filename_offset++] = *buffer_read;
						++buffer_offset;
						++buffer_read;

				}
				++buffer_offset;
				++buffer_read;
				filename[filename_offset] = 0;

				state = 3;
			break;
			case 3: //file size
				buffer_offset += elix_memcopy(&file_size, buffer_read, sizeof(int64_t));
				if ( buffer_offset < buffer.actual_size ) {
					buffer_read =  buffer.data + buffer_offset;
				}

				state = 4;
			break;
			case 4: // File data
				file_left = buffer.actual_size;

				buffer_read = buffer.data + buffer_offset;
				if ( elix_compare("___DUKTO___TEXT___", filename, filename_offset)) {
					elix_window_notification_settings setting = elix_window_notification_settings_create("Incoming Text", "Copy", " Copy Text to Clipboard", (char*)buffer_read);
					elix_window_notification_message * message = elix_window_notification_add(notify_handler, setting, &write_clipboard, buffer_read);
					buffer_offset = buffer.actual_size;
				} else {
					elix_window_notification_message * message = elix_window_notification_add(notify_handler, notify_incomingfile, nullptr, nullptr);
					buffer_offset = buffer.actual_size;
					if ( file_size )
						entities_count++;
					if ( entities_count < entities )
						state = 2;
					else
						state = 5;
				}

				break;
			}
		}


	}
	return true;
}

bool elix_filetransfer_listentcp(elix_filetranfer_peer_list & peers, elix_networksocket &socket) {

	elix_network_peer remote_peer = {};

	if (elix_networksocket_listen_for_message(&socket, &remote_peer) ) {
		///TODO: push to thread
		elix_filetransfer_recieveviadukto(socket, remote_peer);
	}

	return true;
}

bool elix_filetransfer_listen(elix_filetranfer_peer_list & peers, elix_networksocket &socket) {
	if ( socket.socket_type == UDP ) {
		return elix_filetransfer_listenudp(peers, socket);
	} else if ( socket.socket_type == TCP ) {
		return elix_filetransfer_listentcp(peers, socket);
	}
	return false;
}

bool elix_filetransfer_sendviadukto(elix_network_peer & peer, uint8_t * name, uint8_t * text_message) {
	// dukto header
	// Entities
	// total size
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
		elix_file_open(&input_file, (char*)name, EFF_FILE_READ, 0);
		data_size = input_file.length;
	} else {
		data_size = elix_cstring_length(text_message,1);
	}

	elix_networksocket tcp_sender;
	elix_networksocket_create(&tcp_sender, TCP, &peer, false);
	elix_networksocket_send_message(&tcp_sender, &peer, (uint8_t*) &entities, 8);
	elix_networksocket_send_message(&tcp_sender, &peer, (uint8_t*) &data_size, 8);

	if ( text_message ) {
		elix_networksocket_send_message(&tcp_sender, &peer, text_message_filename, 19);
		elix_networksocket_send_message(&tcp_sender, &peer, (uint8_t*) &data_size, 8);
		elix_networksocket_send_message(&tcp_sender, &peer, text_message, 17);
	} else {
		uint8_t * base_name = name;
		uint8_t * s = (uint8_t*)strrchr((char*)name, '/');
		if (s) {
			base_name = (s + 1);
		}
		//Read thought file
		elix_networksocket_send_message(&tcp_sender, &peer, base_name, elix_cstring_length(base_name,1));
		elix_networksocket_send_message(&tcp_sender, &peer, (uint8_t*) &data_size, 8);

		uint8_t buffer[512] = {};
		int64_t buffer_size = 0;
		do {
			buffer_size = elix_file_read(&input_file, buffer, 1, 512);
			elix_networksocket_send_message(&tcp_sender, &peer, buffer, buffer_size);
		} while (!elix_file_at_end(&input_file));

	}

	elix_networksocket_destroy(&tcp_sender);

	return true;
}

int test_main()
{
	elix_networksocket udp, tcp;
	
	elix_filetranfer_peer_list peers_list;
	
	elix_network_peer broadcast_peer = { {.ip4={.ip=0xFFFFFFFF} }, 4644};
	elix_network_peer any_peer = { {0x00000000, 0x00000000}, 4644};
	elix_network_peer test_peer = { {0x00000000, 0x00000000}, 4644};
	test_peer.ip.ip4.ip = 0x5110A8C0;

	//uint8_t broadcast_hello[] = {0x01, 'T', ' ', 'a', 't', ' ', 0};
	uint8_t broadcast_hello[] = "\1TestBot at address (CLI)";
	uint8_t broadcast_bye[] = {0x04, 'B', 'y', 'e', ' '};

	elix_networksocket_create(&udp, UDP, &any_peer, true);
	elix_networksocket_create(&tcp, TCP, &any_peer, true);
	
	//elix_filetransfer_sendviadukto(test_peer, nullptr, (uint8_t*)"Hello world sdaf");
	elix_filetransfer_sendviadukto(test_peer, (uint8_t*)"genscript.c", nullptr);

	elix_networksocket_send_message(&udp, &broadcast_peer, broadcast_hello, 26);
	elix_networksocket_send_message(&udp, &test_peer, broadcast_hello, 26);

	while (true) {

		elix_filetransfer_listen(peers_list, udp);
		elix_filetransfer_listen(peers_list, tcp);
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		//elix_networksocket_send_message(udp, broadcast_peer, broadcast_bye, 5);
	}

	return 0;
}

char blank_string[256] = {'\0'};

int main(int argc, char *argv[]) {
	char * arg0 = argv[0];

	notify_incomingfile = elix_window_notification_settings_create("Incoming File", blank_string, "", "");

	elix_program_info info = elix_program_info_create(arg0, "File Transfer", "0", "0");
	elix_network_interface * local_interface = elix_network_gather_ip_addresses(true);

	elix_network_peer broadcast_peer = { {.ip4={.ip=0xFFFFFFFF} }, 4644};
	elix_network_peer any_peer = { {0x00000000, 0x00000000}, 4644};
	elix_network_peer test_peer = { {0x00000000, 0x00000000}, 4644};
	test_peer.ip.ip4.ip = 0x5110A8C0;


	//uint8_t broadcast_hello[] = {0x01, 'T', ' ', 'a', 't', ' ', 0};
	uint8_t broadcast_bye[] = {0x04, 'B', 'y', 'e', ' '};
	uint8_t broadcast_hello[64] = "\1TestBot at address (CLI)";
	uint8_t broadcast_hello_size = 26;

	if ( local_interface ) {
		broadcast_hello_size = snprintf((char*)broadcast_hello, 64, "\1%s at %d.%d.%d.%d", info.user, local_interface->ip.ip4.octel[0], local_interface->ip.ip4.octel[1], local_interface->ip.ip4.octel[2], local_interface->ip.ip4.octel[3] );
	}

	elix_filetranfer_peer_list peers_list;

	elix_networksocket udp, tcp;

	elix_networksocket_create(&udp, UDP, &any_peer, true);
	elix_networksocket_create(&tcp, TCP, &any_peer, true);

	elix_networksocket_send_message(&udp, &broadcast_peer, broadcast_hello, broadcast_hello_size);

	int option_index = 0;

	char * filename = nullptr;
	char * message = nullptr;
	int list_connections = false;
	while (( option_index = getopt(argc, argv, ":f:m:i:l")) != -1){
	    switch (option_index) {
			case 'f':
				filename = optarg;
				LOG_MESSAGE("Sending File: %s", optarg);
			break;
			case 'm':
				message = optarg;
				LOG_MESSAGE("Sending Message");
			break;
		
			case 'i':
				LOG_MESSAGE("Sending File: %s", optarg);
				test_peer.ip.ip4.ip = inet_addr(optarg);
			break;

			case 'l':
				list_connections = true;
			break;

			default:
				LOG_MESSAGE("No options given, running in listening mode");
		}
	}


	if ( message ) {
		elix_filetransfer_sendviadukto(test_peer, nullptr, (uint8_t*)message);
	} else if ( filename ) {
		elix_filetransfer_sendviadukto(test_peer, (uint8_t*)filename, nullptr);
	} else if ( list_connections ) {
		uint8_t trycount = 255;
		LOG_MESSAGE("Looking");
		std::this_thread::sleep_for(std::chrono::seconds(1));
		while ( trycount-- ) {
			elix_filetransfer_listen(peers_list, udp);
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

		for (uint8_t i = 0; i < peers_list.counter; i++) {
			LOG_MESSAGE("[%d] %s", i, peers_list.peers_names[i]);
		}	
	} else {

		notify_handler = elix_window_notification_new(
			"filetransfer.elix"
		);
		signal(SIGINT, programSignalHandler);
		signal(SIGTERM, programSignalHandler);
		signal(SIGHUP, programSignalHandler);

		while ( !notify_handler.exiting ) {
			elix_window_notification_tick(notify_handler);
			elix_filetransfer_listen(peers_list, udp);
			elix_filetransfer_listen(peers_list, tcp);
			elix_networksocket_send_message(&udp, &broadcast_peer, broadcast_hello, broadcast_hello_size);

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		
		elix_window_notification_close(notify_handler);
	}
	return 0;
}