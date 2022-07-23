#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

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

struct elix_filetranfer_settings {
	uint16_t udp;
	uint16_t tcp;
};

struct elix_network_interface {
	elix_ipaddress ip;
	uint16_t listening_port;
	elix_network_interface * next;
};


struct elix_filetranfer_peer {
	elix_ipaddress ip;
	uint16_t port;
	int socket_handle;
};

struct elix_filetranfer_buffer {
	uint8_t data[1024];
	uint16_t data_size = 1024;
	uint16_t actual_size = 0;
};

class elix_filetranfer_peer_list {
public:
	elix_filetranfer_peer peers[8] = {};
	char peers_names[8][16] = {{}};
	uint8_t counter = 0;
	uint8_t add( elix_filetranfer_peer peer) {
		for ( uint8_t c; c < 8; c++) {
			if (!peers[c].ip.raw[1]) {
				peers[c] = peer;
				return c;
			}
		}
		return 0xFF;

	}
	uint8_t remove( elix_filetranfer_peer peer) {
		for ( uint8_t c; c < 8; c++) {
			if (peers[c].ip.raw[0] == peer.ip.raw[0] && peers[c].ip.raw[1] == peer.ip.raw[1]) {
				peers[c] = { 0x00000000, 0, 0};
				return c;
			}
		}
		return 0xFF;

	}
};

#define IPOCTALTOLONG(a,b,c,d) d | (c << 8) | (b << 16) | (a << 24)

#include <cstring>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>

uint8_t elix_compare( const void * p1, const void * p2, size_t size ) {
	uint8_t * a = (uint8_t *)p1, * b = (uint8_t *)p2;
	for (size_t i = 0; i < size; ++i) {
		if ( a[i] != b[i] ) {
			return 0;
		}
	}
	return 1;
}

size_t elix_memcopy( const void * dest, const void * src, size_t size ) {
	uint8_t * a = (uint8_t *)dest, * b = (uint8_t *)src;
	for (size_t i = 0; i < size; ++i) {
		 a[i] = b[i];
	}
	return size;
}

elix_network_interface * elix_network_gather_ip_addresses() {
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

				if ( addr->sin_addr.s_addr == INADDR_LOOPBACK ) {
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

elix_filetranfer_peer elix_network_ip_address(sockaddr_in & peer) {
	elix_filetranfer_peer output = {};
	if ( peer.sin_family == AF_INET ) {
		output.ip.ip4.ip = peer.sin_addr.s_addr;
	} else if ( peer.sin_family == AF_INET6 ) {
		sockaddr_in6 * addr = (sockaddr_in6 *)&peer;
		elix_memcopy(&output.ip.ip6.word, &addr->sin6_addr, 128);
	}
	output.port = peer.sin_port;
	return output;
}

sockaddr_in elix_network_socket_address(elix_filetranfer_peer & peer) {
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

class elix_networksocket {
	public:
	enum {UDP, TCP};

	int invalid_handle = ~0;
	uint8_t socket_type = TCP;
	elix_network_interface * local_interfaces = nullptr;

	elix_networksocket(uint8_t type, elix_filetranfer_peer &peer, uint8_t listening){
		local_interfaces = elix_network_gather_ip_addresses();
		sockaddr_in address = elix_network_socket_address(peer);

		socket_type = type;
		if ( type == TCP ) {
			socket_handle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if ( listening ) {
				if ( ::bind(socket_handle, (sockaddr*)&address, sizeof (address)) < 0 ) {
					std::cout << "IPPROTO_TCP Bind Error " << std::strerror(errno) << std::endl;
				}
				if ( ::listen(socket_handle, 10) < 0 ) {
					std::cout << "IPPROTO_TCP Listen Error " << std::strerror(errno) << std::endl;
				}
			} else {
				if ( ::connect(socket_handle, (sockaddr*)&address, sizeof (address)) < 0 ) {
					std::cout << "IPPROTO_TCP connect Error " << std::strerror(errno) << std::endl;
				} else {
					std::cout << "Connected to ";
					std::cout << +peer.ip.ip4.octel[0] << ".";
					std::cout << +peer.ip.ip4.octel[1] << ".";
					std::cout << +peer.ip.ip4.octel[2] << ".";
					std::cout << +peer.ip.ip4.octel[3];
					std::cout << std::endl;
				}
			}

		} else if ( type == UDP) {
			socket_handle = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if ( ::bind(socket_handle, (sockaddr*)&address, sizeof (address)) < 0 ) {
				std::cout << "IPPROTO_UDP Bind Error " << std::strerror(errno) <<  std::endl;
			}
		}
		fcntl(socket_handle, F_SETFL, O_NONBLOCK);

		if ( listening ) {
			int broadcast = 1;
			setsockopt(socket_handle, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
		}

	}
	~elix_networksocket() {
		close_socket();
	}

	void close_socket() {
		//std::cout << "close_socket" <<  std::endl;
		::close(socket_handle);
	}

	bool from_local_address(elix_filetranfer_peer & remote_peer ) {
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
	uint8_t listen_for_message(elix_filetranfer_peer & remote_peer) {
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

	uint8_t receive_message(elix_filetranfer_buffer & buffer, elix_filetranfer_peer & remote_peer ) {
		int read = 0;
		sockaddr_in remote;
		socklen_t remote_size = sizeof(remote);
		buffer.actual_size = 0;

		if ( socket_type == TCP ) {

			// Server mode, use peer socket handle otherwise
			if ( remote_peer.socket_handle != INVALID_SOCKET ) {
				read = recv(remote_peer.socket_handle, buffer.data, buffer.data_size, MSG_DONTWAIT);
			} else {
				read = recv(socket_handle, buffer.data, buffer.data_size, MSG_DONTWAIT);
			}

			if (read > 0) {
				buffer.actual_size = read;
				//std::cout << read <<std::endl;
			} else if ( read < 0 ) {
				//std::cout << "TCP recvfrom Error " << std::strerror(errno) <<  std::endl;
			}
		} else if ( socket_type == UDP) {
			read = recvfrom(socket_handle, buffer.data, buffer.data_size, 0, (sockaddr*)&remote, &remote_size);
			buffer.actual_size = read;
			remote_peer = elix_network_ip_address(remote);
		}

		// Ignore Local Broadcast
		bool local_broadcast = from_local_address(remote_peer);

		if ( read > 0 && !local_broadcast) {
			return 1;

		}
		return 0;
	}


	uint8_t send_message(elix_filetranfer_peer & target, uint8_t * message, uint64_t message_size) {
		sockaddr_in address = elix_network_socket_address(target);

		int results = 0;
		if ( socket_type == TCP ) {
			results = ::send(socket_handle, message, message_size, 0);
		} else if ( socket_type == UDP) {
			results = ::sendto(socket_handle, message, message_size, 0, (sockaddr*)&address, sizeof (address));
		}

		if ( results == -1 ) {
			std::cout << "Send Error " << std::strerror(errno) << std::endl;
		}

/*
		std::cout << "Sent " << results << "b to ";
		std::cout << +target.ip.ip4.octel[0] << ".";
		std::cout << +target.ip.ip4.octel[1] << ".";
		std::cout << +target.ip.ip4.octel[2] << ".";
		std::cout << +target.ip.ip4.octel[3];
		std::cout << std::endl;
*/		
		return 0;
	}

private:
	int socket_handle;

};


bool elix_filetransfer_init(elix_networksocket &socket) {
	return false;
}

bool elix_filetransfer_close(elix_networksocket &socket) {
	return false;
}


bool elix_filetransfer_listenudp(elix_filetranfer_peer_list & peers, elix_networksocket &socket) {
	elix_filetranfer_buffer buffer = {};
	elix_filetranfer_peer remote_peer = {};
	if ( socket.receive_message(buffer, remote_peer) ) {
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
				socket.send_message(remote_peer, broadcast_hello, 31);
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

bool elix_filetranfer_recieveviadukto(elix_networksocket & socket, elix_filetranfer_peer & peer);

bool elix_filetransfer_listentcp(elix_filetranfer_peer_list & peers, elix_networksocket &socket) {

	elix_filetranfer_peer remote_peer = {};

	if ( socket.listen_for_message(remote_peer) ) {
		///TODO: push to thread
		elix_filetranfer_recieveviadukto(socket, remote_peer);
	}

	return true;
}

bool elix_filetransfer_listen(elix_filetranfer_peer_list & peers, elix_networksocket &socket) {
	if ( socket.socket_type == elix_networksocket::UDP ) {
		return elix_filetransfer_listenudp(peers, socket);
	} else if ( socket.socket_type == elix_networksocket::TCP ) {
		return elix_filetransfer_listentcp(peers, socket);
	}
	return false;
}

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

#include "elix_file.hpp"

bool elix_filetranfer_recieveviadukto(elix_networksocket & socket, elix_filetranfer_peer & peer) {
	// dukto header
	// Entities
	// total size
	// string (nul term) - file
	// int64 - data size
	// data
	static uint8_t dukto_clipboard[] = "___DUKTO___TEXT___";
	elix_filetranfer_buffer buffer = {};

	int64_t data_size = 0;
	int64_t file_size = 0;
	int64_t file_left = 0;
	int64_t entities = 1, entities_count = 0;
	uint8_t state = 0;
	bool reading_loop = 0;
	char filename[FILENAME_MAX] = {};
	while ( socket.receive_message(buffer, peer) > 0 ) {
		uint8_t * buffer_read = buffer.data;
		size_t filename_offset = 0;
		size_t buffer_offset = 0;
		size_t data_offset = 0;
		std::cout << "Receive from ";
		std::cout << +peer.ip.ip4.octel[0] << ".";
		std::cout << +peer.ip.ip4.octel[1] << ".";
		std::cout << +peer.ip.ip4.octel[2] << ".";
		std::cout << +peer.ip.ip4.octel[3] << " via " << +socket.socket_type << std::endl;
		std::cout << "Size: " << buffer.actual_size << std::endl;

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
				std::cout << "Filename: " << filename << std::endl;
				state = 3;
			break;
			case 3: //file size
				buffer_offset += elix_memcopy(&file_size, buffer_read, sizeof(int64_t));
				if ( buffer_offset < buffer.actual_size ) {
					buffer_read =  buffer.data + buffer_offset;
				}
				std::cout << "Filesize: " << file_size << std::endl;
				state = 4;
			break;
			case 4: // File data
				file_left = buffer.actual_size;

				std::cout << buffer_offset << std::endl;
				buffer_read =  buffer.data + buffer_offset;
				if ( elix_compare("___DUKTO___TEXT___", filename, filename_offset)) {
					std::cout << "Clipboard:" << buffer_read << std::endl;

					buffer_offset = buffer.actual_size;
				} else {
					std::cout << "TODO: write to file" << std::endl << buffer_read << std::endl;
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

bool elix_filetranfer_sendviadukto(elix_filetranfer_peer & peer, uint8_t * name, uint8_t * text_message) {
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
		elix_file_open(&input_file, (char*)name);
		data_size = input_file.length;
	} else {
		data_size = elix_cstring_length(text_message,1);
	}

	elix_networksocket tcp_sender = elix_networksocket(elix_networksocket::TCP, peer, false);
	tcp_sender.send_message(peer, (uint8_t*) &entities, 8);
	tcp_sender.send_message(peer, (uint8_t*) &data_size, 8);

	if ( text_message ) {
		tcp_sender.send_message(peer, text_message_filename, 19);
		tcp_sender.send_message(peer, (uint8_t*) &data_size, 8);
		tcp_sender.send_message(peer, text_message, 17);
	} else {
		uint8_t * base_name = name;
		uint8_t * s = (uint8_t*)strrchr((char*)name, '/');
		if (s) {
			base_name = (s + 1);
		}
		//Read thought file
		tcp_sender.send_message(peer, base_name, elix_cstring_length(base_name,1));
		tcp_sender.send_message(peer, (uint8_t*) &data_size, 8);

		uint8_t buffer[512] = {};
		int64_t buffer_size = 0;
		do {
			buffer_size = elix_file_read(&input_file, buffer, 1, 512);
			tcp_sender.send_message(peer, buffer, buffer_size);
		} while (!elix_file_at_end(&input_file));

	}

	tcp_sender.close_socket();




	return true;
}


#include <thread>
int tesmain()
{
	elix_filetranfer_peer broadcast_peer = { {.ip4={.ip=0xFFFFFFFF} }, 4644};
	elix_filetranfer_peer any_peer = { {0x00000000, 0x00000000}, 4644};

	//uint8_t broadcast_hello[] = {0x01, 'T', ' ', 'a', 't', ' ', 0};
	uint8_t broadcast_hello[] = "\1TestBot at address (CLI)";
	uint8_t broadcast_bye[] = {0x04, 'B', 'y', 'e', ' '};

	elix_filetranfer_peer_list peers_list;
	
	elix_networksocket udp = elix_networksocket(elix_networksocket::UDP, any_peer, true);
	elix_networksocket tcp = elix_networksocket(elix_networksocket::TCP, any_peer, true);


	elix_filetranfer_peer test_peer = { {0x00000000, 0x00000000}, 4644};
	test_peer.ip.ip4.ip = 0x5110A8C0;

	//elix_filetranfer_sendviadukto(test_peer, nullptr, (uint8_t*)"Hello world sdaf");
	elix_filetranfer_sendviadukto(test_peer, (uint8_t*)"genscript.c", nullptr);

	udp.send_message(broadcast_peer, broadcast_hello, 26);
	udp.send_message(test_peer, broadcast_hello, 26);

	while (true) {

		elix_filetransfer_listen(peers_list, udp);
		elix_filetransfer_listen(peers_list, tcp);
		std::this_thread::sleep_for(std::chrono::microseconds(100));
		//udp.send_message(broadcast_peer, broadcast_bye, 5);
	}


	return 0;
}

#include "elix_os_window.hpp"
#include "elix_os_notification.hpp"
uint8_t running = true;

int main() {
	
	elix_os_clipboard_put("Test elix_clipboard_putelix_clipboard_put 2");
	elix_window_notification note = elix_window_notification_new(
		"testing.elix",
		0,
		"dialog-information",
		"Hello world!",
		"notification.",
		"Copy",
		"Copy Text"
	);
	elix_window_notification_watch(note);
	elix_window_notification_close(note);
}

void programSignalHandler(int signal) {
	running = false;
	std::cout << "Quitting" << std::endl;
}
int main2(int argc, char *argv[])
{
	elix_filetranfer_peer broadcast_peer = { {.ip4={.ip=0xFFFFFFFF} }, 4644};
	elix_filetranfer_peer any_peer = { {0x00000000, 0x00000000}, 4644};
	
	//signal(SIGINT, programSignalHandler);
	//signal(SIGTERM, programSignalHandler);
	//ignal(SIGHUP, programSignalHandler);


	//uint8_t broadcast_hello[] = {0x01, 'T', ' ', 'a', 't', ' ', 0};
	uint8_t broadcast_hello[] = "\1TestBot at address (CLI)";
	uint8_t broadcast_bye[] = {0x04, 'B', 'y', 'e', ' '};


	elix_filetranfer_peer_list peers_list;



	elix_networksocket udp = elix_networksocket(elix_networksocket::UDP, any_peer, true);
	elix_networksocket tcp = elix_networksocket(elix_networksocket::TCP, any_peer, true);

	udp.send_message(broadcast_peer, broadcast_hello, 26);

	elix_filetranfer_peer test_peer = { {0x00000000, 0x00000000}, 4644};
	test_peer.ip.ip4.ip = 0x5110A8C0;

	int option_index = 0;

	char * filename = nullptr;
	char * message = nullptr;
	int list_connections = false;
	while (( option_index = getopt(argc, argv, ":f:m:i:l")) != -1){
	    switch (option_index) {
			case 'f':
				filename = optarg;
				std::cout << "Sending File: " << optarg << std::endl;
			break;
			case 'm':
				message = optarg;
				std::cout << "Sending Message" << std::endl;
			break;
		
			case 'i':
				std::cout << "To IP Address: " << optarg << std::endl;
				test_peer.ip.ip4.ip = inet_addr(optarg);
			break;

			case 'l':
				list_connections = true;
				
			break;

			default:
				std::cout << "No options given, running in listening mode" << std::endl;

		
		}
	}
	if ( message ) {
		elix_filetranfer_sendviadukto(test_peer, nullptr, (uint8_t*)message);
	} else if ( filename ) {
		elix_filetranfer_sendviadukto(test_peer, (uint8_t*)filename, nullptr);
	} else if ( list_connections ) {
		uint8_t trycount = 255;
		std::cout << "Looking" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		while ( trycount-- ) {
			elix_filetransfer_listen(peers_list, udp);
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
		std::cout << std::endl;
		for (uint8_t i = 0; i < peers_list.counter; i++) {
			std::cout << peers_list.peers_names[i] <<std::endl;
		}	
	} else {

		while ( running ) {
			elix_filetransfer_listen(peers_list, udp);
			elix_filetransfer_listen(peers_list, tcp);
			std::this_thread::sleep_for(std::chrono::microseconds(33));

		}
		
	}
	return 0;
}