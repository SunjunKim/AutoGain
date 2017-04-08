/* -*- mode: c++ -*-
 *
 * pointing/utils/ByteOrder.h --
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

#ifndef ByteOrder_h
#define ByteOrder_h

#include <stdint.h>

namespace pointing {

  bool isLittleEndian(void) ;

  uint16_t swap16(uint16_t arg) ;
  uint32_t swap32(uint32_t arg) ;
  uint64_t swap164(uint64_t arg) ;

  uint16_t swap16ifle(uint16_t arg) ;
  uint32_t swap32ifle(uint32_t arg) ;
  uint64_t swap64ifle(uint64_t arg) ;

  uint16_t swap16ifbe(uint16_t arg) ;
  uint32_t swap32ifbe(uint32_t arg) ;
  uint64_t swap64ifbe(uint64_t arg) ;

}

#endif
