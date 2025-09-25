/*
 * File   : cli.h
 * Author : Thomas Mailaender
 * Date   : 2025-09-16
 *
 * Description : Definitions for simple colorized console logging
 */

#ifndef __CLI_H__
#define __CLI_H__

#include <stddef.h>
#include <stdio.h>

  //#define LF  0x0A
  //#define CR  0x0D
	//#define ESC	0x1B

  enum ASCII_Key {
    BS  = 0x08,
    LF  = 0x0A,
    CR  = 0x0D,
    ESC = 0x1B,
    DEL = 0x7F
  };


	// ANSI Escape SEQUENZEN  für Farbausgabe in Serial Terminal see https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences

	//#define RED		"\e[1;31m"	// So werden fette kraeftife Farben angezeigt (1; = BOLD)
	#define BOLD		"\e[1m"
	#define UNDERLINE	"\e[4m"
	// RESET all attributes
	#define COLOREND	"\e[0m"
	// Foreground Colors
	#define DARKGRAY	"\e[30m"
	#define RED			"\e[31m"
	#define GREEN		"\e[32m"
	#define YELLOW		"\e[33m"
	#define BLUE		"\e[34m"
	#define MAGENTA		"\e[35m"
	#define CYAN		"\e[36m"
	#define WHITE		"\e[37m"
	// Background colors
	#define BG_DARKGRAY	"\e[40m"
	#define BG_RED		"\e[41m"
	#define BG_GREEN	"\e[42m"
	#define BG_YELLOW	"\e[43m"
	#define BG_BLUE		"\e[44m"
	#define BG_MAGENTA	"\e[45m"
	#define BG_CYAN		"\e[46m"
	#define BG_WHITE	"\e[47m"


	#define log(...)		print(__VA_ARGS__)
	#define log_s(txt)		print(GREEN txt COLOREND)
	#define log_i(txt)		print(CYAN txt COLOREND)
	#define log_w(txt)		print(YELLOW txt COLOREND)
	#define log_e(txt)		fprint(stderr, RED txt COLOREND)

	#define log_ln(...)		printf(__VA_ARGS__ "\n")
	#define log_sln(txt)	printf(GREEN txt COLOREND "\n")
	#define log_iln(txt)	printf(CYAN txt COLOREND "\n")
	#define log_wln(txt)	printf(YELLOW txt COLOREND "\n")
	#define log_eln(txt)	fprintf(stderr, RED txt COLOREND "\n")

	#define log_f(...)				printf(__VA_ARGS__)
	#define log_sf(format, ...)		printf(GREEN format COLOREND, __VA_ARGS__)
	#define log_if(format, ...)		printf(CYAN format COLOREND, __VA_ARGS__)
	#define log_wf(format, ...)		printf(YELLOW format COLOREND, __VA_ARGS__)
	#define log_ef(format, ...)		fprintf(stderr, RED format COLOREND, __VA_ARGS__)

	#define log_fln(format, ...)	printf(format "\n", __VA_ARGS__)
	#define log_sfln(format, ...)	printf(GREEN format COLOREND "\n", __VA_ARGS__)
	#define log_ifln(format, ...)	printf(CYAN format COLOREND "\n", __VA_ARGS__)
	#define log_wfln(format, ...)	printf(YELLOW format COLOREND "\n", __VA_ARGS__)
	#define log_efln(format, ...)	fprintf(stderr, RED format COLOREND "\n", __VA_ARGS__)

	const char* getDTstr();
	//const char* txtMilliSec(uint64_t millisec);
	//const char* timeMilliSec(uint64_t millisec);

#endif
