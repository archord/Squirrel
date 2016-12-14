/*
 * NamedPipe.h
 *
 *  Created on: Nov 29, 2016
 *      Author: wamdm
 */

#ifndef CROSS_SRC_NAMEDPIPE_H_
#define CROSS_SRC_NAMEDPIPE_H_

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

using namespace std;

class NamedPipe {
public:

	static const int buffer_size = 1024;
	string fifo_name;
	// write fd is useless
	int pipe_fd_wr;
	int pipe_fd_rd;
	int res;
	const int read_mode = O_RDONLY;
	const int write_mode = O_WRONLY;
	const int nonblock_mode = O_NONBLOCK;
	int bytes_sent;
	char buffer[buffer_size];

	NamedPipe( string fifo_name) {
		//buffer = new char[buffer_size];
		bytes_sent = 0;
		this->fifo_name = fifo_name;
		// first delete the fifo file
		//remove(fifo_name.c_str());
		if(access(fifo_name.c_str(), F_OK) == -1)
		{
			res = mkfifo(fifo_name.c_str(), 0777);
			if(res != 0)
			{
				fprintf(stderr, "Could not create fifo %s\n", fifo_name.c_str());
				exit(EXIT_FAILURE);
			}
		}
		pipe_fd_rd = open(fifo_name.c_str(), read_mode);
		pipe_fd_wr = open(fifo_name.c_str(), write_mode);
		memset(buffer, '\0', buffer_size);

	};

	~NamedPipe() {
		close(pipe_fd_wr);
		close(pipe_fd_rd);
	}

	char * getCommand() {
		//printf("call getCommand\n");
		if(pipe_fd_rd != -1)
		{
			do
			{
				//sleep(1);
				//printf("block\n");

				res = read(pipe_fd_rd, buffer, buffer_size);
			//	printf("%s\n",strerror(errno));
			}while(res <= 0);
		}
		return buffer;
	}
};


#endif /* CROSS_SRC_NAMEDPIPE_H_ */

