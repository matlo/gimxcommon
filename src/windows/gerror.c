/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include <windows.h>
#include <stdio.h>

static char * utf16le_to_utf8(const wchar_t * inbuf)
{
  char * outbuf = NULL;
  int outsize = WideCharToMultiByte(CP_UTF8, 0, inbuf, -1, NULL, 0, NULL, NULL);
  if (outsize != 0) {
      outbuf = (char*) malloc(outsize * sizeof(*outbuf));
      if (outbuf != NULL) {
         int res = WideCharToMultiByte(CP_UTF8, 0, inbuf, -1, outbuf, outsize, NULL, NULL);
         if (res == 0) {
             free(outbuf);
             outbuf = NULL;
         }
      }
  }

  return outbuf;
}

/*
 * This is the Windows equivalent for perror.
 */
void gerror_print_last(const char * msg) {

  DWORD error = GetLastError();
  LPWSTR pBuffer = NULL;

  if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL, error, 0, (LPWSTR) & pBuffer, 0, NULL)) {
    fprintf(stderr, "%s: code = %lu\n", msg, error);
  } else {
    char * message = utf16le_to_utf8(pBuffer);
    fprintf(stderr, "%s: %s\n", msg, message);
    free(message);
    LocalFree(pBuffer);
  }
}
