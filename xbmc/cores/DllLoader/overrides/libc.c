/*
 *      Copyright (C) 2007-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include <limits.h>

/* The names of libc libraries for different systems */
#if defined(_WIN32)
#  define C_LIB "msvcr90.dll"
#elif defined(__APPLE__)
#  define C_LIB "libc.dylib"
#else
#  define C_LIB "libc.so.6"
#endif

/* Global dlopen handle */
void *dlopen_handle;

/* Global dlsym handles */
char *(*libc_getcwd)(char*,size_t);
int (*libc_chdir)(const char*);

/* Function prototypes of overridden libc functions */
char *getcwd(char *buf, size_t size);
int chdir(const char *path);

/* Function to initialize dlopen and dlsym handles. It should be called before
 * using any overridden libc function.
 */
void xbmc_libc_init(void)
{
  dlopen_handle = dlopen(C_LIB, RTLD_LOCAL | RTLD_LAZY);
  libc_getcwd = dlsym(dlopen_handle, "getcwd");
  libc_chdir = dlsym(dlopen_handle, "chdir");
}

#ifdef __APPLE__
/* Use pthread's built-in support for TLS, it's more portable. */
static pthread_once_t keyOnce = PTHREAD_ONCE_INIT;
static pthread_key_t  tWorkingDir = 0;

/* Called once and only once. */
static void MakeTlsKeys()
{
  pthread_key_create(&tWorkingDir, free);
}

#define xbp_cw_dir (char*)pthread_getspecific(tWorkingDir)

#else
__thread char xbp_cw_dir[PATH_MAX] = "";
#endif

/* Overridden functions */
char *getcwd(char *buf, size_t size)
{
  if (buf == NULL) buf = (char *)malloc(size);
  strcpy(buf, xbp_cw_dir);
  return buf;
}

int chdir(const char *dirname)
{
  if (strlen(dirname) > PATH_MAX) return -1;
  strcpy(xbp_cw_dir, dirname);
  return 0;
}
