/*
 *  Low: a yum-like package manager
 *
 *  Download code adopted from razor:
 *  http://github.com/krh/razor/wikis
 *
 *  Copyright (C) 2008 James Bowes <jbowes@dangerouslyinc.com>
 *  Copyright (C) 2008 Devan Goodwin <dgoodwin@dangerouslyinc.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>
#include <glib.h>
#include <nss3/nss.h>
#include <nss3/sechash.h>
#include "low-download.h"

int
low_download_show_progress (void *clientp, double dltotal, double dlnow,
			    double ultotal G_GNUC_UNUSED,
			    double ulnow G_GNUC_UNUSED)
{
	const char *file = clientp;

	float tmp_now = dlnow;
	float tmp_total = dltotal;

	if (dlnow > dltotal || dltotal == 0) {
		return 0;
	}

	printf ("\rdownloading %s, ", file);

	if (tmp_total < 1023) {
		printf ("%.0fB/%.0fB", tmp_now, tmp_total);
		fflush (stdout);
		return 0;
	}

	tmp_now /= 1024;
	tmp_total /= 1024;
	if (tmp_total < 1023) {
		printf ("%.1fKB/%.1fKB", tmp_now, tmp_total);
		fflush (stdout);
		return 0;
	}

	tmp_now /= 1024;
	tmp_total /= 1024;
	if (tmp_total < 1023) {
		printf ("%.1fMB/%.1fMB", tmp_now, tmp_total);
		fflush (stdout);
		return 0;
	}

	tmp_now /= 1024;
	tmp_total /= 1024;
	printf ("%.1fGB/%.1fGB", tmp_now, tmp_total);
	fflush (stdout);
	return 0;
}

int
low_download (const char *url, const char *file, const char *basename)
{
	CURL *curl;
	char error[256];
	FILE *fp;
	CURLcode res;
	long response;

	curl = curl_easy_init ();
	if (curl == NULL) {
		return 1;
	}

	curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, error);
	curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION,
			  low_download_show_progress);
	curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, basename);

	fp = fopen (file, "w");
	if (fp == NULL) {
		fprintf (stderr, "failed to open %s for writing\n", file);
		return -1;
	}
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt (curl, CURLOPT_URL, url);
	res = curl_easy_perform (curl);
	fclose (fp);
	if (res != CURLE_OK) {
		fprintf (stderr, "curl error: %s\n", error);
		unlink (file);
		return -1;
	}
	res = curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response);
	if (res != CURLE_OK) {
		fprintf (stderr, "curl error: %s\n", error);
		unlink (file);
		return -1;
	}
	if (response != 200) {
		if (!(response == 226 && strncmp ("ftp", url, 3) == 0)) {
			fprintf (stderr, " - failed %ld\n", response);
			unlink (file);
			return -1;
		}
	}
	printf ("\n");

	curl_easy_cleanup (curl);

	return 0;
}

#define BUF_SIZE 32

static gboolean
compare_digest (const char *file, const char *expected,
		LowDigestType digest_type)
{
	unsigned char result[BUF_SIZE];
	char expected_raw[BUF_SIZE];
	unsigned int size;
	unsigned int i;
	int fd;
	HASHContext *ctx;

	NSS_NoDB_Init(NULL);

	switch (digest_type)
	{
		case DIGEST_MD5:
			ctx = HASH_Create(HASH_AlgMD5);
			break;
		case DIGEST_SHA1:
			ctx = HASH_Create(HASH_AlgSHA1);
			break;
		case DIGEST_SHA256:
			ctx = HASH_Create(HASH_AlgSHA256);
			break;
		case DIGEST_UNKNOWN:
		case DIGEST_NONE:
		default:
			return FALSE;
	}

	fd = open (file, O_RDONLY);

	HASH_Begin (ctx);

	while (1) {
		unsigned char buf[BUF_SIZE];
		int cnt = read(fd, &buf, BUF_SIZE);

		if (cnt == 0) {
			break;
		}

		HASH_Update (ctx, buf, cnt);
	}

	close (fd);
	HASH_End (ctx, result, &size, BUF_SIZE);

	for (i = 0; i < strlen(expected); i += 2) {
		char tmp[2];
		tmp[0] = expected[i];
		tmp[1] = expected[i + 1];

		expected_raw[i/2] = strtoul(tmp, NULL, 16);
	}

	return strncmp((char *) result, expected_raw, size) == 0;
}

int
low_download_if_missing (const char *url, const char *file,
			 const char *basename, const char *digest,
			 LowDigestType digest_type, off_t size)
{
	struct stat buf;
	int res;

	if (stat (file, &buf) < 0 || buf.st_size != size) {
		res = low_download (url, file, basename);
		if (res != 0) {
			unlink (file);
			return res;
		}
	}

	if (!compare_digest (file, digest, digest_type)) {
		unlink (file);
		return -1;
	}

	return 0;
}

/* vim: set ts=8 sw=8 noet: */
