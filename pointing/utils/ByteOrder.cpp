/* -*- mode: c++ -*-
 *
 * pointing/utils/ByteOrder.cpp --
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

#include <pointing/utils/ByteOrder.h>

#include <iostream>

namespace pointing {

  // --------------------------------------------------------

  bool isLittleEndian(void) {
    static uint32_t littleEndianTest = 1 ;
    return (*(char *)&littleEndianTest == 1) ;
  }

  // --------------------------------------------------------

  uint16_t swap16(uint16_t arg) {
    return ((((arg) & 0xff) << 8) | (((arg) >> 8) & 0xff)) ;
  }

  uint16_t swap16ifle(uint16_t arg) {
    return isLittleEndian() ? swap16(arg) : arg ;
  }

  uint16_t swap16ifbe(uint16_t arg) {
    return isLittleEndian() ? arg : swap16(arg) ;
  }

  // --------------------------------------------------------

  uint32_t swap32(uint32_t arg) {
    return ((((arg) & 0xff000000) >> 24) | \
	    (((arg) & 0x00ff0000) >> 8)  | \
	    (((arg) & 0x0000ff00) << 8)  | \
	    (((arg) & 0x000000ff) << 24)) ;
  }

  uint32_t swap32ifle(uint32_t arg) {
    return isLittleEndian() ? swap32(arg) : arg ;
    return arg ;
  }

  uint32_t swap32ifbe(uint32_t arg) {
    return isLittleEndian() ? arg : swap32(arg) ;
  }

  // --------------------------------------------------------

  uint64_t swap64(uint64_t arg) {
    return ((((arg) & 0xff00000000000000ull) >> 56) \
	    | (((arg) & 0x00ff000000000000ull) >> 40) \
	    | (((arg) & 0x0000ff0000000000ull) >> 24) \
	    | (((arg) & 0x000000ff00000000ull) >> 8) \
	    | (((arg) & 0x00000000ff000000ull) << 8) \
	    | (((arg) & 0x0000000000ff0000ull) << 24) \
	    | (((arg) & 0x000000000000ff00ull) << 40) \
	    | (((arg) & 0x00000000000000ffull) << 56)) ;
  }

  uint64_t swap64ifle(uint64_t arg) {
    return isLittleEndian() ? swap64(arg) : arg ;
  }

  uint64_t swap64ifbe(uint64_t arg) {
    return isLittleEndian() ? arg : swap64(arg) ;
  }

  // --------------------------------------------------------

}
