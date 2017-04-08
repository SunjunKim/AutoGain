/* -*- mode: c++ -*-
 *
 * pointing/utils/ConfigDict.cpp --
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

#include <pointing/utils/ConfigDict.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace pointing {

  const std::string ConfigDict::defkvpairseps ="\r\n" ;
  const std::string ConfigDict::defkvseps = ":" ;
  const std::string ConfigDict::defcomment = "#" ;
  const std::string ConfigDict::defkvpairsep ="\n" ;
  const std::string ConfigDict::defkvsep = ": " ;

  static std::string &
  trimString(std::string &s, std::string seps=" \t\n\r") {
    std::string::size_type end = s.find_last_not_of(seps) ;
    std::string::size_type begin = s.find_first_not_of(seps) ;
    if (end==std::string::npos && begin==std::string::npos)
      s.clear() ;
    else {
      if (end!=std::string::npos) s.resize(end+1) ;
      if (begin!=std::string::npos) s.erase(0,begin) ;
    }
    return s ;
  }

  static std::string
  pullWord(std::string &s, std::string seps=" \t\n\r") {
    std::string result ;
    std::string::size_type i ;

    i = s.find_first_not_of(seps, 0) ;
    if (i>0) s.erase(0,i) ;

    i = s.find_first_of(seps, 0) ;
    result.assign(s, 0, i) ;
  
    i = s.find_first_not_of(seps, i) ;
    s.erase(0,i) ;
  
    return result ;
  }

  static std::map<std::string, std::string>
  str2strmap(std::string kvpairs,
	     std::string kvpairseps, std::string kvseps,
	     std::string commentmarker) {
    std::map<std::string, std::string> result ;
    for (std::string kvpair=pullWord(kvpairs, kvpairseps);
	 !kvpair.empty();
	 kvpair=pullWord(kvpairs, kvpairseps)) {
      std::string key = pullWord(kvpair, kvseps) ;
      if (commentmarker.empty())
	result[trimString(key)] = trimString(kvpair) ;
      else {
	std::string::size_type cm = key.find(commentmarker) ;
	if (cm==0) {
	  // ignore
	} else if (cm!=std::string::npos) {
	  key.resize(cm) ;
	  result[trimString(key)] = "true" ;
	} else {
	  cm = kvpair.find(commentmarker) ;
	  if (cm!=std::string::npos) kvpair.resize(cm) ;
	  std::string value = trimString(kvpair) ;
	  if (value.empty()) value = "true" ;
	  result[trimString(key)] = value ;	
	}
      }	  
    }
    return result ;
  }

  bool
  ConfigDict::load(std::string data,
		   std::string kvpairseps, std::string kvseps,
		   std::string commentmarker) {
    dict = str2strmap(data, kvpairseps, kvseps, commentmarker) ;
    return true ;
  }

  bool
  ConfigDict::loadFrom(std::string filename,
		       std::string kvpairseps, std::string kvseps,
		       std::string commentmarker) {
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary) ;
    if (!in) return false ;

    std::string contents ;
    in.seekg(0, std::ios::end) ;
    contents.resize(in.tellg()) ;
    in.seekg(0, std::ios::beg) ;
    in.read(&contents[0], contents.size()) ;
    in.close() ;

    return load(contents, kvpairseps, kvseps, commentmarker) ;
  }

  bool
  ConfigDict::str2bool(const std::string &str) const {
    std::string tmp = str ;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower) ;
    if (tmp=="true" || tmp=="yes" || tmp=="1") return true ;
    return false ;
  }

  std::string
  ConfigDict::setdefault(const std::string &key, const char *defval) {
    return setdefault(key, std::string(defval)) ;
  }

  bool
  ConfigDict::get(const std::string &key) const {
    return str2bool(get<std::string>(key)) ;
  }

  bool
  ConfigDict::get(const char *key) const {
    return str2bool(get<std::string>(std::string(key))) ;
  }
  
  std::string
  ConfigDict::dump(std::string kvpairsep, std::string kvsep) const {
    std::stringstream out ;
    for (const_iterator i=dict.begin(); i!=dict.end();) {
      out << (*i).first << kvsep << (*i).second ;
      i++ ;
      if (i!=dict.end()) out << kvpairsep ;
    }
    return out.str() ;
  }

  void
  ConfigDict::saveAs(std::string filename, std::string kvpairsep, std::string kvsep) {
    std::ofstream f ;
    f.open(filename.c_str()) ;
    f << dump(kvpairsep, kvsep) ;
    f.close();
  }

  void
  ConfigDict::exportToJSON(std::string filename) const {
    std::ofstream f ;
    f.open(filename.c_str()) ;
    f << "{" << std::endl ;
    for (const_iterator i=dict.begin(); i!=dict.end();) {
      f << "   \"" << (*i).first << "\": \"" << (*i).second << "\"" ;
      i++ ;
      if (i!=dict.end()) f << "," ;
      f << std::endl ;
    }
    f << "}" << std::endl ;
    f.close();
  }

}
