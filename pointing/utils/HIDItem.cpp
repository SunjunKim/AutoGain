/* -*- mode: c++ -*-
 *
 * pointing/utils/HIDItem.cpp --
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

#include <pointing/utils/HIDItem.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <limits>

namespace pointing
{
    HIDItem::HIDItem(const unsigned char * inputArray)
    {
        setRawDataFrom(inputArray);
    }

    HIDItem::HIDItem(const HIDItem & copySource)
    {
        setRawDataFrom(copySource.rawData);
    }

    void HIDItem::setRawDataFrom(const unsigned char* inputArray)
    {
        unsigned int size = totalSizeForByteArray(inputArray);
        if(size > 258) throw std::runtime_error("Item size is very large") ;;
        rawData = static_cast<unsigned char*>(malloc(size));
        if(!rawData) throw std::runtime_error("Item size is very large") ;;
        memcpy(rawData, inputArray, size);
    }

    HIDItem::~HIDItem()
    {
        free(rawData);
    }

    unsigned char HIDItem::bits(const unsigned char input, const unsigned char from, const unsigned char to)
    {
        unsigned char result = input << (7 - to);
        result = result >> (7 - to + from);
        return result;
    }

    unsigned char HIDItem::tag() const
    {
        if(isLong())
            return rawData[2];
        return bits(rawData[0], 4, 7);
    }

    unsigned char HIDItem::dataSizeForByteArray(const unsigned char * array)
    {
        if(isLongForByteArray(array))
            return array[1];
        unsigned char rawSize = bits(array[0], 0, 1);
        if(rawSize == 3)
            return 4;
        return rawSize;
    }

    unsigned int HIDItem::totalSizeForByteArray(const unsigned char * array)
    {
        unsigned char dataSize = dataSizeForByteArray(array);
        if(isLongForByteArray(array))
            return dataSize + 3;
        return dataSize + 1;
    }

    unsigned int HIDItem::totalSize() const
    {
        return totalSizeForByteArray(rawData);
    }

    unsigned int HIDItem::typeAndTag() const
    {
        int result = tag() << 2;
        return result | type();
    }

    bool HIDItem::isLongForByteArray(const unsigned char * array)
    {
        return array[0] == 0xFE;
    }

    bool HIDItem::isLong() const
    {
        return isLongForByteArray(rawData);
    }

    unsigned char HIDItem::dataSize() const
    {
        return dataSizeForByteArray(rawData);
    }

    const unsigned char* HIDItem::data() const
    {
        if(isLong())
            return rawData + 3;
        return rawData + 1;
    }

    long HIDItem::dataAsSignedLong() const
    {
        int maxSize = 4; //only 4 bits at max
        int size = maxSize > dataSize() ? dataSize() : maxSize;
        long result;
        if(data()[size - 1] & 0x80)
        {
            //the high bit is set - therefore this number is negative
            result = -1; //all 0xFFs
        } else {
            result = 0;
        }
        for(int i = 0; i < size; i++)
        {
            result = (result << 8) + data()[size - i - 1];
        }
        return result;
    }

    unsigned long HIDItem::dataAsUnsignedLong() const
    {
        unsigned long result = 0;
        int maxSize = 4; //only 4 bits at max
        int size = maxSize > dataSize() ? dataSize() : maxSize;
        for(int i = 0; i < size; i++)
        {
            result = (result << 8) + data()[size - i - 1];
        }
        return result;
    }

    HIDItem::ItemTypes HIDItem::type() const
    {
        return static_cast<ItemTypes>(bits(rawData[0], 2, 3));
    }
}

