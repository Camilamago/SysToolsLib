﻿/*****************************************************************************\
*									      *
*  File name:	    chars.c						      *
*									      *
*  Description:     Display all 8-bit characters in hexadecimal		      *
*									      *
*  Notes:	    Useful to see what characters look like in your current   *
*		    code page.						      *
*									      *
*  History:								      *
*    1995-11-03 JFL jf.larvoire@hp.com created this program.		      *
*    2015-01-08 JFL Fix output for Linux, including when encoding is UTF-8.   *
*		    Version 1.1.					      *
*    2017-03-06 JFL Added an optional code page argument for Windows.         *
*		    Version 1.2.					      *
*    2017-11-17 JFL Fixed the output of the NUL character.		      *
*    2019-01-14 JFL Added option -u to display Unicode characters or ranges.  *
*		    Improved error reporting when switching code pages.	      *
*		    Added option -v to display verbose information.	      *
*		    Version 1.4.					      *
*    2019-01-16 JFL Avoid outputing bytes \x80-\xFF by default for UTF-8 CPs. *
*		    Version 1.4.1.					      *
*    2019-04-19 JFL Use the version strings from the new stversion.h. V.1.4.2.*
*    2019-06-12 JFL Added PROGRAM_DESCRIPTION definition. Version 1.4.3.      *
*    2020-04-19 JFL Added support for MacOS. Version 1.5.                     *
*    2022-02-01 JFL Prevent a misalignment in Windows Terminal. Version 1.6.  *
*    2022-10-19 JFL Moved IsSwitch() to SysLib. Version 1.6.1.		      *
*    2023-01-14 JFL Allow building BIOS & LODOS versions. Version 1.6.2.      *
*    2023-01-16 JFL Make sure the output columns are always aligned, even     *
*		    when outputing undefined or 0-width characters.           *
*    2023-01-18 JFL Moved GetCursorPosition() to SysLib. Version 1.7.         *
*		    							      *
*       © Copyright 2016-2017 Hewlett Packard Enterprise Development LP       *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#define PROGRAM_DESCRIPTION "Output character tables"
#define PROGRAM_NAME    "chars"
#define PROGRAM_VERSION "1.7"
#define PROGRAM_DATE    "2023-01-18"

#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ 2005 security warnings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _BIOS
#include <unistd.h>	/* For getpid(), isatty(), read(), write() */
#endif

/************************ Win32-specific definitions *************************/

#ifdef _WIN32		/* Automatically defined when targeting a Win32 app. */

#include <windows.h>

#include <io.h>		/* For _setmode() */
#include <fcntl.h>	/* For _O_BINARY */

#define EOL "\r\n"

#define SUPPORTS_UTF8 TRUE

#endif /* _WIN32 */

/************************ MS-DOS-specific definitions ************************/

#ifdef _MSDOS		/* Automatically defined when targeting an MS-DOS app. */

#include <io.h>		/* For _setmode() */
#if !(defined(_BIOS) || defined(_LODOS))
#include <fcntl.h>	/* For _O_BINARY */
#endif

#define EOL "\r\n"

#define SUPPORTS_UTF8 FALSE

#endif /* _MSDOS */

/************************* OS/2-specific definitions *************************/

#ifdef _OS2	/* To be defined on the command line for the OS/2 version */

#define EOL "\r\n"

#define SUPPORTS_UTF8 FALSE

#endif /* _OS2 */

/************************* Unix-specific definitions *************************/

#if defined(__unix__) || defined(__MACH__) /* Automatically defined when targeting Unix or Mach apps. */

#define _UNIX

#define EOL "\n"

#define SUPPORTS_UTF8 TRUE

#endif /* __unix__ */

/********************** End of OS-specific definitions ***********************/

#ifndef CDECL
#ifdef _MSC_VER
#define CDECL _cdecl /* LODOS builds generate a warning if main() does not specify this */
#else
#define CDECL
#endif
#endif

/* SysToolsLib include files */
#include "console.h"	/* SysLib console management. Ex: GetCursorPosition */
#include "mainutil.h"	/* SysLib helper routines for main() */
#include "stversion.h"	/* SysToolsLib version strings. Include last. */

char spaces[] = "            ";

void usage(void);
#if SUPPORTS_UTF8
int ToUtf8(unsigned int c, char *b);
#endif /* SUPPORTS_UTF8 */

#ifdef _WIN32
int GetAncestorProcessName(pid_t pid, int iLevel, char *name, size_t lname);
#endif

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    main						      |
|									      |
|   Description     Program main initialization routine			      |
|									      |
|   Parameters      int argc		    Number of arguments 	      |
|		    char *argv[]	    List of arguments		      |
|									      |
|   Returns	    The return code to pass to the OS			      |
|									      |
|   History								      |
|    1995-11-03 JFL Created this routine				      |
*									      *
\*---------------------------------------------------------------------------*/

int CDECL main(int argc, char *argv[]) {
  int i, j;
#ifdef _WIN32
  unsigned uCP = 0;
  unsigned uCP0 = GetConsoleOutputCP();
#endif
  int iAll = FALSE;
  int iFirst = 0;
  int iLast = 0xFF;
  int iVerbose = FALSE;
  int iBase;
  int nBlock = 0;
#if SUPPORTS_UTF8
  int isUTF8 = FALSE;
#endif /* SUPPORTS_UTF8 */
#ifdef _UNIX
  char *pszLang = getenv("LANG");
#endif
#ifdef _WIN32
  /* The new Windows Terminal behaves differently from the old Windows Console */
  char szNameBuf[32];
  pid_t pid = GetAncestorProcessName(0, 2, szNameBuf, sizeof(szNameBuf));
  int isWindowsTerminal = (pid && streq(szNameBuf, "WindowsTerminal.exe"));
#endif
#ifndef _BIOS
  int isTTY = isatty(1);
#else
  int isTTY = TRUE; /* The BIOS version can only write to the console */
#endif

#if SUPPORTS_UTF8
#ifdef _WIN32
  if (uCP0 == 65001) isUTF8 = TRUE;
#endif
#ifdef _UNIX
  if (pszLang && strstr(pszLang, "UTF-8")) isUTF8 = TRUE;
  /* Note that Unix XTerm considers bytes \x80-\x9F as control sequences
     equivalent to ESC @, ESC A, ESC B, ..., ESC _ . 
     Do not output them, else there may be unpredictable effects on the console,
     depending on what follows. */
#endif
  if (isUTF8) iLast = 0x7F;
#endif /* SUPPORTS_UTF8 */

  for (i=1; i<argc; i++) {
    char *arg = argv[i];
    if (IsSwitch(arg)) {
      char *opt = arg+1;
      if (   streq(opt, "a")     /* -a: Display all characters */
	  || streq(opt, "-all")) {
	iAll = TRUE;
	continue;
      }
      if (   streq(opt, "h")	    /* Display usage */
	  || streq(opt, "help")	/* The historical name of that switch */
	  || streq(opt, "-help")
	  || streq(opt, "?")) {
	usage();
      }
#if SUPPORTS_UTF8
      if (   streq(opt, "u")     /* -u: Display unicode characters */
	  || streq(opt, "-unicode")) {
	int nc;
	if (((i+1)<argc) && sscanf(argv[++i], "%x%n", &iFirst, &nc)) {
#ifdef _WIN32
	  isUTF8 = TRUE;
	  uCP = 65001;
#endif /* defined(_WIN32) */
	  iLast = iFirst;
	  if (iVerbose) printf("Code point 0x%X", iFirst);
	} else {
	  fprintf(stderr, "No or bad unicode code point.\n");
	  return 1;
	}
	arg = argv[i];
	if ((arg[nc] == '-') && sscanf(arg+nc+1, "%x", &iLast)) {
	  if (iLast < iFirst) iLast = iFirst;
	  if (iVerbose) printf(" until 0x%X", iLast);
	}
	if (iVerbose) printf("\n");
	continue;
      }
#endif /* SUPPORTS_UTF8 */
      if (   streq(opt, "v")     /* -a: Display all characters */
	  || streq(opt, "-verbose")) {
	iVerbose = TRUE;
	continue;
      }
      if (   streq(opt, "V")     /* -V: Display the version */
	  || streq(opt, "-version")) {
	puts(DETAILED_VERSION);
	exit(0);
      }
      fprintf(stderr, "Unrecognized switch %s. Ignored.\n", arg);
      continue;
    }
#ifdef _WIN32
    if (!uCP) {
      uCP = atoi(arg);
      if ((uCP <= 0) || (uCP > 65535)) {
	fprintf(stderr, "Invalid code page: %s\n", arg);
	return 1;
      }
      continue;
    }
#endif /* defined(_WIN32) */
    fprintf(stderr, "Unrecognized argument %s. Ignored.\n", arg);
  }

#ifdef _WIN32
  if (uCP && (uCP != uCP0)) {
    if (iVerbose) printf("Switching to code page %d.\n", uCP);
    if (!SetConsoleOutputCP(uCP)) {
      fprintf(stderr, "Failed to switch to code page %d.\n", uCP);
      return 1;
    }
  } else if (iVerbose) printf("Active code page: %d\n", uCP0);
#endif /* defined(_WIN32) */

#if defined(_WIN32) || (defined(_MSDOS) && !(defined(_BIOS) || defined(_LODOS)))
  fflush(stdout); /* Make sure any previous output is done in text mode */
  _setmode(1, _O_BINARY ); /* fileno(stdout) = 1 */
#endif

#if SUPPORTS_UTF8
  if (iFirst == iLast) {
    char buf[5];
    int n = ToUtf8(iFirst, buf);
    buf[n] = '\0';
    if (!n) {
      fprintf(stderr, "Invalid code point: 0x%X.\n", iFirst);
      return 1;
    }
    if (iVerbose) {
      printf("UTF-8 ");
      for (i=0; i<n; i++) printf("\\x%02X ", buf[i]&0xFF);
      printf("\n");
    }
    printf("%s\n", buf);
    goto cleanup;
  }
#endif /* SUPPORTS_UTF8 */

  for (iBase = (iFirst & -0x80); iBase < ((iLast + 0x7F) & -0x80); iBase += 0x80) {
    int iDigits = 2;
    for (i=0x100; i; i<<=4) if (iBase >= i) iDigits += 1;
    if (nBlock) printf(EOL);
    if (iVerbose || (iFirst != 0) || ((iLast != 0x7F) && (iLast != 0xFF))) {
      printf("[0x%X-0x%X]\n", iBase, iBase + 0x7F);
    }
    for (j=0; j<16; j++) {
      for (i=0; i<8; i++) {
	int k, l;
	int iRow0, iCol0, iRow1, iCol1;

	if (!(i&3)) printf("  ");

	l = k = (iBase + 16*i)+j;
	if ((k < iFirst) || (k > iLast)) {
	  printf("%.*s", iDigits+4, spaces);
	  continue;
	}
	switch (k) {
#ifndef _UNIX
	  case 0x07:
	  case 0x08:
	  case 0x09:
	  case 0x0A:
	  case 0x0D:
	  case 0x1A:
	    if (!iAll) l = ' ';
	    break;
#endif
	  default:
#ifdef _UNIX
	    if (k < 0x20) {
	      if (!iAll) l = ' ';
	      break;
	    }
#endif
#ifdef _WIN32
	    /* The new Windows Terminal interprets several ASCII control chars */
	    if (isWindowsTerminal) {
	      switch (k) {
		case 0x00:
		case 0x0B:
		case 0x0C:
		case 0x0E:
		case 0x0F:
		case 0x1B:
		  if (!iAll) l = ' ';
		  break;
	      }
	    }
#endif
	    break;
	}

	printf("  %02X ", k); /* Do not use %c for the char, else the NUL char is not written */
        iRow0 = iCol0 = 0; /* Prevent an (incorrect) uninitialized variable warning later on */
	if (isTTY && ((l < ' ') || (l >= '\x7F'))) {
	  fflush(stdout);
	  GetCursorPosition(&iCol0, &iRow0);
	}
#if SUPPORTS_UTF8
	if ((k > 0x7F) && isUTF8) {
	  char buf[5];
	  int n = ToUtf8(k, buf);
	  buf[n] = '\0';
	  printf("%s", buf);
	} else
#endif /* SUPPORTS_UTF8 */
	{
#ifndef _BIOS
	  fwrite(&l, 1, 1, stdout); /* Do not use printf %c for the char, else the NUL char is not written */
#else
	  putchar((char)l); /* BIOSLIB's putchar() uses the BIOS int 10H */
#endif
	}
	if (isTTY && ((l < ' ') || (l >= '\x7F'))) { /* The cursor may not move for undefined or 0-width characters */
	  fflush(stdout);
	  GetCursorPosition(&iCol1, &iRow1);
	  /* If it did not move, then add a space to keep the alignment of subsequent columns */
	  if (iCol1 == iCol0 && iRow1 == iRow0) putchar(' ');
	}
      }
      printf(EOL);
      nBlock += 1;
    }
  }

#if SUPPORTS_UTF8
cleanup:
#endif /* SUPPORTS_UTF8 */
#ifdef _WIN32
  if (uCP && (uCP != uCP0)) {
    if (iVerbose) printf("Switching back to code page %d.\n", uCP0);
    if (!SetConsoleOutputCP(uCP0)) {
      fprintf(stderr, "Failed to switch to code page %d.\n", uCP0);
      return 1;
    }
  }
#endif

  return 0;
}

void usage(void) {
#if SUPPORTS_UTF8 && _WIN32
  unsigned uCP0 = GetConsoleOutputCP();;
  if (uCP0 != 65001) SetConsoleOutputCP(65001);
#endif

  printf(
PROGRAM_NAME_AND_VERSION " - " PROGRAM_DESCRIPTION "\n\
\n"
#ifdef _WIN32
"Usage: chars [SWITCHES] [CODEPAGE]\n"
#else
"Usage: chars [SWITCHES]\n"
#endif
"\n\
Switches:\n\
  -?|-h|--help        Display this help screen\n\
  -a|--all            Output all characters, even control chars like CR LF, etc\n"
#if SUPPORTS_UTF8
"\
  -u|--unicode X[-Y]  Display a Unicode character, or a range of characters\n"
#endif
"\
  -v|--verbose        Display verbose information\n\
  -V|--version        Display this program version and exit\n\
"
#include "footnote.h"
);

#if SUPPORTS_UTF8 && _WIN32
  if (uCP0 != 65001) SetConsoleOutputCP(uCP0);
#endif

  exit(0);
}

#if SUPPORTS_UTF8
#define B(c) ((char)(c))
int ToUtf8(unsigned int c, char *b) {
  char *b0 = b;
  if (c<0x80) *b++=B(c);
  else if (c<0x800) *b++=B(192+c/64), *b++=B(128+c%64);
  else if (c-0xd800u<0x800) return 0;
  else if (c<0x10000) *b++=B(224+c/4096), *b++=B(128+c/64%64), *b++=B(128+c%64);
  else if (c<0x110000) *b++=B(240+c/262144), *b++=B(128+c/4096%64), *b++=B(128+c/64%64), *b++=B(128+c%64);
  else return 0;
  return (int)(b-b0);
}
#endif /* SUPPORTS_UTF8 */

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function	    GetAncestorProcessName				      |
|									      |
|   Description     Get the process ID and name of an ancestor process 	      |
|									      |
|   Notes	    							      |
|									      |
|   History								      |
|    2022-02-01 JFL Initial implementattion, based on GetProcessName().	      |
*									      *
\*---------------------------------------------------------------------------*/

#if defined(_WIN32)
#include <windows.h>
#pragma pack(push,8) /* Work around a bug in tlhelp32.h in WIN64, which generates the wrong structure if packing has been changed */
#include <tlhelp32.h>
#pragma pack(pop)

int GetAncestorProcessName(pid_t pid, int iLevel, char *name, size_t lname) {
  size_t len = 0;
  HANDLE h;
  BOOL bFound;
  PROCESSENTRY32 pe = {0};
  int i;

  if (!pid) pid = getpid();

  h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (h == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to get list of processes\n");
    return 0;
  }

  pe.dwSize = sizeof(PROCESSENTRY32);
  for (i=0; i<=iLevel; i++) {
    pid_t ppid = (pid_t)0;
    for (bFound=Process32First(h, &pe); bFound; bFound=Process32Next(h, &pe)) {
      if ((pid_t)(pe.th32ProcessID) == pid) {
      	/* printf("PID %d = %s\n", (int)pid, pe.szExeFile); */
      	ppid = (pid_t)(pe.th32ParentProcessID);
	break;
      }
    }
    if (!ppid) return 0;
    pid = ppid;
  }

  len = strlen(pe.szExeFile);
  if (lname <= len) return -(int)len; /* Not enough room in the output buffer */
  strcpy(name, pe.szExeFile);

  CloseHandle(h);

  return (int)pe.th32ProcessID;
}

#endif

