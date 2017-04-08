/* -*- mode: c++ -*-
 *
 * pointing/utils/HIDItem.h --
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

#ifndef HID_ITEM_H
#define HID_ITEM_H

namespace pointing {

    class HIDItem
    {
    public:
        HIDItem(const unsigned char *);
        HIDItem(const HIDItem &);
        ~HIDItem();

        unsigned char tag() const;
        unsigned char dataSize() const;
        const unsigned char* data() const;
        long dataAsSignedLong() const;
        unsigned long dataAsUnsignedLong() const;
        unsigned int totalSize() const;

        enum ItemTypes { MAIN = 0, GLOBAL, LOCAL, RESERVED };
        ItemTypes type() const;

        /* good for looking up in HIDTags.h
         * type is the lower 2 bits */
        unsigned int typeAndTag() const;

        bool isLong() const;

    protected:
        unsigned char* rawData;
        void setRawDataFrom(const unsigned char* inputArray);
        static unsigned char bits(const unsigned char input, const unsigned char from, const unsigned char to);
        static unsigned char dataSizeForByteArray(const unsigned char* array);
        static unsigned int totalSizeForByteArray(const unsigned char* array);
        static bool isLongForByteArray(const unsigned char* array);

    private:
        HIDItem& operator=(const HIDItem &); /* Items are immutable so you shouldn't
                                          be trying to use this operator */
    };

}

#endif // HID_ITEM_H
