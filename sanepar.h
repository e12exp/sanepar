#ifndef sanepar_h
#define sanepar_h

#include <cstdint>
#include <boost/next_prior.hpp>

#include <yaml-cpp/yaml.h>

namespace roothacks
{

  namespace impl
  {

    // a struct govering a single validity range of a parameter file.
    // note that files can have multiple validity ranges. 
    struct parFile
    {
      std::string fname;
      uint64_t wrtsStart{};
      uint64_t wrtsEnd{};
      std::string comment {};
      std::vector<parFile> children;
      parFile();
      parFile(std::string f, uint64_t start, uint64_t end, std::string com="");
      void addFile(parFile f);
      bool is_subset_of(const parFile& rhs) const;
    };
    std::ostream& operator<<(std::ostream&, const parFile &);
  }

  class SanePar;
  class SaneParLoader
  {
  protected:

    std::map<std::string, impl::parFile> fPars;
    
    static SaneParLoader* sInstance;

    SaneParLoader();
    SaneParLoader& operator=(const SaneParLoader&) = delete;
    SaneParLoader& operator=(SaneParLoader&&) = delete;
  public:
    void FindAndLoad(SanePar&);

    static SaneParLoader& Instance()
    {
      if (!SaneParLoader::sInstance)
        SaneParLoader::sInstance=new SaneParLoader();
      return *SaneParLoader::sInstance;
    }
    
  };

  class SanePar
  {
  public:
    SanePar(std::string parName)
      : fParName(parName)
    {}
    
    void OnExec()
    {
      SaneParLoader::Instance().FindAndLoad(*this);
    }
    
  protected:

    virtual void ParseYAML() = 0;
    virtual void Reset() = 0;
    std::string fParName;
    uint64_t wrtsValidUntil{}; // at this wrts, we will reload the parameters. 
  
  };


} // namespace roothacks
#endif
