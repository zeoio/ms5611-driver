#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUF_MAX		64

char *get_path(const char *base, const char *name)
{
	if (name == NULL)
		return NULL;

	char *pathname = malloc(sizeof(char) * (strlen(base)+strlen(name)+1));
	if (pathname == NULL) {
		printf("ERROR: Get pathname!\n");
		return NULL;
	}

	sprintf(pathname, "%s/%s", base, name);
	return pathname;
}

int write_ops(const char *pathname, unsigned short data)
{
	char buf[BUF_MAX] = "";
	int fd;

	fd = open(pathname, O_RDWR);
	if (fd < 0) {
		printf("ERROR: Open %s fail!\n", pathname);
		return -1;
	}

	sprintf(buf, "%d", data);

	if (write(fd, buf, strlen(buf)+1) < 0) {
		printf("ERROR: Write %s fail!\n", buf);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int read_block_ops(const char *pathname, char **buf)
{
	int fd;
	*buf = (char *)malloc(sizeof(char) * BUF_MAX);

	if (*buf) {
		fd = open(pathname, O_RDWR);
		if (fd < 0) {
			printf("Error: Open %s fail!\n", pathname);
			return -EIO;
		}

		if (read(fd, *buf, BUF_MAX) < 0) {
			printf("Error: Read %s fail!\n", pathname);
			return -EIO;
		}

		close(fd);
		return 0;
	}

	return -ENOMEM;
}

int read_ops(const char *pathname, unsigned short *data)
{
	char buf[BUF_MAX] = "";
	int fd;

	fd = open(pathname, O_RDWR);
	if (fd < 0) {
		printf("ERROR: Open %s fail!\n", pathname);
		return -EIO;
	}

	if (read(fd, buf, sizeof(buf)) < 0) {
		printf("ERROR: Read %s fail!\n", pathname);
		close(fd);
		return -1;
	}

	if (data)
		*data = (unsigned short)strtoul(buf, NULL, 10);

	close(fd);
	return 0;
}

int write_data(const char *base, const char *name, unsigned short data)
{
	char *pathname = get_path(base, name);

	if (pathname) {
		if (write_ops(pathname, data) < 0)
			return -1;
	}

	free(pathname);
	return 0;
}

/* int get_ops(const char *base, const char *name, unsigned short *data)
 * {
 *         char *pathname = get_path(base, name);
 * 
 *         if (pathname) {
 *                 if (read_ops(pathname, data) < 0)
 *                         return -1;
 *         }
 * 
 *         free(pathname);
 *         return 0;
 * } */

int read_data(const char *base, const char *name, unsigned short *data)
{
	char *pathname = get_path(base, name);
	int ret = -1;

	if (pathname) {
		ret = read_ops(pathname, data);
		free(pathname);
	}

	return ret;
}

int read_data_block(const char *base, const char *name, char **buf)
{
	char *pathname = get_path(base, name);
	int ret = -1;

	if (pathname) {
		ret = read_block_ops(pathname, buf);
		free(pathname);
	}

	return ret;
}
