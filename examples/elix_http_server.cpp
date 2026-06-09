#include "elix_core.h"
#include "signal.h"
/*
size_t elix_buffer_find(char * buffer, size_t length, char * search, size_t sl , size_t offset, bool after) {
	char * debug = buffer;
	for (size_t c = offset; c < length; c++, debug++) {
		size_t sc = 0;
		for (sc = 0; sc < sl && c < length; sc++) {
			if ( buffer[c] != search[sc]) {
				break;
			}
			c++, debug++;
			if ( sc+1 == sl ) {
				return  after ? c : c - sl;
			}

		}

	}
	return SIZE_MAX;
}


const char simple_reply[94] ="HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\nError";
const char simple_error_reply[47] ="HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";


int forward_traffic(int32_t socket_proxy,  char* header, char* body, size_t header_size, size_t body_size ) {
	int32_t length = 0;
	int32_t results = 0;
	uint8_t buffer[256] = {0};
	int32_t socket_outgoing;

	printf("Sending %d bytes + %d bytes\n", header_size, body_size);

	struct sockaddr_in client_proxy;
	client_proxy.sin_family = AF_INET;
	//client_proxy.sin_addr.s_addr = inet_addr("127.0.0.1");
	client_proxy.sin_addr.s_addr = inet_addr("192.168.16.104");
	client_proxy.sin_port = htons(8080);

	socket_outgoing = socket(AF_INET , SOCK_STREAM, IPPROTO_TCP);

	connect(socket_outgoing, (struct sockaddr *)&client_proxy, sizeof(client_proxy));
	results = send(socket_outgoing, header, header_size, 0);
	if ( results != INVALID_SOCKET ) {
		results = send(socket_outgoing, "\r\n", 2, 0);
		if ( results != INVALID_SOCKET && body_size ) {
			results = send(socket_outgoing, body, body_size, 0);
		}
	}

	if ( results != INVALID_SOCKET ) {
		while ( ( length = recv(socket_outgoing, (char*)buffer, 256, 0)) > 0 ) {
			send(socket_proxy, buffer, length, 0);
		}
	} else {
		send(socket_proxy, simple_reply, 93, 0);
	}
	closesocket(socket_outgoing);
	return 1;
}



int incoming_handling(int32_t socket_incoming, struct sockaddr_in * client, int32_t * client_length ) {
	/* Buffer is only 65535 bytes, so large uploads will fail. */
	/*
	uint8_t header_list[64][512] = {{0}};
	uint8_t buffer[65535] = {0};
	uint8_t header_out[32768] = {0};
	uint8_t * body = nullptr;
	uint8_t request[512] = {0};
	//memset(buffer,0, 65535);

	int rcvd_buffer = 0;
	int rcvd = recvfrom(socket_incoming, buffer, 65535, 0, (struct sockaddr*)client, client_length);
	if ( rcvd ) {
		printf("Incoming from %s\n", inet_ntoa(client->sin_addr));
		size_t body_offset = elix_buffer_find(buffer, 65535, "\r\n\r\n", 4,0, 0);

		if ( body_offset != SIZE_MAX && body_offset < 65535) {
			rcvd_buffer = rcvd - (body_offset + 4);
			buffer[body_offset] = 0;

			if ( rcvd_buffer )
				body = buffer+body_offset+4;


			uint8_t header_index = 0;
			size_t header_line_start = 0;
			size_t header_line = elix_buffer_find(buffer, body_offset, "\r\n",2,0, 0);
			while (header_line != SIZE_MAX) {
				if ( !header_line_start ) {
					elix_cstring_copy_length(buffer + header_line_start, request, header_line - header_line_start - 1);
				} //

				elix_cstring_copy_length(buffer + header_line_start, header_list[header_index++], header_line - header_line_start - 1);

				header_line_start=header_line+2;
				header_line = elix_buffer_find(buffer, body_offset, "\r\n", 2, header_line_start, 0);
			}

			if ( request[0] ) {
				//First header should be the request
				char * method = nullptr;
				char * url = nullptr;
				char * authority = nullptr;

				size_t split_fst = elix_buffer_find(request, 512, " ", 1,0, 0);
				size_t split_snd = elix_buffer_find(request, 512, " ", 1,split_fst+1, 0);
				method = request;
				url = request + split_fst + 1;
				authority = request + split_snd + 1;

				request[split_fst] = request[split_snd] = 0;
				printf("'%s' method: '%s' url: '%s'\n",authority, method, url);

				for (int var = 1; var < 64; ++var) {
					if ( header_list[var][0] ) {
						if ( elix_buffer_find(header_list[var], 10, "Host:", 5, 0, 0) == 0 ) {
							elix_cstring_copy("Host: localhost:8080", header_list[var]);
						}
						//printf("Header: %s\n",header_list[var]);
					}
				}

				if ( elix_buffer_find(url, 16, "/AccountRight", 13, 0, 0) == 0) {
					size_t header_length = 0;
					for (int var = 0; var < 64; ++var) {
						if ( header_list[var][0] ) {
							header_length += elix_cstring_append(header_out, 32768, header_list[var], elix_cstring_length(header_list[var], 0));
							header_length += elix_cstring_append(header_out, 32768, "\r\n", 2);
						}
					}

					forward_traffic(socket_incoming, header_out, body, header_length, rcvd_buffer);
				} else {
					send(socket_incoming, simple_reply, 93, 0);
				}


			}
		}

		closesocket(socket_incoming);
	}
	return 0;
}


int main(int argc, char * argv[])
{
	for (int var = 0; var < argc; ++var) {
		printf("%d: %s\n",var, argv[var]);
	}

	int32_t sockDesc = 0;
	elix_socket_init( &sockDesc);

	struct sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	service.sin_port = htons(80);


	int  bindresult = bind(sockDesc, (struct sockaddr*)&service, sizeof (service));
	if (bindresult == 0 ) {
		int  listenresult = listen(sockDesc, 128);
		if ( listenresult == 0 )
		{
			printf("Listening\n");
			while (true) {
				fflush(nullptr);

				int32_t socket_incoming, client_length;
				struct sockaddr_in client;
				printf("Waiting for client to connect...\n");

				client_length= sizeof(client);
				socket_incoming = accept(sockDesc, (struct sockaddr*)&client, &client_length);
				if ( socket_incoming != INVALID_SOCKET ) {
					incoming_handling(socket_incoming, &client, &client_length);
				} else {
					printf("accept failed with error: %ld\n", WSAGetLastError());
					break;
				}

				//goto CLEANUP;
			}
		} else {
			printf("listen function failed with error: %ld\n", WSAGetLastError());
		}
	} else {
		printf("bind failed with error %ld\n", WSAGetLastError());
	}

	CLEANUP:
	elix_socket_close( &sockDesc);
	fflush(nullptr);
	return 0;
}

	elix_directory *  dir = elix_os_directory_list_files("./data/", nullptr);
	if ( dir ) {
		for (int a = 0; a < dir->count; ++a) {
			printf("%d: %s\n",a, dir->files[a].filename);
			elix_file file;
			elix_file_open(&file, dir->files[a].uri, EFF_FILE_READ_ONLY);

			scan_mode mode = SM_NONE;
			while (!elix_file_at_end(&file)) {
				char data[256];
				elix_file_read_line(&file, data, 256);
				elix_cstring_trim(data);
				if ( data[0] == '[') {
					if ( elix_cstring_has_prefix(data,"[defines]") ) {
						mode = SM_DEFINES;
					} else if ( elix_cstring_has_prefix(data,"[files]") ) {
						mode = SM_FILE;
					}
				} else if ( data[0] == '#') {

				} else {
					printf("%d: %s\n",mode, data);
				}

			}
			elix_file_close(&file);
		}
		elix_os_directory_list_destroy(&dir);
	}
	*/




#include "elix_cstring.h"
#include "elix_os.h"
#include "elix_networksocket.h"
#include "elix_extra.h"
#include "elix_file.h"

typedef struct remote_response_info {
	elix_networksocket * server_socket;
	elix_network_peer * remote_peer;
} remote_response_info;


const uint8_t simple_reply[87] = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\nError\r\n";
const uint8_t simple_reply_html[79] = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
const uint8_t simple_reply_json[86] = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/json; charset=UTF-8\r\n\r\n";
const uint8_t simple_error_reply[46] = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
uint32_t http_active = 1;
remote_response_info remote_response = {0};

/*
deflate: https://github.com/fxfactorial/sdefl/
gzip: https://github.com/richgel999/miniz/
br: Brotli 
zstd:
function_results elix_networksocket_send_compressed_message(elix_networksocket * networksocket, elix_network_peer * target, const uint8_t * message, uint64_t message_size) {
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
*/

void programSignalHandler(int signal) {
	http_active = 0;
	LOG_MESSAGE("Quitting");
}

function_results http_output_function( const char * string, const void * data ) {
	elix_networksocket_send_message(remote_response.server_socket, remote_response.remote_peer, (const uint8_t*) string , elix_cstring_length(string,0));
	return RESULTS_SUCCESS;
}


function_results http_handle_request(elix_http_request * request, elix_networksocket * server_socket, elix_network_peer * remote_peer) {
	if( request->uri == nullptr ) {
		return RESULTS_FAILED;
	}
	remote_response.server_socket = server_socket;
	remote_response.remote_peer = remote_peer;
	if ( elix_cstring_has_prefix(request->uri, "/list/") ) {
		elix_networksocket_send_message(server_socket, remote_peer, simple_error_reply, 46);
	} else if ( elix_cstring_has_prefix(request->uri, "/update/") ) {
		elix_networksocket_send_message(server_socket, remote_peer, simple_reply_json, 86);
	} else if ( elix_cstring_has_prefix(request->uri, "/QUIT/") ) {
		http_active = 0;
	} else if ( elix_cstring_has_prefix(request->uri, "/") ) {
		elix_file file = {0};
		if ( elix_file_open( &file, "resources/web/server.html", EFF_FILE_READ, nullptr) ) {
			elix_networksocket_send_message(server_socket, remote_peer, simple_reply_html, 78);
			while (!elix_file_at_end(&file)) {
				uint8_t data[256] = {0};

				size_t q = elix_file_read(&file, data, 1, 256);
				if (q) {
					LOG_MESSAGE("%s", data);
					elix_networksocket_send_message(server_socket, remote_peer, data, q);	
				} else {
					break;
				}
			}
			elix_file_close(&file);
		} else {
			LOG_MESSAGE("File not found");
			elix_networksocket_send_message(server_socket, remote_peer, simple_error_reply, 46);
		}
	} else {
		elix_networksocket_send_message(server_socket, remote_peer, simple_error_reply, 46);
	}
	return RESULTS_SUCCESS;
}


function_results http_server_loop(uint16_t listening_port) {

	elix_network_init();

	elix_network_interface * local_interface = elix_network_gather_ip_addresses(true);

	elix_network_peer server_peer = { ELIX_SOCKET_NOTSET, {0x00000000, 0x00000000}, listening_port};
	server_peer.ip = local_interface->ip;

	elix_networksocket server_socket;
	elix_networksocket_create(&server_socket, 1, &server_peer, {true, true, false} );
	
	LOG_PRINT("Listening %d.%d.%d.%d:%d", server_peer.ip.ip4.octel[0], server_peer.ip.ip4.octel[1], server_peer.ip.ip4.octel[2], server_peer.ip.ip4.octel[3], server_peer.port);

	elix_allocated_buffer buffer = {{0}, ELIX_ALLOCATED_BUFFER_SIZE,0};
	elix_network_peer remote_peer = {};

	LOG_PRINT("Waiting for client to connect...");
	signal(SIGINT, programSignalHandler);
	signal(SIGTERM, programSignalHandler);
	signal(SIGHUP, programSignalHandler);

	while (http_active) {
		if ( RESULTS_SUCCESS == elix_networksocket_listen_for_message(&server_socket, &remote_peer) ) {
			if ( RESULTS_SUCCESS == elix_networksocket_receive_message(&server_socket, &buffer, &remote_peer, false) > 0 ) {
				elix_http_request request = {0};
				LOG_PRINT("Receive from %d.%d.%d.%d", remote_peer.ip.ip4.octel[0], remote_peer.ip.ip4.octel[1], remote_peer.ip.ip4.octel[2], remote_peer.ip.ip4.octel[3]);

				request = elix_http_request_parse(&buffer, false, false);

				LOG_PRINT("Requesting %s", request.uri);

				http_handle_request(&request, &server_socket, &remote_peer);
				
				elix_networktsocket_close_peer(&remote_peer);
			}
			LOG_PRINT("Waiting for client to connect...");
		}
	}

	elix_networksocket_destroy(&server_socket);
	elix_network_interface_free(local_interface);
	elix_network_deinit();

	return 0;
}


int main(int argc, char *argv[]) {
	//e h s
	//2 8 19

	uint16_t port = 2819;
	int option_index = 0;

	while (( option_index = getopt(argc, argv, ":p")) != -1){
		switch (option_index) {
			case 'p':
				LOG_PRINT("Port %s", optarg);
			break;
			default:
				break;
		}
	}



	http_server_loop(port);
	return 0;
}
