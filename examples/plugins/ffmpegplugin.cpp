#include <iostream>
#include "ffmpegplugin.h"

using namespace std;

extern "C" std::string execute(std::string params, std::string body)
{
	static FFMpegPlugin o;
	return o.Execute(params, body);
}

FFMpegPlugin::FFMpegPlugin()
{
  
}

std::string FFMpegPlugin::Execute(std::string params, std::string body)
{
	std::cout << "Inside Plugin -- " << params << " " << body << endl;
	//ffmpeg -i in.mkv -c:v h264 -flags +cgop -g 30 -hls_time 1 out.m3u8
	
	return body;
}

