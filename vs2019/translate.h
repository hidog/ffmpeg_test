#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <string>
#include <tuple>
#include "player/play_def.h"


std::tuple<VideoDecodeSetting, AudioDecodeSetting>    extraction_audio( std::string input_file, std::string audio_file );

void    re_encode_media( std::string output_file, VideoDecodeSetting vs, AudioDecodeSetting as );

void    translate_media_file();



#endif