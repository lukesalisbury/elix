

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


void programSignalHandler(int signal) {
	http_active = 0;
	LOG_MESSAGE("Quitting");
}


#include "elix_html.h"

extern "C" elix_html_document * elix_html_new( elix_string_buffer * content);

int main(int argc, char *argv[]) {

	int option_index = 0;

	while (( option_index = getopt(argc, argv, ":u")) != -1){
	    switch (option_index) {
			case 'u':
				LOG_PRINT("Port %s", optarg);
			break;
			default:
				break;
		}
	}

	elix_network_init();

	elix_network_peer remote_peer = {0};
	elix_network_peer target_peer = elix_network_address_info("google.com", "80");
	elix_allocated_buffer buffer = {{0}, ELIX_ALLOCATED_BUFFER_SIZE,0};


	elix_string_buffer html_doc = {0};

	html_doc.string = elix_string_new(8196);

	target_peer.ip.ip4.ip = 17868992;

	elix_networksocket client_socket;
	elix_networksocket_create(&client_socket, TCP, &target_peer, {false, false, false} );

	uint8_t request[73] = "GET / HTTP/1.1\r\nHost: 192.168.16.1\r\nAccept: */*\r\nConnection: close\r\n\r\n";

	if ( RESULTS_SUCCESS == elix_networksocket_send_message(&client_socket, &target_peer, request, 73) ) {
		while ( RESULTS_SUCCESS == elix_networksocket_receive_message(&client_socket, &buffer, &target_peer, false) ) {
			LOG_INFO("[Recieved] %s", buffer.data);

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
	LOG_INFO("Length: %s", responmse.length);

	LOG_INFO("------------- [Document] ------------------");
	elix_html_document * doc = elix_html_new(&html_doc);
	elix_html_parse(doc, &html_doc_status);
	elix_html_print(doc);

	LOG_INFO("-------------------------------------------------");

	elix_networksocket_destroy(&client_socket);

	elix_network_deinit();
	return 0;
}
