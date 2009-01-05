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
#include <curl/curl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include "low-download.h"

int
low_download_show_progress(void *clientp, double dltotal, double dlnow,
			   double ultotal G_GNUC_UNUSED,
			   double ulnow G_GNUC_UNUSED)
{
	const char *file = clientp;

	if (!dlnow < dltotal)
		fprintf(stderr, "\rdownloading %s, %dkB/%dkB",
			file, (int) dlnow / 1024, (int) dltotal / 1024);

	return 0;
}

int
low_download(const char *url, const char *file, const char *basename)
{
	CURL *curl;
	char error[256];
	FILE *fp;
	CURLcode res;
	long response;

	curl = curl_easy_init();
	if (curl == NULL) {
		return 1;
	}

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
			 low_download_show_progress);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, basename);

	fp = fopen(file, "w");
	if (fp == NULL) {
		fprintf(stderr,
			"failed to open %s for writing\n", file);
		return -1;
	}
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	res = curl_easy_perform(curl);
	fclose(fp);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl error: %s\n", error);
		unlink(file);
		return -1;
	}
	res = curl_easy_getinfo(curl,
				CURLINFO_RESPONSE_CODE, &response);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl error: %s\n", error);
		unlink(file);
		return -1;
	}
	if (response != 200) {
		if (!(response == 226 && strncmp ("ftp", url, 3) == 0)) {
			fprintf(stderr, " - failed %ld\n", response);
			unlink(file);
			return -1;
		}
	}
	fprintf(stderr, "\n");

	curl_easy_cleanup(curl);

	return 0;
}
int
low_download_if_missing(const char *url, const char *file, const char *basename)
{
	struct stat buf;

	if (stat(file, &buf) < 0) {
		return low_download (url, file, basename);
	}

	return 0;
}

/* vim: set ts=8 sw=8 noet: */
