

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

const uint8_t simple_request_get[] = "GET / HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\nConnection: close\r\n\r\n";

uint32_t http_active = 1;
remote_response_info remote_response = {0};

/*
User-Agent: 
Accept-Language:
Referer:
Cookie:
*/




void programSignalHandler(int signal) {
	http_active = 0;
	LOG_MESSAGE("Quitting");
}


#include "elix_html.h"
#include <rustls.h>

typedef struct elix_networksocket_rustls {
	elix_networksocket socket;
	struct rustls_connection * connection;
	elix_network_peer remote_peer;

} elix_networksocket_rustls;

function_results elix_networksocket_send_message2(elix_networksocket * networksocket, elix_network_peer * target, const uint8_t * message, uint64_t message_size, uint64_t * byte_sent);
function_results elix_networksocket_receive_message2(elix_networksocket * networksocket, elix_network_peer * remote_peer, uint8_t * message, uint64_t message_size, uint64_t * byte_read );

void elix_networksocket_rustls_create(elix_networksocket_rustls * networksocket, uint8_t type, elix_network_peer * peer, elix_networksocket_options option) {

	uint16_t default_tls_versions[] = { 0x0303, 0x0304 };
    rustls_result result;

	const struct rustls_client_config * config = nullptr;
	rustls_client_config_builder * config_builder = nullptr;
	rustls_root_cert_store * cert_store = nullptr;
	rustls_server_cert_verifier * server_cert_verifier = nullptr;

	config_builder = rustls_client_config_builder_new();

	result = rustls_platform_server_cert_verifier(&server_cert_verifier);
    if(RUSTLS_RESULT_OK != result) {
		LOG_ERROR("failed to construct platform verifier");
		return;
    }
    rustls_client_config_builder_set_server_verifier(config_builder, server_cert_verifier);

	rustls_slice_bytes alpn_http11 = { .data = (unsigned char *)"http/1.1", .len = 8 };
	result = rustls_client_config_builder_set_alpn_protocols(config_builder, &alpn_http11, 1);
	if(RUSTLS_RESULT_OK != result) {
		LOG_ERROR("setting ALPN");
		return;
    }


 	result = rustls_client_config_builder_build(config_builder, &config);
	config_builder = nullptr;
	if(RUSTLS_RESULT_OK != result) {
    	LOG_ERROR("building client config builder");
		return;
	}
	
	result = rustls_client_connection_new(config, "www.google.com", &networksocket->connection);

	if (result != RUSTLS_RESULT_OK) {
		LOG_ERROR("Connection failed");
	}

	elix_networksocket_create(&networksocket->socket, type, peer, option);

	networksocket->remote_peer = *peer;
}

int read_tls_callback(void *userdata, unsigned char *buf, const size_t len, size_t *out_n) {
	elix_networksocket_rustls * socket_rustls = (struct elix_networksocket_rustls *)userdata;
	return !(RESULTS_SUCCESS == elix_networksocket_receive_message2(&socket_rustls->socket, &socket_rustls->remote_peer, buf, len, out_n));
}

int write_tls_callback(void *userdata, const unsigned char *buf, const size_t len, size_t *out_n) {
	elix_networksocket_rustls * socket_rustls = (struct elix_networksocket_rustls *)userdata;
	return !(RESULTS_SUCCESS == elix_networksocket_send_message2(&socket_rustls->socket, &socket_rustls->remote_peer, buf, len, out_n));
}

function_results elix_networksocket_rustls_receive_message(elix_networksocket_rustls * networksocket, elix_network_peer * target_peer, uint8_t * message, uint64_t message_size, uint64_t * size ){
   
	networksocket->remote_peer = *target_peer;
    
    rustls_io_result result = rustls_connection_read_tls(networksocket->connection, read_tls_callback, networksocket, size);
    if (result != 0) {
        return RESULTS_ERROR;
    }

	result = rustls_connection_process_new_packets(networksocket->connection);
	if( RUSTLS_RESULT_OK != result ) {
		LOG_ERROR("error in rustls_connection_process_new_packets");
    }

	result = rustls_connection_read(networksocket->connection, message, message_size, size);
	if( RUSTLS_RESULT_PLAINTEXT_EMPTY == result ) {
		return RESULTS_SUCCESS;
    }

	return RESULTS_ERROR;
}

function_results elix_networksocket_rustls_send_message(elix_networksocket_rustls * networksocket, elix_network_peer * target_peer, const uint8_t * message, uint64_t message_size, uint64_t * size ) {
    
	networksocket->remote_peer = *target_peer;

	rustls_result rr = rustls_connection_write(networksocket->connection, message, message_size, size);
	if (rr != 0) {
		return RESULTS_ERROR;
	}

    rustls_io_result result = rustls_connection_write_tls(networksocket->connection, write_tls_callback, networksocket, size);
	if (result != 0) {
		return RESULTS_ERROR;
	}

    return RESULTS_SUCCESS;
}

elix_html_document * elix_html_new( elix_string_buffer * content);

void http_request() {

	struct elix_networksocket_options socket_settings =  {false, false, false};
	elix_network_peer remote_peer = {0};
	elix_network_peer target_peer = elix_network_address_info("google.com", "80");
	elix_allocated_buffer buffer = {{0}, ELIX_ALLOCATED_BUFFER_SIZE,0};

	elix_string_buffer html_doc = {0};

	html_doc.string = elix_string_new(8196);

	
	elix_networksocket client_socket;
	elix_networksocket_create(&client_socket, TCP, &target_peer, socket_settings );

	uint8_t request[73] = "GET / HTTP/1.1\r\nHost: google.com\r\nAccept: */*\r\nConnection: close\r\n\r\n";

	if ( RESULTS_SUCCESS == elix_networksocket_send_message(&client_socket, &target_peer, request, 73) ) {
		while ( RESULTS_SUCCESS == elix_networksocket_receive_message(&client_socket, &buffer, &target_peer, false) ) {
			//LOG_INFO("[Recieved] %s", buffer.data);

			elix_string_append_data(&html_doc.string, buffer.data, buffer.actual_size);

		}
	}

	LOG_INFO("-------------------------------------------------");
	LOG_INFO("%.*s", html_doc.string.length, html_doc.string.data);
	LOG_INFO("-------------- [Header] -------------------");
	elix_parse_status html_doc_status = {0, SIZE_MAX}; //
	elix_http_response responmse = elix_http_response_parse(&html_doc.string, &html_doc_status.offset );
	LOG_INFO("Date: %s", responmse.date);
	LOG_INFO("Last-Modified: %s", responmse.modified);
	LOG_INFO("length: %s", responmse.length);



	LOG_INFO("------------- [Document] ------------------");
	elix_html_document * doc = elix_html_new(&html_doc);
	elix_html_parse(doc, &html_doc_status);
	elix_html_print(doc);

	LOG_INFO("-------------------------------------------------");
	elix_networksocket_destroy(&client_socket);


}



int main(int argc, char *argv[]) {

	int option_index = 0;

	while (( option_index = getopt(argc, argv, ":u")) != -1){
	    switch (option_index) {
			case 'u':
				
			break;
			default:
				break;
		}
	}


	elix_network_init();

	http_request();

	elix_network_peer remote_peer = {0};
	elix_network_peer target_peer = elix_network_address_info("google.com", "443");
	elix_allocated_buffer buffer = {{0}, ELIX_ALLOCATED_BUFFER_SIZE,0};

	uint8_t message[8196] = {0};
	uint64_t message_size = 8196;
	uint64_t size = 0;

	elix_string_buffer html_doc = {0};

	html_doc.string = elix_string_new(8196);

	//target_peer.ip.ip4.ip = 4212172992;


	LOG_INFO("Connecting to %d.%d.%d.%d:%d", target_peer.ip.ip4.octel[0], target_peer.ip.ip4.octel[1], target_peer.ip.ip4.octel[2], target_peer.ip.ip4.octel[3], target_peer.port);
	LOG_INFO("-------------------------------------------------");

	elix_networksocket_rustls client_socket = {0};
    struct elix_networksocket_options socket_settings =  {false, false, false};
	elix_networksocket_rustls_create(&client_socket, TCP, &target_peer, socket_settings );

	uint8_t request[] = "GET / HTTP/1.1\r\nHost: google.com\r\nAccept: */*\r\nConnection: close\r\n\r\n";

	if ( RESULTS_SUCCESS == elix_networksocket_rustls_send_message(&client_socket, &target_peer, request, 73, &size) ) {
		while ( RESULTS_SUCCESS == elix_networksocket_rustls_receive_message(&client_socket, &target_peer, message, message_size, &size) ) {
			LOG_INFO("[Recieved %d] %.*s ", size, size, message);

			elix_string_append_data(&html_doc.string, message, size);

		}
	}

	LOG_INFO("-------------------------------------------------");
	LOG_INFO("%.*s", html_doc.string.length, html_doc.string.data);
	LOG_INFO("-------------------------------------------------");

	//elix_networksocket_rustls_destroy(&client_socket);

	elix_network_deinit();
	return 0;
}





