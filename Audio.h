#pragma once

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <iostream>
#include <string>
#include <functional>
#include <fmod.hpp>
#include <exception>
#include <time.h>

#ifdef WIN32
#pragma comment(lib, "fmod_vc.lib")
#endif

#define SAFE_RELEASE(object)    { if (object) { object->release(); object = NULL; } }

class AudioSystem {
  private:
    FMOD::System *system;

  public:
    AudioSystem();
    ~AudioSystem();

    FMOD_RESULT createSound(std::string &, FMOD::Sound **);
    FMOD_RESULT createSound(std::string &, uint32_t, uint8_t, uint8_t, FMOD::Sound **);
    FMOD_RESULT playSound(FMOD::Sound *, FMOD::Channel **);
    FMOD_RESULT update();
    FMOD_RESULT getInfo(std::string &, uint32_t &, uint8_t &);
};

class SongSession {
  private:
    AudioSystem *pSystem;

    std::string path;
    uint32_t samplingrate;
    uint8_t channel_count;

    std::string data_192_24;  // Buffer for 192/96 kHz 24Bit audio
    std::string data_48_24;
    std::string data_48_8;

    FMOD::Sound *sound;
    FMOD::Channel *channel;

    bool bSampleRateTest;
    bool bBitDepthTest;

    bool bFirstSoundIsBetter;
    bool bTestingSamplerate;

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

    void startPlaying(bool);
    bool isInited();
    bool isPlaying();
    void togglePlaying();
    void stopPlaying();

    void getTimeInfo(uint32_t &, uint32_t &);
    void setTime(uint32_t);
    
    bool isCorrectAnswer(bool bSelected);
    bool isSamplingrateTest();
};

#endif
