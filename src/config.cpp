/*
   Copyright (c) 2010, The Mineserver Project
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
 * Neither the name of the The Mineserver Project nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "logger.h"
#include "constants.h"
#include "permissions.h"
#include "mineserver.h"

#include "config.h"

// Load/reload configuration
bool Conf::load(std::string configFile, std::string namePrefix)
{
#ifdef _DEBUG
  Mineserver::get()->screen()->log("Loading data from " + configFile);
#endif
  std::ifstream ifs(configFile.c_str());

  // If configfile does not exist
  if(ifs.fail() && configFile == CONFIG_FILE)
  {
    // TODO: Load default configuration from the internets!
    Mineserver::get()->screen()->log(LOG_ERROR, "Warning: " + configFile + " not found! Generating it now.");

    // Open config file
    std::ofstream confofs(configFile.c_str());

    // Write header
    confofs << "# This is the default config, please see http://mineserver.be/wiki/Configuration for more information." << std::endl << std::endl;

    // Write all the default settings
    std::map<std::string, std::string>::iterator iter;
    for(iter=defaultConf.begin();iter!=defaultConf.end();++iter)
    {
      confofs << iter->first << " = " << iter->second << std::endl;
    }

    // Close the config file
    confofs.close();

    this->load(CONFIG_FILE);
  }

  if (ifs.fail())
  {
    Mineserver::get()->screen()->log(LOG_ERROR, "Warning: " + configFile + " not found!");
    ifs.close();
    return true;
  }

  std::string temp;

  // Reading row at a time
  int del;
  int lineNum = 0;
  std::vector<std::string> line;
  std::string text;
  while(getline(ifs, temp))
  {
    //If empty line
    if(temp.size() == 0)
    {
      continue;
    }

    // If commentline -> skip to next
    if(temp[0] == COMMENTPREFIX)
    {
      continue;
    }

    // If not enough characters (Absolute min is 5: "a = s")
    if(temp.length() < 5)
    {
      continue;
    }

    // Init vars
    del = 0;
    line.clear();

    // Process line
    while(temp.length() > 0)
    {
      // Remove white spaces and = characters -_-
      while(temp[0] == ' ' || temp[0] == '=')
      {
        temp = temp.substr(1);
      }

      // Split words
      del = temp.find(' ');
      if(del > -1)
      {
        line.push_back(temp.substr(0, del));
        temp = temp.substr(del+1);
      }
      else
      {
        line.push_back(temp);
        break;
      }
    }

    // If under two words skip the line and log skipping.
    if(line.size() < 2)
    {
      Mineserver::get()->screen()->log(LOG_ERROR, "Invalid configuration at line " + dtos(lineNum) + " of " + configFile);
      continue;
    }

    // Construct strings if needed
    text = "";
    if(line[1][0] == '"')
    {
      // Append to text
      for(unsigned int i = 1; i < line.size(); i++)
      {
        text += line[i] + " ";
      }
      // Remove ""
      text = text.substr(1, text.length()-3);
    }
    else
    {
      text = line[1];
    }

    if (line[0] == "include")
    {
      #ifdef _DEBUG
      Mineserver::get()->screen()->log("Including config file " + text);
      #endif
      load(text);
      continue;
    }

    // Update existing configuration and add new lines
    if(m_confSet.find(namePrefix + line[0]) != m_confSet.end())
    {
      m_confSet[namePrefix + line[0]] = text;
    }
    else
    {
      // Push to configuration
      m_confSet.insert(std::pair<std::string, std::string>(namePrefix + line[0], text));
    }

    // Count line numbers
    lineNum++;
  }
  ifs.close();
  #ifdef _DEBUG
  Mineserver::get()->screen()->log("Loaded " + dtos(lineNum) + " lines from " + configFile);
  #endif

  return true;
}

// Return values
std::string Conf::sValue(std::string name)
{
  if(m_confSet.find(name) != m_confSet.end())
  {
    return m_confSet[name];
  }
  else
  {
		Mineserver::get()->screen()->log("Warning! " + name + " not defined in configuration. Using default value: " + defaultConf[name]);
    return defaultConf[name];
  }
}

int Conf::iValue(std::string name)
{
  if(m_confSet.find(name) != m_confSet.end())
  {
    return atoi(m_confSet[name].c_str());
  }
  else
  {
    Mineserver::get()->screen()->log("Warning! " + name + " not defined in configuration. Using default value: " + defaultConf[name]);
    return atoi(defaultConf[name].c_str());
  }
}

bool Conf::bValue(std::string name)
{
  if(m_confSet.find(name) != m_confSet.end())
  {
    return (m_confSet[name] == "true")?true:false;
  }
  else
  {
    Mineserver::get()->screen()->log("Warning! " + name + " not defined in configuration. Using default value: " + defaultConf[name]);
    return (defaultConf[name] == "true")?true:false;
  }
}

std::vector<int> Conf::vValue(std::string name)
{
  if(m_confSet.find(name) != m_confSet.end())
  {
    return stringToVec(m_confSet[name]);
  }
  else
  {
    Mineserver::get()->screen()->log("Warning! " + name + " not defined in configuration.");
    return std::vector<int>();
  }
}

int Conf::commandPermission(std::string commandName)
{
  return permissionByName(sValue(COMMANDS_NAME_PREFIX + commandName));
}

int Conf::permissionByName(std::string permissionName)
{
  if(permissionName == "admin")
  {
    return PERM_ADMIN;
  }

  if(permissionName == "op")
  {
    return PERM_OP;
  }

  if(permissionName == "member")
  {
    return PERM_MEMBER;
  }

  if(permissionName == "guest")
  {
    return PERM_GUEST;
  }

  Mineserver::get()->screen()->log("Warning! Unknown permission name: " + permissionName + " - Using GUEST permission by default!");

  return PERM_GUEST; // default
}

std::vector<int> Conf::stringToVec(std::string& str)
{
  std::vector<int> temp;
  int del;
  // Process "array"
  while(str.length() > 0)
  {
    // Remove white spaces characters
    while(str[0] == ' ')
    {
      str = str.substr(1);
    }

    // Split words
    del = str.find(',');
    if(del > -1)
    {
      temp.push_back(atoi(str.substr(0, del).c_str()));
      str = str.substr(del+1);
    }
    else
    {
      temp.push_back(atoi(str.c_str()));
      break;
    }
  }

  return temp;
}
