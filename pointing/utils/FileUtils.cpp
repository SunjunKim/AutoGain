/* -*- mode: c++ -*-
 *
 * pointing/utils/FileUtils.cpp --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/utils/FileUtils.h>

#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <errno.h>

#ifndef _MSC_VER // Not include it on Windows
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>

#ifdef __APPLE__
#include <sys/syslimits.h>
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#define SLASH '/'
#endif

#ifdef __linux__
#include <limits.h>
#endif

#ifdef _WIN32
#include <windows.h>
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define SLASH '\\'
#ifndef PATH_MAX
#define PATH_MAX 256
#endif
#endif

namespace pointing {

  bool
  fileExists(const char *filename) {
#ifndef _MSC_VER // Visual Studio C++
    struct stat statinfo ;
    if (stat(filename, &statinfo)==-1) return false ;
    return true ;
#else
#pragma warning(disable:4100)
    std::cerr << "fileExists (FileUtils) is not implemented on this platform" << std::endl ;
    return false ;
#endif
  }

  uint64_t
  getFileSize(const char *filename) {
#ifndef _MSC_VER // Visual Studio C++
    struct stat statinfo ;
    if (stat(filename, &statinfo)==-1) return 0 ;
    return statinfo.st_size ;
#else
    std::cerr << "getFileSize (FileUtils) is not implemented on this platform" << std::endl ;
    return 0 ;
#endif
  }

  void
  readFromFile(const char *filename, char *data, unsigned int size) {
#ifndef _MSC_VER // Visual Studio C++
    int fd = open(filename, O_RDONLY) ;
    if (fd==-1) {
      std::string msg("can't open ") ;
      msg.append(filename) ;
      msg.append(" (readFromFile)") ;
      throw std::runtime_error(msg) ;
    }
    if (read(fd, data, size)!=(int)size) {
      std::string msg("can't read from ") ;
      msg.append(filename) ;
      msg.append(" (readFromFile)") ;
      throw std::runtime_error(msg) ;
    }
    close(fd) ;
#else
    std::cerr << "FIXME: readFromFile (FileUtils) is not implemented on this platform" << std::endl ;
#endif
  }

  bool pointingDirExists(const char *path)
  {
    const char pointingDir[] = "/pointing-echomouse";
    char tmp[PATH_MAX];
    sprintf(tmp, "%s%s", path, pointingDir);
    struct stat sb;
    return stat(tmp, &sb) == 0 && S_ISDIR(sb.st_mode);
  }

  bool getModulePath(char *path)
  {
#ifdef _WIN32
    HMODULE hm = NULL;

    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR) &moduleHeadersPath,
            &hm))
    {
      std::cerr << "GetModuleHandle returned " << GetLastError() << std::endl;
      return false;
    }
    if (!GetModuleFileNameA(hm, path, PATH_MAX))
    {
      std::cerr << "GetModuleFileNameA returned " << GetLastError() << std::endl;
      return false;
    }
    // Remove \\?\ from the beginning on VMWare
    if (strncmp(path, "\\\\?\\", 4) == 0)
      strcpy(path, path + 4);
    return true;
#else
    Dl_info info;
    if (dladdr((void *)moduleHeadersPath, &info))
    {
      // If found path is relative
      if (info.dli_fname[0] != '/')
      {
        char *real_path = realpath(info.dli_fname, NULL);
        if (real_path)
        {
          strcpy(path, real_path);
          free(real_path);
        }
        else
        {
          printf ("realpath function failed with error: %s\n", strerror(errno));
          return false;
        }
      }
      else
        strcpy(path, info.dli_fname);
      return true;
    }
    return false;
#endif
  }

  std::string moduleHeadersPath()
  {
    char path[PATH_MAX];
    if (getModulePath(path))
    {
      for (int i = 0; i < 10; i++) // Check 10 levels max
      {
        char *lastSlash = strrchr(path, SLASH);
        if (!lastSlash)
          break;

        sprintf(lastSlash, "%s", "/include");
        if (pointingDirExists(path))
          return std::string(path);

        // Cut the path (go to path/..)
        *lastSlash = 0;
        if (pointingDirExists(path))
          return std::string(path);
      }
    }
    return "";
  }
}
