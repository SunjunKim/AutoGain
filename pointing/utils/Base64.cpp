/* -*- mode: c++ -*-
 *
 * pointing/utils/Base64.cpp --
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

#include <pointing/utils/Base64.h>

#include <iostream>
#include <stdexcept>

namespace pointing {

  std::string
  Base64::encode(std::string input) {
    std::string result ;

    unsigned char dtable[256] ;
    for(int i= 0;i<9;i++){
      dtable[i]= 'A'+i;
      dtable[i+9]= 'J'+i;
      dtable[26+i]= 'a'+i;
      dtable[26+i+9]= 'j'+i;
    }
    for(int i= 0;i<8;i++){
      dtable[i+18]= 'S'+i;
      dtable[26+i+18]= 's'+i;
    }
    for(int i= 0;i<10;i++) dtable[52+i]= '0'+i;
    dtable[62]= '+';
    dtable[63]= '/';

    unsigned long length = input.length() ;
    for (unsigned int iInput=0; iInput<length;) {
      unsigned char igroup[3],ogroup[4];
      igroup[0]= igroup[1]= igroup[2]= 0;
      int n = 0 ;
      while (n<3 && iInput<length)
	igroup[n++]= (unsigned char)input[iInput++] ;
      if(n> 0){
	ogroup[0]= dtable[igroup[0]>>2];
	ogroup[1]= dtable[((igroup[0]&3)<<4)|(igroup[1]>>4)];
	ogroup[2]= dtable[((igroup[1]&0xF)<<2)|(igroup[2]>>6)];
	ogroup[3]= dtable[igroup[2]&0x3F];	 
	if(n<3){
	  ogroup[3]= '=';
	  if(n<2) ogroup[2]= '=';
	}
	for(int i= 0;i<4;i++) result = result + (char)ogroup[i] ;
      }
    }
  
    return result ;
  }

  std::string
  Base64::decode(std::string input) {
    std::string result ;

    unsigned char dtable[256] ;
    for(int i= 0;i<255;i++) dtable[i]= 0x80;
    for(int i= 'A';i<='I';i++) dtable[i]= 0+(i-'A');
    for(int i= 'J';i<='R';i++) dtable[i]= 9+(i-'J');
    for(int i= 'S';i<='Z';i++) dtable[i]= 18+(i-'S');
    for(int i= 'a';i<='i';i++) dtable[i]= 26+(i-'a');
    for(int i= 'j';i<='r';i++) dtable[i]= 35+(i-'j');
    for(int i= 's';i<='z';i++) dtable[i]= 44+(i-'s');
    for(int i= '0';i<='9';i++) dtable[i]= 52+(i-'0');
    dtable[(int)'+']= 62;
    dtable[(int)'/']= 63;
    dtable[(int)'=']= 0;

    unsigned long length = input.length() ;
    for (unsigned int iInput=0 ;;) {
      unsigned char a[4],b[4],o[3];
    
      for(int i= 0;i<4;i++){
	if (iInput==length) {
	  // Incomplete input
	  return result ;
	}
	int c = (int)input[iInput++] ;
	if(dtable[c]&0x80){
	  i--; // Illegal character
	  continue;
	}
	a[i]= (unsigned char)c;
	b[i]= (unsigned char)dtable[c];
      }

      o[0]= (b[0]<<2)|(b[1]>>4);
      o[1]= (b[1]<<4)|(b[2]>>2);
      o[2]= (b[2]<<6)|b[3];

      int i= a[2]=='='?1:(a[3]=='='?2:3);
      result.append((char*)o,i) ;
      if(i<3) return result ;
    }

    return result ;
  }

}
