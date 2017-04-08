/* -*- mode: c++ -*-
 *
 * pointing/utils/FileUtils.h --
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

#ifndef FileUtils_h
#define FileUtils_h

#include <stdint.h>
#include <string>

namespace pointing {

  bool fileExists(const char *filename) ;

  uint64_t getFileSize(const char *filename) ;

  void readFromFile(const char *filename, char *data, unsigned int size) ;

  std::string moduleHeadersPath();
}

#endif
