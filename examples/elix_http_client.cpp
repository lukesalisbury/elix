

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


static inline elix_network_peer elix_network_ip_address_raw(struct sockaddr * peer) {
	elix_network_peer output = {0};

	///TODO: Do proper fix, for endiness 
	elix_memcopy_flipped(&output.port, &peer->sa_data[0], 2);
	if ( peer->sa_family == AF_INET ) {
		elix_memcopy(&output.ip.ip4.ip, &peer->sa_data[2], 4);

	} else if ( peer->sa_family == AF_INET6 ) {
		LOG_ERROR("elix_network_ip_address_raw doesn't support IPv6 yet");
		//struct sockaddr_in6 * addr = (struct sockaddr_in6 *)peer;
		//elix_memcopy(&output.ip.ip6.word, &addr->sin6_addr, 128);
	}
	
	return output;
}

elix_network_peer elix_network_address_info( const char * domain, const char * port ) {
	elix_network_peer peer = {0};
	struct addrinfo * address = nullptr;
	int results = getaddrinfo(domain,port, nullptr, &address);

	
	if ( address ) {
		peer = elix_network_ip_address_raw(address->ai_addr);
		freeaddrinfo(address);
	}

	return peer;
}

#ifdef __cplusplus
extern "C" {
uint8_t elix_networksocket_response_message(elix_networksocket * networksocket, elix_allocated_buffer * buffer );
}
#endif
int main(int argc, char *argv[]) {
	elix_network_init();

	elix_network_peer remote_peer = {0};
	elix_network_peer target_peer = elix_network_address_info("google.com", "80");
	elix_allocated_buffer buffer = {{0}, ELIX_ALLOCATED_BUFFER_SIZE,0};

	target_peer.ip.ip4.ip = 17868992;

	elix_networksocket client_socket;
	elix_networksocket_create(&client_socket, TCP, &target_peer, false);

	uint8_t request[73] = "GET / HTTP/1.1\r\nHost: 192.168.16.1\r\nAccept: */*\r\nConnection: close\r\n\r\n";

	elix_networksocket_send_message(&client_socket, &target_peer, request, 73);
	while ( elix_networksocket_response_message(&client_socket, &buffer) ) {
		LOG_INFO("%s", buffer.data);
	}

	elix_networksocket_destroy(&client_socket);

	elix_network_deinit();
	return 0;
}
