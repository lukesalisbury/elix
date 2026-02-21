#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef nullptr
	#define nullptr NULL
#endif
#define LOG_MESSAGE(M, ...) printf("%18s:%04d | " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define FH(q) q

#define PLATFORM_WINDOWS

#ifndef ELIX_FILE_PATH_LENGTH
	#define ELIX_FILE_PATH_LENGTH 768
#endif

#ifndef ELIX_FILE_NAME_LENGTH
	#define ELIX_FILE_NAME_LENGTH 256
#endif

typedef void* data_pointer;
typedef FILE * file_pointer;

typedef uint64_t file_size;
typedef int64_t file_offset;
typedef enum {
	EFF_STATUS_UNKNOWN = 1 << 0,
	EFF_FILE_OPEN = 1 << 1, EFF_FILE_READ_ONLY = 1 << 2, EFF_FILE_READ_ERROR = 1 << 3 ,
	EFF_FILE_WRITE = 1 << 4, EFF_FILE_WRITE_ERROR = 1 << 5, EFF_WRITABLE = 1 << 6
} ElixFileFlag;

struct ElixFile {
	FILE * handle;
	file_size length;
	file_size pos;
	uint32_t flag;
};
typedef struct ElixFile elix_file;


file_offset elix_file_offset(elix_file * file) {
	// Note: due to pointless warnings, using ftello64 instead of ftello
	file_offset q = ftello64( FH(file->handle) );
	if ( q < 0 ) {
		return 0;
	}
	return q;
}

uint32_t elix_file_at_end(elix_file * file) {
	if ( file->handle ) {
		if ( file->pos == file->length )
			return true;
		return !(feof( FH(file->handle) ) == 0);
	}
	return true;
}

uint32_t elix_file_seek(elix_file * file, file_offset pos ) {
	if ( fseeko64( FH(file->handle), pos, SEEK_SET ) == 0) {
		file->pos = pos;
		return true;
	}
	return false;
}

size_t elix_file_read(elix_file * file, data_pointer buffer, size_t data_size, size_t amount) {
	if ( !file || !file->handle || file->length < data_size || !buffer ) {
		return 0;
	}

	size_t q = fread(buffer, data_size, amount, FH(file->handle));
	if ( q != 0 ) {
		file_offset p = ftello64( FH(file->handle) );
		if ( p >= 0 ) {
			file->pos = p;
		}
	}
	return q;
}

static inline file_size elix_file_tell(elix_file * file) {
	return (file_size)(elix_file_offset(file));
}

file_size elix_file__update_length(elix_file * file) {
	file_offset pos = elix_file_offset(file);
	fseeko64( FH(file->handle), 0, SEEK_END );
	file->length = elix_file_tell(file);
	fseeko64( FH(file->handle), pos, SEEK_SET );
	return file->length;
}

uint32_t elix_file_open(elix_file * file, const char * filename, ElixFileFlag mode) {
	fopen_s( &file->handle, filename, (mode & EFF_FILE_WRITE ? "wb" : "rb") );
	if ( file->handle ) {
		file->flag = mode;
		elix_file__update_length(file);
		return true;
	} else {
		LOG_MESSAGE("File not found %s", filename);
	}
	file->flag = (mode | EFF_FILE_WRITE ? EFF_FILE_WRITE_ERROR : EFF_FILE_READ_ERROR);
	return false;

}

uint32_t elix_file_close(elix_file * file) {
	if ( file->handle )	{
		return fclose( FH(file->handle) ) == 0;
	}
	return false;
}

#include <sys/types.h>
#include <dirent.h>

static inline size_t elix_cstring_length(const char * string, uint8_t include_terminator ) {
	if (string) {
		size_t c = 0;
		while(*string++) {
			++c;
		}
		return c + include_terminator;
	}
	return 0;
}

bool elix_cstring_has_suffix( const char * str, const char * suffix) {
	size_t str_length = elix_cstring_length(str, 0);
	size_t suffix_length = elix_cstring_length(suffix, 0);
	size_t offset = 0;

	if ( str_length < suffix_length )
		return false;

	offset = str_length - suffix_length;
	for (size_t c = 0; c < suffix_length;  c++ ) {
		if ( str[offset + c] != suffix[c] )
			return false;
	}
	return true;
}


size_t elix_cstring_append( char * str, const size_t len, const char * text, const size_t text_len) {
	size_t length = elix_cstring_length(str, 0);
	// TODO: Switch to memcpy
	for (size_t c = 0;length < len && c < text_len; length++, c++) {
		str[length] = text[c];
	}
	str[length+1] = 0;
	return length;
}

char * elix_cstring_from( const char * source, const char * default_str, size_t max_length ) {
	//ASSERT( default_str != nullptr )
	const char * ptr = source != nullptr ? source : default_str;
	size_t length = (max_length == SIZE_MAX) ? elix_cstring_length(ptr, 1) : max_length;

	if ( length <= 1 ) {
		//LOG_ERROR("elix_cstring_from failed, source was empty");
		return nullptr;
	}

	char * dest = malloc(length); //new char[length]();
	memcpy(dest, ptr, length);
	dest[length] = 0;
	return dest;
}


void elix_cstring_copy( const char * source_init, char * dest_init)
{
	char * source = (char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
	} while(*source != 0);
	*dest = 0;
}

void elix_cstring_copy_length( const char * source_init, char * dest_init, size_t length)
{
	size_t c = 0;
	char * source = (char *)source_init;
	char * dest = dest_init;
	do {
		*dest++ = *source++;
	} while(*source != 0 && c++ < length );
	*dest = 0;
}

struct ElixPath {
	char * uri;
	char * path;
	char * filename;
	char * filetype;
};
typedef struct ElixPath elix_path;


struct ElixDirectory {
	uint16_t count;
	elix_path * files;
};
typedef struct ElixDirectory elix_directory;


elix_path elix_path_create(const char * string) {
	elix_path uri;
	size_t length = elix_cstring_length(string, 0);
	size_t split = SIZE_MAX;
	size_t extension_split = SIZE_MAX;
	for (split = length-1; split > 0; split--) {
		if ( string[split] == '\\' || string[split] == '/') {
			split++;
			break;
		}
		if ( extension_split == SIZE_MAX && string[split] == '.') {
			extension_split = split +1;
		}
	}

	//ASSERT(split < length);

	uri.uri = malloc(length+1);
	uri.uri[length]=0;
	elix_cstring_copy(string, uri.uri);
	uri.path = elix_cstring_from(string, "/", split );
#ifdef PLATFORM_WINDOWS
	//elix_cstring_char_replace(uri.path, '\\', '/');
#endif
	if ( extension_split != SIZE_MAX ) {
		uri.filename = elix_cstring_from(string + split, "/", extension_split - split);
		uri.filetype = elix_cstring_from(string + extension_split, "/",SIZE_MAX);
	} else {
		uri.filename = elix_cstring_from(string + split, "/", length - split);
		uri.filetype = nullptr;
	}

	return uri;
}


void elix_os_directory_list_destroy(elix_directory ** directory) {
	for (int f = 0; f < (*directory)->count; ++f) {
		//free((*directory)->files[f]);
	}
	free((*directory)->files);
	//free(*directory);
	*directory = nullptr;
}

elix_directory * elix_os_directory_list_files(const char * path, const char * suffix) {
	elix_directory * directory = nullptr;
	DIR * current_directory;
	struct dirent * entity;
	current_directory = opendir(path);
	if ( !current_directory ) {
		return directory;
	}
	directory = calloc(1, sizeof(directory));
	while (entity = readdir(current_directory) ) {
		if ( entity->d_name[0] == '.' && (entity->d_name[1] == '.'|| entity->d_name[1]== 0)){

		} else if (suffix) {
			if ( elix_cstring_has_suffix(entity->d_name, suffix ) )
				directory->count++;
		} else {
			directory->count++;
		}
	}
	size_t path_len = elix_cstring_length(path, 0);
	directory->files = malloc(directory->count * sizeof(elix_path));
	rewinddir(current_directory);
	uint16_t index = 0;
	char buffer[ELIX_FILE_PATH_LENGTH] = {0};
	elix_cstring_copy(path, buffer);
	while (entity = readdir(current_directory) ) {
		if ( entity->d_name[0] == '.' && (entity->d_name[1] == '.'|| entity->d_name[1]== 0)){

		} else if (suffix) {
			if ( elix_cstring_has_suffix(entity->d_name, suffix ) ) {
				memset(buffer+path_len, 0, ELIX_FILE_PATH_LENGTH-path_len);
				elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, entity->d_name, elix_cstring_length(entity->d_name, 0));
				directory->files[index] = elix_path_create(buffer);
				index++;

			}
		} else {
			memset(buffer+path_len, 0, ELIX_FILE_PATH_LENGTH-path_len);
			elix_cstring_append(buffer, ELIX_FILE_PATH_LENGTH, entity->d_name, elix_cstring_length(entity->d_name, 0));
			directory->files[index] = elix_path_create(buffer);
			index++;

		}
	}
	closedir(current_directory);
	return directory;

}



size_t elix_file_read_line(elix_file * file, data_pointer data, size_t data_size) {
	if ( !file || !file->handle || !data ) {
		return 0;
	}

	uint8_t char_buffer;
	uint8_t * data_buffer = data;
	size_t index = 0;
	size_t q;// = elix_file_read(file, &char_buffer, 1,1);

	do {
		q = elix_file_read(file, &char_buffer, 1,1);

		if (char_buffer > 13 )
			data_buffer[index++] = char_buffer;

	} while ( char_buffer > 10  && index < data_size && !elix_file_at_end(file));
	data_buffer[index] = 0;
	return index;
}

bool elix_cstring_has_prefix( const char * str, const char * prefix) {
	size_t str_length = elix_cstring_length(str, 0);
	size_t prefix_length = elix_cstring_length(prefix, 0);

	if ( str_length < prefix_length )
		return false;

	for (size_t c = 0; c < prefix_length;  c++ ) {
		if ( str[c] != prefix[c] )
			return false;
	}
	return true;
}

void elix_cstring_trim( char * string ) {

	size_t length = elix_cstring_length(string, 0);

	while( length && string[0] == ' ' ) {
		memmove(string, string + 1, length);
		length--;
	}
	size_t pos = length ? length-1 : 0;
	while( pos && string[pos] == ' ' ) {
		string[pos] = 0;
		length--;
		pos--;
	}
}

	#include <winsock.h>

bool elix_socket_init( int32_t * sockDesc) {
	#if defined PLATFORM_WINDOWS
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		return 0;
	}
	#endif

	#if defined PLATFORM_NDS
	if ( !Wifi_InitDefault(WFC_CONNECT) )
	{
		return 0;
	}
	if ( (*sockDesc = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		return 0;
	}
	#elif defined PLATFORM_GCN
	if ( (*sockDesc = net_socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
		return 0;
	}
	#else
	if ( (*sockDesc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
	{
		return 0;
	}
	#endif
	return 1;
}


bool elix_socket_close( int32_t * sockDesc) {

	#if defined PLATFORM_WINDOWS
	closesocket(*sockDesc);
	WSACleanup();
	#elif defined PLATFORM_NDS
	closesocket(*sockDesc);
	#elif defined PLATFORM_GCN

	#else
	close(*sockDesc);
	#endif
}

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
/*
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