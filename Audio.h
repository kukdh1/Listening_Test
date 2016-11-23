#pragma once

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <iostream>
#include <string>
#include <functional>
#include <exception>
#include <vector>
#include <time.h>
#include <portaudio.h>

#include "Model.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

#define MAX_AUDIO_FRAME_SIZE    192000

#define STRING_COMBO_TESTTYPE   "<Test Type>"
#define STRING_COMBO_HQ_AUDIO   "<HQ Audio Factor>"
#define STRING_COMBO_LQ_AUDIO   "<LQ Audio Factor>"

#ifdef _WIN32
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "portaudio_x86.lib")
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
    uint32_t bitdepth;

    uint32_t audio_index;
    PaStreamParameters spec;
    PaStream *current_stream;
    uint32_t current_freq;
    uint32_t byte_per_sample;
    std::string *current_data;

    std::string data_original;
    std::string data_hq;
    std::string data_lq;

    bool bFirstSoundIsBetter;
    bool bTestingSamplerate;
    uint32_t uiFactorHQ;
    uint32_t uiFactorLQ;

    static int fill_audio(const void *, void *, unsigned long, const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *);
    uint32_t msToSample(uint32_t);
    uint32_t sampleToMs(uint32_t);

    static void convertSamplingRate(std::string &, std::string &, uint32_t, uint32_t, uint32_t);
    static void convertBitdepth(std::string &, std::string &, uint32_t, uint32_t, uint32_t);

  public:
    SongSession(AudioSystem *);
    ~SongSession();

    void getTestTypes(std::vector<std::string> &);
    void getHQFactors(std::vector<std::string> &);
    void getLQFactors(std::vector<std::string> &);
  
    void getTestInfo(bool &, uint32_t &, uint32_t &);
    bool setTestType(std::string);
    bool setTestInfo(std::string, std::string);
  
    void sineWaveTest();

    bool openSound(const char *);
    bool readSound();

    uint32_t getSamplingrate();
    uint8_t getBitdepth();

    bool startPlaying(bool);
    bool isInited();
    bool isPlaying();
    void togglePlaying();
    void stopPlaying();

    void getTimeInfo(uint32_t &, uint32_t &);
    void setTime(uint32_t);
    
    void getTestResult(bool &);
};

#endif
