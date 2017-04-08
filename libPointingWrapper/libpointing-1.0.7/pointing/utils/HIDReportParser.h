/* -*- mode: c++ -*-
 *
 * pointing/utils/HIDReportParser.h --
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

#ifndef HID_REPORT_PARSER_H
#define HID_REPORT_PARSER_H

#include <pointing/utils/HIDItem.h>
#include <list>
#include <iostream>
#include <map>

namespace pointing
{
  /**
   * @brief The HIDReportParser class is very specific HID Report parser
   * which attempts to find only button and relative mouse displacement positions
   * in a given HID descriptor and according to that descriptor outputs the found
   * values in given HID reports
   */
  class HIDReportParser
  {
    struct MouseReport
    {
      int reportId;
      int size;
      int dxPos;
      int dyPos;

      // dMask is mask that can read bits which do not correspond to entire bytes
      // for example: for 12 bits this will be 0xFFF
      int dMask;
      int min;
      int max;
      int buttonsPos;

      MouseReport():reportId(0),size(0),dxPos(0),dyPos(0),dMask(0),min(0),max(0),buttonsPos(-1) {}
    };

    unsigned int lastUsage, parentUsage, lastUsagePage;
    unsigned int lastRepCount, lastRepSize; // If one of them is not present, we pick the last available
    std::map<int, MouseReport> reportMap;
    std::map<int, int> dataMap;
    std::list<int> usageList;

    MouseReport *curRepInfo;
    unsigned char *report;

    int debugLevel;

    void parseItem(const HIDItem &item);

    // Find the report which contains relative X and Y
    bool findCorrectReport();

  public:
    HIDReportParser();
    HIDReportParser(unsigned char *desc, int size, int debugLevel=0);

    HIDReportParser(const HIDReportParser &);
    HIDReportParser& operator=(const HIDReportParser &);

    ~HIDReportParser();

    /**
     * @brief Clears all data, so the parser can't parse anymore
     */
    void clearDescriptor();

    /**
     * @brief setDescriptor Sets the HID descriptor, parses it.
     * @param desc
     * @param size
     * @return True of correctly parsed False otherwise
     */
    bool setDescriptor(const unsigned char *desc, int size);

    /**
     * @brief setReport Set any input report.
     * @param report
     * @return If reportId corresponds to the reportId containing dX and dY
     */
    bool setReport(const unsigned char *report);

    /**
     * @brief getReportLength
     * @return The length of the Mouse Report in bytes deduced from the report descriptor
     */
    int getReportLength() const;

    /**
     * @brief getDxDy Gives the relative X and Y in mouse points
     * @param dx Relative X
     * @param dy Relative Y
     * @return True if read successfully False otherwise
     */
    bool getReportData(int *dx, int *dy, int *buttons) const;
  };
}
#endif // HID_REPORT_PARSER_H
