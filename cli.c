/*
 * File   : cli.c
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Implementation for console logging timestamps
 */

#include <time.h>

const char* getDTstr() {
	static char strDt[32];
    time_t t;
    time(&t);
	struct tm *gmt = gmtime(&t);
	strftime(strDt, sizeof(strDt), "%Y-%m-%d %H:%M:%S", gmt);
	return strDt;
}
