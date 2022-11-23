#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <stdlib.h>
//#include <FairLogger.h>
#include <fairlogger/Logger.h>

#include <boost/format.hpp>
#include <ctime>

#include <sanepar.h>

using namespace roothacks;
using namespace roothacks::impl;

SaneParLoader* SaneParLoader::sInstance {};


// parse the header, and return the value of the data-type field. 
std::string parseHeader(YAML::Node& n) 
{
  std::string res;
  if (!n["data-type"])
    throw std::runtime_error("yaml document zero does not contain the key \'data-type\'.");
  const std::vector<std::string> header_fields={"created-at", "created-by", "command",
                                                "version", "comment", "action"};
  for (auto& h: header_fields)
    if (!n[h])
      LOG(warning) << "yaml document zero does not contain the key \'"<<h <<"\' ";
  return n["data-type"].as<std::string>();
}

void parseRange(std::vector<parFile>& out, const YAML::Node& node, const std::string& fname, uint64_t& prevEnd)
{
  if (!node["wrtsStart"] || (!node["wrtsEnd"]))
    {
      throw std::runtime_error("Document does not contain wrtsStart and wrtsEnd.");
    }
  uint64_t start=node["wrtsStart"].as<uint64_t>();
  uint64_t end=node["wrtsEnd"].as<uint64_t>();
  if (start<prevEnd)
    {
      throw std::runtime_error("wrtsStart is smaller than previous wrtsEnd.");
    }
  prevEnd=end;
  parFile res(fname, start, end);
  if (node["comment"])
    res.comment=node["comment"].as<std::string>();
  LOG(info) << "Adding "<< res ;
  out.push_back(res);
}

std::map<std::string, std::vector<parFile>> parseAllYaml()
{
  std::string cfg=std::getenv("R3BCONF");
  if (cfg.empty())
    LOG(fatal) << "Please export R3BCONF to point to the directory containing the yaml configurations.\n";
  LOG(info) << "Parsing yaml configurations in \""<< cfg << "\".\n";
  
  std::map<std::string, std::vector<parFile>> cfgFiles{};
  
  for (auto& f: std::filesystem::recursive_directory_iterator(cfg))
    {
      auto fname=f.path().string();
      auto n=fname.length();
      if (fname.substr(n-5, n)==(".yaml"))
        {
          std::string dataname{};
          std::vector<YAML::Node> nodes;
          try
            {
              nodes=YAML::LoadAllFromFile(fname);
            }
          catch (std::runtime_error& e)
            {
              LOG(fatal) << "File "<< fname << " could not be parsed as yaml: "<< e.what() << "\n";
            }
          
          if (nodes.size()<3)
            LOG(fatal) << "File "<< fname << " contains less than three yaml documents separated by ---\n";
          dataname=parseHeader(nodes[0]);
          
          uint64_t prevEnd=0;
          for (size_t i=1; i<nodes.size()-1; i++)
            {
              try
                {
                  parseRange(cfgFiles[dataname], nodes[i], fname, prevEnd);
                }
              catch(std::runtime_error& e)
                {
                  LOG(fatal) << fname << ", yaml document "<< i << ": "
                             << "\n------------------------------------------\n"
                             << nodes[i]
                             << "\n------------------------------------------\n"
                             << "   Error: " <<e.what()<<"  . This time range will be ignored.";
                }
            }
        }
    }
  return cfgFiles;
}



SaneParLoader::SaneParLoader()
{
  auto cfgFiles=parseAllYaml();
  for (auto& c: cfgFiles)
    {
      auto& v= c.second; // vector of parFiles
      std::sort(v.begin(), v.end(), [](const parFile& lhs, const parFile& rhs) {return lhs.wrtsStart < rhs.wrtsStart;} );
      parFile res("NOFILE", 0L, (uint64_t)(-1), "WRTS Range");
      for (auto& f: c.second)
        res.addFile(f);
      fPars[c.first]=res;
      std::cout << "===== " << c.first << "=====\n";
      std::cout <<res << "\n";
    }
}




bool parFile::is_subset_of(const parFile& rhs) const // subset operator
{
  auto& lhs=*this;
  if (lhs.wrtsStart==rhs.wrtsStart && lhs.wrtsEnd==rhs.wrtsEnd)
    {
          LOG(fatal) << __PRETTY_FUNCTION__ <<": Error: parameter files with identical validity ranges: \n"
                     << lhs << "\n"
                     << rhs << "\n";
    }
  
  if ( rhs.wrtsStart <= lhs.wrtsStart )
    {
      // possible subset
      if (lhs.wrtsEnd <= rhs.wrtsEnd )
        {
          return 1;
        }
      else if (rhs.wrtsEnd >= lhs.wrtsStart) // overlap
        {
          LOG(fatal) << __PRETTY_FUNCTION__ <<": Error: partial overlap in parameter files validity: \n"
                     << lhs << "\n"
                     << rhs << "\n";
        }
      return 0;
    }
  else
    {
      rhs.is_subset_of(lhs); // check for overlaps
      return 0;
    }
}


namespace roothacks { namespace impl {

    struct wrts_conv
    {
      uint64_t wrts;
      
    };
    std::ostream& operator<<(std::ostream& out, const wrts_conv &f)
    {
      const uint64_t oneG=1000000000L;
      auto quot=f.wrts/oneG;
      auto rem =f.wrts%oneG;
      char buf[100];
      const int utc_offset=37;
      time_t time=quot-utc_offset;
      strftime(buf, 100, "(approx %F %H:%M:%S UTC)", gmtime(&time));

      out << boost::format("% 12d") % quot << "."
          << boost::format("%09d") % rem
          <<  " TAI "
          << buf;
      
      return out;
    }
    
    std::ostream& operator<<(std::ostream& out, const parFile &f)
    {
      static int ident{};
      std::string skip (ident, '*');
      out << boost::format("%-50s")%(skip+" "+f.fname) << "    [" << wrts_conv{f.wrtsStart} << "  to "
          << wrts_conv{f.wrtsEnd} << "] "
          << skip << " "<<f.comment << " ";
      if (!f.children.empty())
        {
          ident++;
          for (auto c: f.children)
            {
              out <<"\n"<<c;
            }
          ident--;
        }
      return out;
    }
  }};

parFile::parFile()
  : parFile("INVALID", 0, 1, "INVALID")
{}

parFile::parFile(std::string f, uint64_t start, uint64_t end, std::string com)
  :fname(f)
  ,wrtsStart(start)
  ,wrtsEnd(end)
  ,comment(com)
{
  if (start>=end)
    {
      LOG(fatal) << __PRETTY_FUNCTION__ <<": Error: parameter file valid for less than 1ns: \n"
                 << *this << "\n";
    }
}

void parFile::addFile(parFile p)
{
  auto& self=*this;
  if (!(p.is_subset_of(self)))
    {
      LOG(fatal) << __PRETTY_FUNCTION__ << ": Internal error: tried to add a subfile which does not cover a subrange\n"
                 << "*this=" << self << "\n"
                 << "p    =" << p << "\n";
    }
  for (auto& c: self.children)
    if (p.is_subset_of(c))
      {
        c.addFile(p);
        return;
      }
  self.children.push_back(p);
}
