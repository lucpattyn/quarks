#ifndef __FFMPEGPLUGIN_H__
#define __FFMPEGPLUGIN_H__

class FFMpegPlugin
{
public:
  FFMpegPlugin();

  /* use virtual otherwise linker will try to perform static linkage */
  virtual std::string Execute(std::string params, std::string body);

private:
  
};

// compile with
//g++ -fPIC -shared ffmpegplugin.cpp -o ffmpegplugin.so

#endif
