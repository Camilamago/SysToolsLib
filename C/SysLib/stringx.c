﻿/*****************************************************************************\
*                                                                             *
*   Filename:	    stringx.c						      *
*									      *
*   Description:    Case-insensitive string management routines		      *
*                                                                             *
*   Notes:	    							      *
*									      *
*   History:								      *
*    2012-02-02 JFL Created this file.					      *
*    2016-04-12 JFL Include MultiOS.h.					      *
*    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
*    2023-02-17 JFL Renamed stristr as strcasestr, and moved it to MsvcLibX.  *
*									      *
*         © Copyright 2016 Hewlett Packard Enterprise Development LP          *
* Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 *
\*****************************************************************************/

#include <stringx.h>

/*---------------------------------------------------------------------------*\
*                                                                             *
|   Function:	    strnirepl						      |
|									      |
|   Description:    Case-insensitive string replacement			      |
|									      |
|   Parameters:     char *pszResultBuffer   Output buffer       	      |
|		    size_t lResultBuffer    Output buffer size  	      |
|		    const char *pszString   Source string to copy from	      |
|		    const char *pszSearch   String to search in pszString     |
|		    const char *pszReplace  String to write instead	      |
|									      |
|   Returns:	    The length of the output string.			      |
|									      |
|   History:								      |
|    2012-02-02 JFL Created this routine.				      |
*									      *
\*---------------------------------------------------------------------------*/

size_t strnirepl(char *pszResultBuffer, size_t lResultBuffer, const char *pszString,
                 const char *pszSearch, const char *pszReplace) {
  size_t lSearch = strlen(pszSearch);
  size_t lReplace = strlen(pszReplace);
  size_t lRemain = lResultBuffer;
  char *pszIn = pszString;
  char *pszOut = pszResultBuffer;
  if (!lRemain) return 0; /* Prevent catastrophies */
  lRemain -= 1; /* Leave room for the final NUL */
  while (*pszIn && lRemain) {
    if (strnieq(pszIn, pszSearch, lSearch)) {
      strncpy(pszOut, pszReplace, lRemain);
      pszIn += lSearch;
      if (lReplace < lRemain) {
	pszOut += lReplace;
      	lRemain -= lReplace;
      } else {
	pszOut += lRemain;
      	lRemain = 0;
      }
      continue;
    }
    *(pszOut++) = *(pszIn++);
    lRemain -= 1;
  }
  *pszOut = '\0';
  return pszOut - pszResultBuffer;
}

