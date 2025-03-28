
#ifndef _SERVER_H
#define _SERVER_H

#include <sys/socket.h>

#define FILE_SPRITE_SIZE 1488170

#define BMP_SIZE_LOC 2
#define BMP_OFFSET_LOC 10
#define BMP_WIDTH_LOC 18
#define BMP_HEIGHT_LOC 22

extern std::string userid;

int create_listener_socket(int port, int& server_socket);
int handle_communication_until_done(int server_fd, struct sockaddr* address, socklen_t* addrlen);

#endif // _SERVER_H
