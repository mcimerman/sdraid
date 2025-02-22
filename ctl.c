#define _DEFAULT_SOURCE

#include <linux/limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <assert.h>
#include <poll.h>
#include <stdbool.h>

#include "common.h"

#define MAX_INPUT_SIZE 256

#define ASSERT_HARD	true
#define ASSERT_SOFT	false

#ifndef BAUDRATE
#error "BAUDRATE not set"
#endif

/* all modes */
#define HELP_STR	"help"
#define FLUSH_STR	"flush"
/* init mode */
#define CREATE_STR	"create"
#define ASSEMBLE_STR	"assemble"
/* normal mode */
#define LS_STR		"ls"
#define PWD_STR		"pwd"
#define CD_STR		"cd "
#define STATUS_STR	"status"
#define PUT_STR		"put "
#define GET_STR		"get "
#define DEBUG_STR	"debug"
/* debug mode */
#define READ_STR	"read"
#define READ2_STR	"read2"
#define EXIT_STR	"exit"

typedef enum {
	INVALID,

	USAGE,
	FLUSH,

	CREATE,
	ASSEMBLE,

	LS,
	PWD,
	CD,
	STATUS,
	PUT,
	GET,
	DEBUG,

	READ,
	READ2,
	EXIT
} cmd_type_t;


static void usage(void);
static int init_tty(const char *);
static void init_menu(void);
static void normal_menu(void);
static void debug_menu(void);
static int cwd(char **);
static cmd_type_t parse_input(char *, char **);
static void clear_serial_buffer(int);
static bool assert_ok(bool);
static void print_block(uint32_t, uint8_t *);
static void get_block(uint8_t *);
static void put(const char *);
static void get(const char *);
static int create(char *);


int ttyfd; /* laziness */


int
main(int argc, char **argv)
{
	if (argc != 2) {
		printf("usage: ctl <serial dev>\n");
		return (1);
	}

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	if (init_tty(argv[1]) != 0)
		err(EXIT_FAILURE, "init_tty()");

	init_menu();

	return (0);
}

static void
usage(void)
{
	printf("all modes:\n");
	printf("\tflush - flushes uart read buffer\n");
	printf("initialize mode:\n");
	printf("\tcreate - create new volume and write metadata\n");
	printf("\tassemble - assemble volume from metadata\n");
	printf("normal mode:\n");
	printf("\tpwd - print current working directory\n");
	printf("\tls - list current working directory\n");
	printf("\tcd <path> - change working directory\n");
	printf("\tput <file> - writes file to the volume\n");
	printf("\tget <file> - reads volume contents into file\n");
	printf("\tdebug - switches to debug mode\n");
	printf("debug mode:\n");
	printf("\tread - reads single block from volume\n");
	printf("\tread2 - reads single block from specified sd\n");
	printf("\texit - exits back into normal mode\n");
}

static int
init_tty(const char *tty_path)
{
	struct termios tty;
	ttyfd = open(tty_path, O_RDWR | O_NOCTTY);
	if (ttyfd == -1)
		err(EXIT_FAILURE, "open()");

	tcgetattr(ttyfd, &tty);

	cfsetispeed(&tty, BAUDRATE);
	cfsetospeed(&tty, BAUDRATE);

	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;
	tty.c_cflag |= CREAD | CLOCAL;

	cfmakeraw(&tty);

	tcsetattr(ttyfd, TCSANOW, &tty);

	sleep(1);

	clear_serial_buffer(1500);

	return (0);
}

static void
init_menu(void)
{
	char input[MAX_INPUT_SIZE];
	cmd_type_t intype;
	size_t len;

	for (;;) {
		printf("(init) > ");
		fgets(input, MAX_INPUT_SIZE, stdin);
		len = strlen(input);
		input[len - 1] = '\0';

		intype = parse_input(input, NULL);
		switch (intype) {
		case ASSEMBLE:
			write(ttyfd, "A", 1);
			clear_serial_buffer(250);
			if (!assert_ok(ASSERT_SOFT)) {
				break;
			}
			normal_menu();
			break;
		case CREATE:
			if (create(input) == 0)
				normal_menu();
			break;
		case USAGE:
			usage();
			break;
		case FLUSH:
			clear_serial_buffer(1500);
			break;
		default:
			printf("invalid command, for usage see: help\n");
			break;
		}
	}
}

static void
normal_menu(void)
{
	char input[MAX_INPUT_SIZE];
	cmd_type_t intype;
	struct dirent *entry;
	char *arg;

	for (;;) {
		printf("> ");
		fgets(input, MAX_INPUT_SIZE, stdin);
		size_t len = strlen(input);
		input[len - 1] = '\0';
		intype = parse_input(input, &arg);
		switch (intype) {
		case LS:
			if (cwd(&arg) != 0)
				break;
			DIR *dir = opendir(arg);
			if (dir == NULL)
				err(EXIT_FAILURE, "opendir()");
			while ((entry = readdir(dir)) != NULL)
				printf("| %s\n", entry->d_name);
			free(arg);
			break;
		case CD:
			if (chdir(arg) != 0)
				perror("chdir()");
			break;
		case PWD:
			(void)cwd(NULL);
			break;
		case STATUS:
			write(ttyfd, "S", 1);
			clear_serial_buffer(1500);
			break;
		case PUT:
			put(arg);
			break;
		case GET:
			get(arg);
			break;
		case DEBUG:
			write(ttyfd, "D", 1);
			clear_serial_buffer(1500);
			debug_menu();
			break;
		case USAGE:
			usage();
			break;
		case FLUSH:
			clear_serial_buffer(1500);
			break;
		default:
			printf("invalid command, for usage see: help\n");
			break;
		}
	}
}

static void
debug_menu(void)
{
	char input[MAX_INPUT_SIZE];
	cmd_type_t intype;

	for (;;) {
		printf("(debug) > ");
		fgets(input, MAX_INPUT_SIZE, stdin);
		size_t len = strlen(input);
		input[len - 1] = '\0';
		intype = parse_input(input, NULL);
		switch (intype) {
		case READ:
			write(ttyfd, "r", 1);
			printf("(volume) block address? > ");
			goto read_common;
		case READ2:
			printf("device number? [0, DEVNO) > ");
			fgets(input, MAX_INPUT_SIZE, stdin);
			uint8_t devno = atoi(input);

			write(ttyfd, "R", 1);

			write(ttyfd, &devno, 1);

			printf("(sd) block address? > ");
	read_common:
			fgets(input, MAX_INPUT_SIZE, stdin);
			uint32_t offset = strtol(input, NULL, 10);
			/* XXX: little endian is assumed */
			write(ttyfd, &offset, sizeof(uint32_t));

			poll(NULL, 0, 100);

			if (!assert_ok(ASSERT_SOFT))
				break;

			sleep(3);

			uint8_t readbuf[BLKSIZE] = { 0 };
			get_block(readbuf);

			if (!assert_ok(ASSERT_SOFT))
				break;

			print_block(offset, readbuf);

			break;
		case EXIT:
			write(ttyfd, "E", 1);
			return;
		case USAGE:
			usage();
			break;
		case FLUSH:
			clear_serial_buffer(1500);
			break;
		default:
			printf("invalid command, for usage see: help\n");
			break;
		}
	}
}

static cmd_type_t
parse_input(char *in, char **rarg1)
{
	if (strncmp(in, HELP_STR, sizeof(HELP_STR)) == 0) {
		return (USAGE);
	} else if (strncmp(in, FLUSH_STR, sizeof(FLUSH_STR)) == 0) {
		return (FLUSH);
	} else if (strncmp(in, CREATE_STR, sizeof(CREATE_STR)) == 0) {
		return (CREATE);
	} else if (strncmp(in, ASSEMBLE_STR, sizeof(ASSEMBLE_STR)) == 0) {
		return (ASSEMBLE);
	} else if (strncmp(in, PWD_STR, sizeof(PWD_STR)) == 0) {
		return (PWD);
	} else if (strncmp(in, LS_STR, sizeof(LS_STR)) == 0) {
		return (LS);
	} else if (strncmp(in, CD_STR, sizeof(CD_STR) - 1) == 0) {
		in += strlen(CD_STR);
		while (*in == ' ')
			in++;
		if (rarg1 != NULL)
			*rarg1 = in;
		return (CD);
	} else if (strncmp(in, STATUS_STR, sizeof(STATUS_STR)) == 0) {
		return (STATUS);
	} else if (strncmp(in, PUT_STR, sizeof(PUT_STR) - 1) == 0) {
		in += strlen(PUT_STR);
		while (*in == ' ')
			in++;
		if (rarg1 != NULL)
			*rarg1 = in;
		return (PUT);
	} else if (strncmp(in, GET_STR, sizeof(GET_STR) - 1) == 0) {
		in += strlen(GET_STR);
		while (*in == ' ')
			in++;
		if (rarg1 != NULL)
			*rarg1 = in;
		return (GET);
	} else if (strncmp(in, DEBUG_STR, sizeof(DEBUG_STR)) == 0) {
		return (DEBUG);
	} else if (strncmp(in, READ_STR, sizeof(READ_STR)) == 0) {
		return (READ);
	} else if (strncmp(in, READ2_STR, sizeof(READ2_STR)) == 0) {
		return (READ2);
	} else if (strncmp(in, EXIT_STR, sizeof(EXIT_STR)) == 0) {
		return (EXIT);
	}

	return (INVALID);
}

static int 
cwd(char **rcwd)
{
	char cwd[PATH_MAX];

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd()");
		return (1);
	} else {
		printf("cwd: %s\n", cwd);
	}

	if (rcwd != NULL) {
		/* caller has to free() */
		char *buf = calloc(1, strlen(cwd) + 1);
		if (buf == NULL)
			err(EXIT_FAILURE, "calloc()");
		memcpy(buf, cwd, strlen(cwd));
		*rcwd = buf;
	}
	return (0);
}

static void
clear_serial_buffer(int timeout)
{
	struct pollfd fds;
	fds.fd = ttyfd;
	fds.events = POLLIN;
	int poll_result;
	char dump_buffer[BLKSIZE + 1] = { 0 };

	poll_result = poll(&fds, 1, timeout);

	if (poll_result > 0) {
		read(ttyfd, dump_buffer, BLKSIZE);
		printf("%s", dump_buffer);
		clear_serial_buffer(timeout);
	} else if (poll_result == 0) {
	} else {
		perror("polling error");
	}
}

static bool 
assert_ok(bool hard)
{
	struct pollfd fds;
	fds.fd = ttyfd;
	fds.events = POLLIN;
	int poll_result;
	char dump_buffer[BLKSIZE + 1] = { 0 };

	poll_result = poll(&fds, 1, 1500);

	if (poll_result > 0) {
		read(ttyfd, dump_buffer, BLKSIZE);
		if (strncmp(dump_buffer, "OK", 2) == 0)
			return (true);
		printf("%s", dump_buffer);
	} else if (poll_result == 0) {
	} else {
		perror("polling error");
	}

	if (hard)
		assert(0);
	else
		printf("failed to (soft) assert OK\n");

	return (false);
}

static void
put(const char *filename)
{
	struct stat st;

	int filefd = open(filename, O_RDONLY);
	if (filefd == -1) {
		perror("open()");
		return;
	}

	if (stat(filename, &st) != 0) {
		perror("stat()");
		goto end;
	}

	uint32_t file_size = st.st_size;

	if (file_size == 0) {
		printf("file size cannot be 0\n");
		goto end;
	}

	uint32_t blks = file_size / BLKSIZE;
	if (file_size % BLKSIZE != 0)
		blks++;

	write(ttyfd, "P", 1);
	/* XXX: little endian is assumed */
	write(ttyfd, &file_size, sizeof(uint32_t));
	write(ttyfd, &blks, sizeof(uint32_t));
	clear_serial_buffer(250);
	if (!assert_ok(ASSERT_SOFT))
		goto end;

	sleep(3);

	uint32_t written_blks = 0;
	uint8_t filebuf[BLKSIZE + 1] = { 0 };
	while (written_blks != blks) {
		int read_bytes = read(filefd, filebuf, BLKSIZE);
		if (read_bytes < 0) {
			perror("read()");
			goto end;
		} else if (read_bytes < BLKSIZE) {
			memset(filebuf + read_bytes, 0, BLKSIZE - read_bytes);
		}

		int n = write(ttyfd, filebuf, BLKSIZE);
		if (!assert_ok(ASSERT_SOFT))
			goto end;

		if (n < BLKSIZE) {
			perror("write()");
			goto end;
		}
		written_blks++;

		printf("progress: %u/%u blocks written (%u%%)\n",
		    written_blks, blks, (uint32_t)((float)written_blks / blks * 100));
	}

	assert_ok(ASSERT_SOFT);

end:
	close(filefd);
}

static void
get(const char *filename)
{
	uint32_t file_size;

	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int filefd = open(filename, O_WRONLY | O_CREAT | O_EXCL, mode);
	if (filefd == -1) {
		perror("open()");
		return;
	}

	write(ttyfd, "G", 1);
	read(ttyfd, &file_size, sizeof(uint32_t));

	if (file_size == 0) {
		printf("file_size = 0, nothing to get\n");
		close(filefd);
		if (remove(filename) != 0)
			perror("remove()");
		return;
	}

	uint32_t blks = file_size / BLKSIZE;
	bool truncate = false;
	if (file_size % BLKSIZE != 0) {
		blks++;
		truncate = true;
	}

	uint8_t filebuf[BLKSIZE] = { 0 };
	for (uint32_t i = 0; i < blks; i++) {
		get_block(filebuf);
		if (i + 1 == blks && truncate)
			write(filefd, filebuf, file_size % BLKSIZE);
		else
			write(filefd, filebuf, BLKSIZE);
		printf("progress: %u/%u blocks read (%u%%)\n",
		    i + 1, blks, (uint32_t)(((float)i + 1) / blks * 100));
	}

	assert_ok(ASSERT_SOFT);

	close(filefd);
}

static int
create(char *input)
{
	printf("device number? [2, %u] > ", MAX_DEVNO);
	fgets(input, MAX_INPUT_SIZE, stdin);
	uint8_t devno = atoi(input);
	assert(devno > 1 && devno <= MAX_DEVNO);

	write(ttyfd, "C", 1);

	write(ttyfd, &devno, sizeof(uint8_t));

	printf("level? (0, 1, 5) > ");
	fgets(input, MAX_INPUT_SIZE, stdin);
	uint8_t level = atoi(input);
	assert(level == RAID0 || level == RAID1 || level == RAID5);

	write(ttyfd, &level, sizeof(uint8_t));

	if (level == RAID5) {
		printf("layout?\n");
		printf("\tHR_RLQ_RAID4_0  = 0\n");
		printf("\tHR_RLQ_RAID4_N  = 1\n");
		printf("\tHR_RLQ_RAID5_0R = 2\n");
		printf("\tHR_RLQ_RAID5_NR = 3\n");
		printf("\tHR_RLQ_RAID5_NC = 4\n");
		printf("layout? > ");
		fgets(input, MAX_INPUT_SIZE, stdin);
		uint8_t layout = atoi(input);
		assert(layout <= HR_RLQ_RAID5_NC);

		write(ttyfd, &layout, sizeof(uint8_t));
	}

	clear_serial_buffer(250);

	if (!assert_ok(ASSERT_SOFT))
		return (1);

	return (0);
}

static void
print_block(uint32_t offset, uint8_t *buf)
{
	printf("0x%.8x:\t", offset * BLKSIZE);
	for (int i = 0; i < BLKSIZE; i++) {
		printf("%.2x ", buf[i]);
		if (i > 0 && (i + 1) % 16 == 0) {
			printf("|");
			for (int j = i - 15; j <= i; j++) {
				if (buf[j] >= 33 && buf[j] <= 126)
					printf("%c", buf[j]);
				else
					printf(".");
			}
			printf("|");
			printf("\n");
			if (i + 1 == BLKSIZE)
				break;
			printf("0x%.8x:\t", offset * BLKSIZE + i);
		}
	}
}

static void
get_block(uint8_t *buf)
{
	int bytes_read = 0;
	while (bytes_read < BLKSIZE) {
		/* 32 is the serial buffer size */
		int n = read(ttyfd, buf + bytes_read, 32);
		bytes_read += n;
	}
}

