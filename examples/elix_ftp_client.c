#include "elix_cstring.h"
#include "elix_os.h"
#include "elix_networksocket.h"
#include "elix_extra.h"
#include "elix_file.h"

typedef struct remote_response_info {
	elix_networksocket * server_socket;
	elix_network_peer * remote_peer;
} remote_response_info;

remote_response_info remote_response = {0};


void programSignalHandler(int signal) {
	LOG_MESSAGE("Quitting");
}

int8_t stringarray_count( uint8_t * string, size_t string_length ) {
	int8_t count = 0;
	size_t index = 0;
	uint8_t prev = 0;

	while ( index < string_length ) {
		if ( string[index] == 0 && prev != 0) {
			count++;
		}
		prev = string[index++];
	}
	return count;
}

uint8_t * stringarray_value( uint8_t * string, size_t string_length, int8_t index ) {
	int8_t count = 0;
	size_t i = 0;
	uint8_t * ptr = 0;
	if ( index == 0 )
		return string;

	while ( i < string_length ) {
		if ( count == index ) {
			return ptr;
		}
		if ( string[i] == 0) {
			count++;
			ptr = string + i + 1;
		}
		i++;

	}
	return string;
}

uint16_t str_to_uint16( uint8_t * string ) {
	uint16_t result = 0;
	uint8_t c, v;
	while ( (c = *string++)) {
		if ( ('0' <= c && c <= '9') ) {
			result = result * 10 + (c - '0');
		}
	}
}

uint16_t stringarray_uint16( uint8_t * string, size_t string_length, int8_t index ) {
	int8_t count = 0;
	size_t i = 0;
	uint8_t * ptr = string;

	if ( index > 0 ) {
		while ( i < string_length ) {
			if ( count == index ) {
				i == string_length;
				break;
			} else {
				if ( string[i] == 0) {
					count++;
					ptr = string + i + 1;
				}
				i++;
			}
		}
	}

	return str_to_uint16(ptr);
}


uint8_t command_login_user[512] = "USER anonymous\r\n";
uint8_t command_login_pass[512] = "PASS blank@EXAMPLE.COM\r\n";


typedef struct elix_ftp_client {
	elix_networksocket socket_command;
	elix_networksocket socket_data;
	elix_network_peer peer_command;
	elix_network_peer peer_data;
} elix_ftp_client;

uint64_t elix_ftp_send_command(elix_ftp_client * client, elix_allocated_buffer * buffer, const uint8_t * cmd, uint64_t cmd_size) {
	uint64_t read = 0;
	elix_networksocket_send_message(&client->socket_command, &client->peer_command, cmd, cmd_size, nullptr) ;
	elix_networksocket_receive_message(&client->socket_command, &client->peer_command, buffer->data, buffer->data_size, &read, false);

	return read;
}

function_results elix_ftp_connect( elix_ftp_client * client, elix_allocated_buffer * buffer, const char * user, const char * pass) {
	elix_networksocket_options socket_client_option = { .listening = false, .public_only = false, .non_blocking = false};

	elix_networksocket_create(&client->socket_command, TCP, &client->peer_command, socket_client_option);

	elix_networksocket_receive_message(&client->socket_command, &client->peer_command, buffer->data, buffer->data_size, nullptr, false);
	LOG_INFO("Recieved['%s']", buffer->data);

	elix_networksocket_send_message(&client->socket_command, &client->peer_command, command_login_user, elix_cstring_length(command_login_user, 0), nullptr) ;
	elix_networksocket_receive_message(&client->socket_command, &client->peer_command, buffer->data, buffer->data_size, nullptr, false);
	LOG_INFO("Recieved['%s']", buffer->data);

	elix_networksocket_send_message(&client->socket_command, &client->peer_command, command_login_pass, elix_cstring_length(command_login_pass, 0), nullptr) ;
	elix_networksocket_receive_message(&client->socket_command, &client->peer_command, buffer->data, buffer->data_size, nullptr, false);
	LOG_INFO("Recieved['%s']", buffer->data);

	if ( elix_cstring_has_prefix(buffer->data, "230 ") ) {
		return RESULTS_SUCCESS;
	} else {
		return RESULTS_FAILED;
	}
}

function_results elix_ftp_passive_mode_open( elix_ftp_client * client, elix_allocated_buffer * buffer) {
	uint64_t read = 0;
	elix_networksocket_send_message(&client->socket_command, &client->peer_command, "PASV\r\n", 6, nullptr) ;
	elix_networksocket_receive_message(&client->socket_command, &client->peer_command, buffer->data, buffer->data_size, &read, false);
	buffer->actual_size = read;
	LOG_INFO("Recieved['%s']", buffer->data);

	if ( elix_cstring_has_prefix(buffer->data, "227 Entering Passive Mode (") ) {
		//227 	Entering Passive Mode (h1,h2,h3,h4,p1,p2)
		// sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) == 6
		uint8_t * array = buffer->data + 27;
		for (size_t i = 27; i < buffer->actual_size; i++) {
			if ( buffer->data[i] < '0' || buffer->data[i] > '9' ) {
				buffer->data[i] = 0;
			}
		}
		client->peer_data.ip.ip4.octel[0] = stringarray_uint16(array, buffer->actual_size - 27, 0);
		client->peer_data.ip.ip4.octel[1] = stringarray_uint16(array, buffer->actual_size - 27, 1);
		client->peer_data.ip.ip4.octel[2] = stringarray_uint16(array, buffer->actual_size - 27, 2);
		client->peer_data.ip.ip4.octel[3] = stringarray_uint16(array, buffer->actual_size - 27, 3);

		uint16_t p1 = stringarray_uint16(array, buffer->actual_size - 27, 4);
		uint16_t p2 = stringarray_uint16(array, buffer->actual_size - 27, 5);

		client->peer_data.port = (p1 << 8) | p2;

	} else if ( elix_cstring_has_prefix(buffer->data, "228 ") ) {
		//228 	Entering Long Passive Mode (long address, port).
		//client->peer_data.ip.ip6 = 
		//client->peer_data.port = 
	} else if ( elix_cstring_has_prefix(buffer->data, "229 ") ) {
		//229 	Entering Extended Passive Mode (|||port|).
		//client->peer_data.port = 
	} else {
		return RESULTS_FAILED;
	}


	elix_networksocket_create(&client->socket_data, TCP, &client->peer_data, (elix_networksocket_options) { .listening = false, .public_only = false, .non_blocking = false});


	return RESULTS_SUCCESS;
}


int main(int argc, char *argv[]) {

	elix_ftp_client client = {
		.peer_command = { .socket_handle = -1, .ip= {.ip4.octel = {192,168,16,1}}, .port = 21 },
		.peer_data = { .socket_handle = -1, .ip= {.ip4.octel = {192,168,16,1}}, .port = 22 }
	};
	elix_allocated_buffer buffer = {{0}, ELIX_ALLOCATED_BUFFER_SIZE, 0};

	elix_network_init();

	if ( RESULTS_SUCCESS == elix_ftp_connect(&client, &buffer, "anonymous", "blank@EXAMPLE.COM") ) {
		LOG_INFO("FTP Login Success");

		elix_ftp_send_command(&client, &buffer, "TYPE I\r\n", 8);
		LOG_INFO("Recieved['%s']", buffer.data);

		elix_ftp_send_command(&client, &buffer, "CWD Media\r\n", 11);
		LOG_INFO("Recieved['%s']", buffer.data);

		
		if ( RESULTS_SUCCESS == elix_ftp_passive_mode_open(&client, &buffer) ) {
			elix_ftp_send_command(&client, &buffer, "LIST\r\n", 6);
			LOG_INFO("Recieved['%s']", buffer.data);
			if ( elix_cstring_has_prefix(buffer.data, "150 ") ) {
				LOG_INFO("--- Directory Listing ---");
				uint64_t read = 0;
				while( RESULTS_SUCCESS == elix_networksocket_receive_message(&client.socket_data, &client.peer_data, buffer.data, buffer.data_size, &read, false) ) {
					LOG_INFO("%.*s", read, buffer.data);
				}
				LOG_INFO("--- End of Listing ---");

				elix_networksocket_destroy(&client.socket_data);

				elix_networksocket_receive_message(&client.socket_command, &client.peer_command, buffer.data, buffer.data_size, nullptr, false);
				LOG_INFO("Recieved['%s']", buffer.data);

				if ( elix_cstring_has_prefix(buffer.data, "226 Transfer complete") ) {
					
				}
			}
		}
		elix_ftp_send_command(&client, &buffer, "SIZE sdlgamecontrollerdb.txt\r\n", 36);
		LOG_INFO("Recieved['%s']", buffer.data);
		if ( elix_cstring_has_prefix(buffer.data, "213 ") ) {
			LOG_INFO("File Size: '%s' %d", buffer.data + 4, str_to_uint16(buffer.data + 4));
		}

		if ( RESULTS_SUCCESS == elix_ftp_passive_mode_open(&client, &buffer) ) {
			

			elix_ftp_send_command(&client, &buffer, "RETR sdlgamecontrollerdb.txt\r\n", 36);
			LOG_INFO("Recieved['%s']", buffer.data);
			if ( elix_cstring_has_prefix(buffer.data, "150 ") ) {
				LOG_INFO("--- File content ---");
				uint64_t read = 0;
				while( RESULTS_SUCCESS == elix_networksocket_receive_message(&client.socket_data, &client.peer_data, buffer.data, buffer.data_size, &read, false) ) {
					LOG_INFO("%.*s", read, buffer.data);
				}
				LOG_INFO("--- File content ---");
				
				elix_networksocket_destroy(&client.socket_data);

				elix_networksocket_receive_message(&client.socket_command, &client.peer_command, buffer.data, buffer.data_size, nullptr, false);
				LOG_INFO("Recieved['%s']", buffer.data);
				if ( elix_cstring_has_prefix(buffer.data, "226 Transfer complete") ) {
					
				}
			}

		}
		elix_networksocket_send_message(&client.socket_command, &client.peer_command, "QUIT\r\n", 6, nullptr) ;

	} else {
		LOG_INFO("FTP Login Failed");
	}







	elix_networksocket_destroy(&client.socket_command);

	elix_network_deinit();
	return 0;
}
