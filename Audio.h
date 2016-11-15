#pragma once

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <iostream>
#include <string>
#include <functional>
#include <exception>
#include <time.h>
#include <SDL.h>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

#define MAX_AUDIO_FRAME_SIZE    192000

#ifdef _WIN32
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#endif

#ifdef QT_NEEDS_QMAIN
#undef QT_NEEDS_QMAIN
#endif

class AudioSystem {
  public:
    AudioSystem();
    ~AudioSystem();

    bool getInfo(std::string &, uint32_t &, uint8_t &);
};

class SongSession {
  private:
    AudioSystem *pSystem;

    AVFormatContext *avf_context;
    uint32_t stream_id;
    uint32_t samplingrate;
    uint32_t channel_count;

    uint32_t audio_index;
    SDL_AudioSpec spec;
    uint32_t byte_per_sample;
    std::string *current_data;

    std::string data_192_32;  // Buffer for 192/96 kHz 24Bit audio
    std::string data_48_32;
    std::string data_48_8;

    bool bSampleRateTest;
    bool bBitDepthTest;

    bool bFirstSoundIsBetter;
    bool bTestingSamplerate;

    static void fill_audio(void *, uint8_t *, int);
    uint32_t msToSample(uint32_t);
    uint32_t sampleToMs(uint32_t);

  public:
    SongSession(AudioSystem *);
    ~SongSession();

    bool openSound(const char *);
    bool readSound();
    bool enableSamplingrateTest();
    bool enableBitdepthTest();

    uint32_t getSamplingrate();
    uint8_t getBitdepth();

    void convertSamplingrate();
    void convertBitdepth();

    bool startPlaying(bool);
    bool isInited();
    bool isPlaying();
    void togglePlaying();
    void stopPlaying();

    void getTimeInfo(uint32_t &, uint32_t &);
    void setTime(uint32_t);
    
    bool isCorrectAnswer(bool);
    bool isSamplingrateTest();
};

#endif
