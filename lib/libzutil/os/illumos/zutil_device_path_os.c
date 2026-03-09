// SPDX-License-Identifier: CDDL-1.0
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or https://opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * CDDL HEADER END
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/dkio.h>

#include <libzutil.h>

/*
 * On illumos, device paths are typically /dev/dsk/cXtXdXsX or /dev/dsk/cXdXsX.
 * Partitions are indicated by the trailing 'sX' or 'pX' component.
 */

char *
zfs_strip_partition(const char *dev)
{
	char *copy = strdup(dev);
	if (copy == NULL)
		return (NULL);

	char *part = strrchr(copy, 's');
	if (part == NULL)
		part = strrchr(copy, 'p');
	if (part != NULL && part != copy) {
		char *p = part + 1;
		while (*p != '\0' && isdigit(*p))
			p++;
		if (*p == '\0')
			*part = '\0';
	}
	return (copy);
}

int
zfs_append_partition(char *path, size_t max_len)
{
	/* On illumos, append slice 0 (s0) for whole-disk usage */
	if (strlcat(path, "s0", max_len) >= max_len)
		return (-1);
	return (strnlen(path, max_len));
}

const char *
zfs_strip_path(const char *path)
{
	const char *dsk = "/dev/dsk/";
	size_t len = strlen(dsk);

	if (strncmp(path, dsk, len) == 0)
		return (path + len);
	if (strncmp(path, "/dev/", 5) == 0)
		return (path + 5);
	return (path);
}

char *
zfs_get_underlying_path(const char *dev_name)
{
	if (dev_name == NULL)
		return (NULL);
	return (realpath(dev_name, NULL));
}

boolean_t
zfs_dev_is_whole_disk(const char *dev_name)
{
	struct dk_gpt *vtoc;
	int fd;

	fd = open(dev_name, O_RDONLY);
	if (fd < 0)
		return (B_FALSE);
	(void) close(fd);
	return (B_TRUE);
}

int
zpool_label_disk_wait(const char *path, int timeout_ms)
{
	int settle_ms = 50;
	long sleep_ms = 10;
	hrtime_t start, settle;
	struct stat64 statbuf;

	start = gethrtime();
	settle = 0;

	do {
		errno = 0;
		if ((stat64(path, &statbuf) == 0) && (errno == 0)) {
			if (settle == 0)
				settle = gethrtime();
			else if (NSEC2MSEC(gethrtime() - settle) >= settle_ms)
				return (0);
		} else if (errno != ENOENT) {
			return (errno);
		}

		usleep(sleep_ms * MILLISEC);
	} while (NSEC2MSEC(gethrtime() - start) < timeout_ms);

	return (ENODEV);
}

boolean_t
is_mpath_whole_disk(const char *path)
{
	(void) path;
	return (B_FALSE);
}
