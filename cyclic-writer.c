#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define BLKSIZE 512

int
main(int argc, char **argv)
{
	if (argc != 3)
		return (1);

	long blkno = strtol(argv[2], NULL, 10);

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int fd = open(argv[1], O_WRONLY | O_CREAT | O_EXCL, mode);
	if (fd < 0) {
		perror("open");
		return (1);
	}

	uint8_t *buf = malloc(BLKSIZE);
	if (buf == NULL)
		return (1);

	uint64_t left = blkno;

	size_t i = 0;
	while (left != 0) {
		memset(buf, 'A' + (i++ % 26), BLKSIZE);

		if (write(fd, buf, BLKSIZE) < 0) {
			perror("write");
			break;
		}

		left--;
	}

	close(fd);

	return(0);
}
