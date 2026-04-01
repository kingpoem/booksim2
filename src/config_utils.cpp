// $Id$
/*
 Copyright (c) 2007-2015, Trustees of The Leland Stanford Junior University
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this 
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*config_utils.cpp
 *
 *The configuration object which contained the parsed data from the 
 *configuration file
 */

#include "booksim.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <iterator>

#include "config_utils.hpp"

Configuration *Configuration::theConfig = 0;

Configuration::Configuration()
{
  theConfig = this;
  _config_file = 0;
}

void Configuration::AddStrField(string const & field, string const & value)
{
  _str_map[field] = value;
}

void Configuration::Assign(string const & field, string const & value)
{
  map<string, string>::const_iterator match;
  
  match = _str_map.find(field);
  if(match != _str_map.end()) {
    _str_map[field] = value;
  } else {
    ParseError("Unknown string field: " + field);
  }
}

void Configuration::Assign(string const & field, int value)
{
  map<string, int>::const_iterator match;
  
  match = _int_map.find(field);
  if(match != _int_map.end()) {
    _int_map[field] = value;
  } else {
    ParseError("Unknown integer field: " + field);
  }
}

void Configuration::Assign(string const & field, double value)
{
  map<string, double>::const_iterator match;
  
  match = _float_map.find(field);
  if(match != _float_map.end()) {
    _float_map[field] = value;
  } else {
    ParseError("Unknown double field: " + field);
  }
}

string Configuration::GetStr(string const & field) const
{
  map<string, string>::const_iterator match;

  match = _str_map.find(field);
  if(match != _str_map.end()) {
    return match->second;
  } else {
    ParseError("Unknown string field: " + field);
    exit(-1);
  }
}

int Configuration::GetInt(string const & field) const
{
  map<string, int>::const_iterator match;

  match = _int_map.find(field);
  if(match != _int_map.end()) {
    return match->second;
  } else {
    ParseError("Unknown integer field: " + field);
    exit(-1);
  }
}

double Configuration::GetFloat(string const & field) const
{  
  map<string,double>::const_iterator match;

  match = _float_map.find(field);
  if(match != _float_map.end()) {
    return match->second;
  } else {
    ParseError("Unknown double field: " + field);
    exit(-1);
  }
}

vector<string> Configuration::GetStrArray(string const & field) const
{
  string const param_str = GetStr(field);
  return tokenize_str(param_str);
}

vector<int> Configuration::GetIntArray(string const & field) const
{
  string const param_str = GetStr(field);
  return tokenize_int(param_str);
}

vector<double> Configuration::GetFloatArray(string const & field) const
{
  string const param_str = GetStr(field);
  return tokenize_float(param_str);
}

void Configuration::ParseFile(string const & filename)
{
  ifstream in(filename.c_str(), ios::in | ios::binary);
  if(!in.is_open()) {
    cerr << "Could not open configuration file " << filename << endl;
    exit(-1);
  }
  const string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  ParseConfigContent(content, this, 1);
}

void Configuration::ParseString(string const & str)
{
  ParseConfigContent(str + ';', this, 1);
}

void Configuration::ParseJsonFile(string const & filename)
{
  ifstream f(filename);
  if (!f.is_open()) {
    ParseError("Cannot open JSON file: " + filename);
    return;
  }

  try {
    _json_config = json::parse(f);
    f.close();

    // 1. 解析全局配置到现有的三个 map
    // 优先检查是否已注册（通过 AddStrField 等预先注册），如果未注册但有值，则添加到对应 map
    for (auto& item : _json_config.items()) {
      string key = item.key();
      if (key == "routers") continue; // 跳过路由器特定配置

      auto& value = item.value();
      if (value.is_string()) {
        // 如果已注册为字符串字段，则更新；否则如果是新字段，也添加到 map
        _str_map[key] = value.get<string>();
      } else if (value.is_number_integer()) {
        // 如果已注册为整数字段，则更新；否则如果是新字段，也添加到 map
        _int_map[key] = value.get<int>();
      } else if (value.is_number_float()) {
        // 如果已注册为浮点数字段，则更新；否则如果是新字段，也添加到 map
        _float_map[key] = value.get<double>();
      }
    }

    // 2. 解析路由器特定配置
    if (_json_config.contains("routers") && _json_config["routers"].is_object()) {
      for (auto& router_item : _json_config["routers"].items()) {
        string router_id_str = router_item.key();
        int router_id = stoi(router_id_str);
        auto& router_config = router_item.value();

        for (auto& param_item : router_config.items()) {
          string key = param_item.key();
          auto& value = param_item.value();

          if (value.is_string()) {
            _router_specific_str_map[router_id][key] = value.get<string>();
          } else if (value.is_number_integer()) {
            _router_specific_int_map[router_id][key] = value.get<int>();
          } else if (value.is_number_float()) {
            _router_specific_float_map[router_id][key] = value.get<double>();
          }
        }
      }
    }
  } catch (json::exception& e) {
    ParseError(string("JSON parse error: ") + e.what());
  } catch (...) {
    ParseError("Unknown error parsing JSON file: " + filename);
  }
}

string Configuration::GetRouterStr(int router_id, string const & field) const
{
  // 优先返回路由器特定配置
  auto router_it = _router_specific_str_map.find(router_id);
  if (router_it != _router_specific_str_map.end()) {
    auto param_it = router_it->second.find(field);
    if (param_it != router_it->second.end()) {
      return param_it->second;
    }
  }
  // 回退到全局配置
  return GetStr(field);
}

int Configuration::GetRouterInt(int router_id, string const & field) const
{
  // 优先返回路由器特定配置
  auto router_it = _router_specific_int_map.find(router_id);
  if (router_it != _router_specific_int_map.end()) {
    auto param_it = router_it->second.find(field);
    if (param_it != router_it->second.end()) {
      return param_it->second;
    }
  }
  // 回退到全局配置
  return GetInt(field);
}

double Configuration::GetRouterFloat(int router_id, string const & field) const
{
  // 优先返回路由器特定配置
  auto router_it = _router_specific_float_map.find(router_id);
  if (router_it != _router_specific_float_map.end()) {
    auto param_it = router_it->second.find(field);
    if (param_it != router_it->second.end()) {
      return param_it->second;
    }
  }
  // 回退到全局配置
  return GetFloat(field);
}

int Configuration::Input(char * line, int max_size)
{
  int length = 0;

  if(_config_file) {
    length = fread(line, 1, max_size, _config_file);
  } else {
    length = _config_string.length();
    _config_string.copy(line, max_size);
    _config_string.clear();
  }

  return length;
}

void Configuration::ParseError(string const & msg, unsigned int lineno) const
{
  if(lineno) {
    cerr << "Parse error on line " << lineno << " : " << msg << endl;
  } else {
    cerr << "Parse error : " << msg << endl;
  }


  exit( -1 );
}

Configuration * Configuration::GetTheConfig()
{
  return theConfig;
}

//============================================================

bool ParseArgs(Configuration * cf, int argc, char * * argv)
{
  bool rc = false;

  //all dashed variables are ignored by the arg parser
  for(int i = 1; i < argc; ++i) {
    string arg(argv[i]);
    size_t pos = arg.find('=');
    bool dash = (argv[i][0] =='-');
    if(pos == string::npos && !dash) {
      // 检测文件扩展名
      if (arg.find(".json") != string::npos) {
        // JSON 文件
        cf->ParseJsonFile(argv[i]);
        ifstream in(argv[i]);
        cout << "BEGIN Configuration File (JSON): " << argv[i] << endl;
        while (!in.eof()) {
          char c;
          in.get(c);
          cout << c;
        }
        cout << "END Configuration File (JSON): " << argv[i] << endl;
        rc = true;
      } else {
        // 传统配置文件
        cf->ParseFile(argv[i]);
        ifstream in(argv[i]);
        cout << "BEGIN Configuration File: " << argv[i] << endl;
        while (!in.eof()) {
          char c;
          in.get(c);
          cout << c;
        }
        cout << "END Configuration File: " << argv[i] << endl;
        rc = true;
      }
    } else if(pos != string::npos)  {
      // override individual parameter
      cout << "OVERRIDE Parameter: " << arg << endl;
      cf->ParseString(argv[i]);
    }
  }

  return rc;
}


//helpful for the GUI, write out nearly all variables contained in a config file.
//However, it can't and won't write out  empty strings since the booksim yacc
//parser won't be abled to parse blank strings
void Configuration::WriteFile(string const & filename) {
  
  ostream *config_out= new ofstream(filename.c_str());
  
  
  for(map<string,string>::const_iterator i = _str_map.begin(); 
      i!=_str_map.end();
      i++){
    //the parser won't read empty strings
    if(i->second[0]!='\0'){
      *config_out<<i->first<<" = "<<i->second<<";"<<endl;
    }
  }
  
  for(map<string, int>::const_iterator i = _int_map.begin(); 
      i!=_int_map.end();
      i++){
    *config_out<<i->first<<" = "<<i->second<<";"<<endl;

  }

  for(map<string, double>::const_iterator i = _float_map.begin(); 
      i!=_float_map.end();
      i++){
    *config_out<<i->first<<" = "<<i->second<<";"<<endl;

  }
  config_out->flush();
  delete config_out;
 
}



void Configuration::WriteMatlabFile(ostream * config_out) const {

  
  
  for(map<string,string>::const_iterator i = _str_map.begin(); 
      i!=_str_map.end();
      i++){
    //the parser won't read blanks lolz
    if(i->second[0]!='\0'){
      *config_out<<"%"<<i->first<<" = \'"<<i->second<<"\';"<<endl;
    }
  }
  
  for(map<string, int>::const_iterator i = _int_map.begin(); 
      i!=_int_map.end();
      i++){
    *config_out<<"%"<<i->first<<" = "<<i->second<<";"<<endl;

  }

  for(map<string, double>::const_iterator i = _float_map.begin(); 
      i!=_float_map.end();
      i++){
    *config_out<<"%"<<i->first<<" = "<<i->second<<";"<<endl;

  }
  config_out->flush();

}

vector<string> tokenize_str(string const & data)
{
  vector<string> values;

  // no elements, no braces --> empty list
  if(data.empty()) {
    return values;
  }

  // doesn't start with an opening brace --> treat as single element
  // note that this element can potentially contain nested lists 
  if(data[0] != '{') {
    values.push_back(data);
    return values;
  }

  size_t start = 1;
  int nested = 0;

  size_t curr = start;

  while(string::npos != (curr = data.find_first_of("{,}", curr))) {
    
    if(data[curr] == '{') {
      ++nested;
    } else if((data[curr] == '}') && nested) {
      --nested;
    } else if(!nested) {
      if(curr > start) {
	string token = data.substr(start, curr - start);
	values.push_back(token);
      }
      start = curr + 1;
    }
    ++curr;
  }
  assert(!nested);

  return values;
}

vector<int> tokenize_int(string const & data)
{
  vector<int> values;

  // no elements, no braces --> empty list
  if(data.empty()) {
    return values;
  }

  // doesn't start with an opening brace --> treat as single element
  // note that this element can potentially contain nested lists 
  if(data[0] != '{') {
    values.push_back(atoi(data.c_str()));
    return values;
  }

  size_t start = 1;
  int nested = 0;

  size_t curr = start;

  while(string::npos != (curr = data.find_first_of("{,}", curr))) {
    
    if(data[curr] == '{') {
      ++nested;
    } else if((data[curr] == '}') && nested) {
      --nested;
    } else if(!nested) {
      if(curr > start) {
	string token = data.substr(start, curr - start);
	values.push_back(atoi(token.c_str()));
      }
      start = curr + 1;
    }
    ++curr;
  }
  assert(!nested);

  return values;
}

vector<double> tokenize_float(string const & data)
{
  vector<double> values;

  // no elements, no braces --> empty list
  if(data.empty()) {
    return values;
  }

  // doesn't start with an opening brace --> treat as single element
  // note that this element can potentially contain nested lists 
  if(data[0] != '{') {
    values.push_back(atof(data.c_str()));
    return values;
  }

  size_t start = 1;
  int nested = 0;

  size_t curr = start;

  while(string::npos != (curr = data.find_first_of("{,}", curr))) {
    
    if(data[curr] == '{') {
      ++nested;
    } else if((data[curr] == '}') && nested) {
      --nested;
    } else if(!nested) {
      if(curr > start) {
	string token = data.substr(start, curr - start);
	values.push_back(atof(token.c_str()));
      }
      start = curr + 1;
    }
    ++curr;
  }
  assert(!nested);

  return values;
}
