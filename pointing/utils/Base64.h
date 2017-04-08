/* -*- mode: c++ -*-
 *
 * pointing/utils/Base64.h --
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

#ifndef Base64_h
#define Base64_h

#include <string>

namespace pointing {

  namespace Base64 {
    std::string encode(std::string input) ;
    std::string decode(std::string input) ;
  }

}

#endif
