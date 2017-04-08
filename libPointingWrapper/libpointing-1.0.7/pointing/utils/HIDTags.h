/* -*- mode: c++ -*-
 *
 * pointing/utils/HIDTags.h --
 *
 * Authors: Izzatbek Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

/*
 * Based on:
 * http://technofetish.net/repos/buffaloplay/HIDParser/
 */

#ifndef HID_TAGS_H
#define HID_TAGS_H

// MAIN tags

#define INPUT             0x20
#define OUTPUT            0x24
#define FEATURE           0x2C
#define COLLECTION        0x28
#define END_COLLECTION    0x30

// GLOBAL tags
#define USAGE_PAGE        0x01
#define LOGICAL_MIN       0x05
#define LOGICAL_MAX       0x09
#define PHYSICAL_MIN      0x0D
#define PHYSICAL_MAX      0x11
#define UNIT_EXPONENT     0x15
#define UNIT              0x19
#define REPORT_SIZE       0x1D
#define REPORT_ID         0x21
#define REPORT_COUNT      0x25
#define PUSH              0x29
#define POP               0x2D

// LOCAL tags

#define USAGE             0x02
#define USAGE_MIN         0x06
#define USAGE_MAX         0x0A
#define DESIGNATOR_INDEX  0x0E
#define DESIGNATOR_MIN    0x12
#define DESIGNATOR_MAX    0x16
#define STRING_INDEX      0x1E
#define STRING_MIN        0x22
#define STRING_MAX        0x26
#define DELIMITER         0x2A

// USAGE values
#define USAGE_POINTER           0x01
#define USAGE_MOUSE             0x02
#define USAGE_X                 0x30
#define USAGE_Y                 0x31

// USAGE_PAGE values
#define USAGE_PAGE_BUTTONS      0x09
#define USAGE_PAGE_GEN_DESKTOP  0x01

// INPUT values
#define INPUT_CONSTANT          0x01

#endif // HID_TAGS_H
