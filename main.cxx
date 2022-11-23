#include <sanepar.h>
#include <iostream>
#include <cstdlib>
#include <fairlogger/Logger.h>

using namespace roothacks;
using namespace roothacks::impl;


int main(int argc, char** argv)
{
  fair::Logger::OnFatal([]{throw std::runtime_error("LoggedError");});
  
  if (argc==2)
    {
      std::cout << "Setting R3BCONF to " <<  argv[1] <<  ".\n";
      setenv("R3BCONF", argv[1], 1);
    }
  //auto& x=
  SaneParLoader::Instance();
}
