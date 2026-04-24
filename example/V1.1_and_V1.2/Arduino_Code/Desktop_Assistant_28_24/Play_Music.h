// Play_Music.h
#pragma once
#include <Arduino.h>  // Ensure Arduino core library is loaded first

#ifdef __cplusplus     // C++ environment only
#include <vector>      // Safely include STL library
using namespace std;   // Add namespace declaration

#include "Audio.h"
#include <WiFi.h>
#include "ui.h"

extern Audio audio;

void audioInit();
void audioLoop();
void playNextSong();
void playPreviousSong();
void togglePlayPause();
bool isAudioPlaying();

#endif
