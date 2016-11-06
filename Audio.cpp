#include "Audio.h"

AudioSystem::AudioSystem() {
  FMOD_RESULT result;

  result = FMOD::System_Create(&system);

  if (result != FMOD_OK) {
    throw std::exception();
  }

  system->init(4, FMOD_INIT_NORMAL, NULL);

  srand(time(NULL));
}

AudioSystem::~AudioSystem() {
  system->close();
  system->release();
}

FMOD_RESULT AudioSystem::createSound(std::string &path, FMOD::Sound **out) {
  return system->createSound(path.c_str(), FMOD_DEFAULT | FMOD_OPENONLY, NULL, out);
}

FMOD_RESULT AudioSystem::createSound(std::string &buffer, uint32_t freq, uint8_t depth, uint8_t channel, FMOD::Sound **out) {
  FMOD_CREATESOUNDEXINFO info;

  memset(&info, 0, sizeof(FMOD_CREATESOUNDEXINFO));
  info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
  info.length = buffer.size();
  info.defaultfrequency = (int)freq;
  info.numchannels = (int)channel;

  switch (depth) {
    case 8:       info.format = FMOD_SOUND_FORMAT_PCM8;   break;
    case 16:      info.format = FMOD_SOUND_FORMAT_PCM16;  break;
    case 24:      info.format = FMOD_SOUND_FORMAT_PCM24;  break;
    case 32:      info.format = FMOD_SOUND_FORMAT_PCM32;  break;
  }

  return system->createSound(buffer.c_str(), FMOD_OPENMEMORY | FMOD_OPENRAW | FMOD_CREATESTREAM, &info, out);
}

FMOD_RESULT AudioSystem::playSound(FMOD::Sound *sound, FMOD::Channel **out) {
  return system->playSound(sound, NULL, true, out);
}

FMOD_RESULT AudioSystem::update() {
  return system->update();
}

FMOD_RESULT AudioSystem::getInfo(std::string &path, uint32_t &samplingrate, uint8_t &bitdepth) {
  FMOD_RESULT result;
  FMOD::Sound *sound;

  result = createSound(path, &sound);

  if (result == FMOD_OK) {
    float freq;
    int bits;

    sound->getDefaults(&freq, NULL);
    sound->getFormat(NULL, NULL, NULL, &bits);

    samplingrate = (uint32_t)freq;
    bitdepth = (uint8_t)bits;

    sound->release();
  }

  return result;
}

SongSession::SongSession(AudioSystem *_pSystem) {
  pSystem = _pSystem;
  sound = NULL;
  channel = NULL;

  bBitDepthTest = false;
  bSampleRateTest = false;
}

SongSession::~SongSession() {
  SAFE_RELEASE(sound);
}

bool SongSession::openSound(const char *filepath) {
  FMOD_RESULT result;

  path = std::string(filepath);
  result = pSystem->createSound(path, &sound);

  if (result == FMOD_OK) {
    float freq;
    int cn, bits;

    sound->getDefaults(&freq, NULL);
    sound->getFormat(NULL, NULL, &cn, &bits);

    samplingrate = (uint32_t)freq;
    channel_count = (uint8_t)cn;

    if (bits == 24) {
      bBitDepthTest = true;

      if (samplingrate == 96000 || samplingrate == 192000)
        bSampleRateTest = true;
    }

    SAFE_RELEASE(sound);

    return true;
  }
  
  return false;
}

bool SongSession::readSound() {
  FMOD_RESULT result;

  result = pSystem->createSound(path, &sound);

  if (result == FMOD_OK) {
    if (bBitDepthTest || bSampleRateTest) {
      uint32_t bytelength, byteread;
      std::string *data;

      sound->getLength(&bytelength, FMOD_TIMEUNIT_PCMBYTES);

      if (bSampleRateTest)
        data = &data_192_24;
      else
        data = &data_48_24;

      data->resize(bytelength);
      sound->readData((char *)data->c_str(), bytelength, &byteread);
    }

    SAFE_RELEASE(sound);

    return true;
  }

  return false;
}

bool SongSession::enableSamplingrateTest() {
  return bSampleRateTest;
}

bool SongSession::enableBitdepthTest() {
  return bBitDepthTest;
}

uint32_t SongSession::getSamplingrate() {
  return samplingrate;
}

uint8_t SongSession::getBitdepth() {
  return 24;
}

void SongSession::convertSamplingrate() {
  bFirstSoundIsBetter = rand() % 2;
  bTestingSamplerate = true;

  if (bSampleRateTest && data_48_24.size() == 0) {
    uint32_t step = samplingrate / 48000;
    uint32_t count = data_192_24.size();
    uint32_t samplesize = 3 * channel_count;

    data_48_24.resize(count / step);
    count = data_48_24.size() / samplesize; // samples

    for (uint32_t i = 0; i < count; i ++) {
      memcpy((char *)data_48_24.c_str() + i * samplesize, data_192_24.c_str() + i * step * samplesize, samplesize);
    }
  }
}

void SongSession::convertBitdepth() {
  bTestingSamplerate = false;

  if (bBitDepthTest && data_48_8.size() == 0) {
    uint32_t count = data_48_24.size();

    data_48_8.resize(data_48_24.size() / 3);
    count = data_48_8.size(); // samples

    for (uint32_t i = 0; i < count; i++) {
      data_48_8.at(i) = data_48_24.at(i * 3 + 2) + 0x80;
    }
  }
}

void SongSession::startPlaying(bool bFirst) {
  FMOD_RESULT result;

  if (isInited())
    return;

  if (bFirstSoundIsBetter ^ bFirst) { // play low quality
    if (bTestingSamplerate) {
      result = pSystem->createSound(data_48_24, 48000, 24, channel_count, &sound);
    }
    else {
      result = pSystem->createSound(data_48_8, 48000, 8, channel_count, &sound);
    }
  }
  else {
    if (bTestingSamplerate) {
      result = pSystem->createSound(data_192_24, samplingrate, 24, channel_count, &sound);
    }
    else {
      result = pSystem->createSound(data_48_24, 48000, 24, channel_count, &sound);
    }
  }

  if (result == FMOD_OK) {
    result = pSystem->playSound(sound, &channel);

    if (result != FMOD_OK) {
      SAFE_RELEASE(sound);
    }
  }
}

bool SongSession::isInited() {
  return sound || channel;
}

bool SongSession::isPlaying() {
  bool bPaused = true;
  
  if (channel) {
    channel->getPaused(&bPaused);
  }

  return !bPaused;
}

void SongSession::togglePlaying() {
  channel->setPaused(isPlaying());
}

void SongSession::stopPlaying() {
  channel->stop();
  channel = NULL;

  SAFE_RELEASE(sound);
}

void SongSession::getTimeInfo(uint32_t &current, uint32_t &max) {
  if (isPlaying()) {
    uint32_t pos;
    uint32_t len;

    channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
    sound->getLength(&len, FMOD_TIMEUNIT_MS);

    current = pos;
    max = len;
  }
}

void SongSession::setTime(uint32_t current) {
  if (channel) {
    channel->setPosition(current, FMOD_TIMEUNIT_MS);
  }
}

bool SongSession::isCorrectAnswer(bool bSelected) {
  return bFirstSoundIsBetter == bSelected;
}

bool SongSession::isSamplingrateTest() {
  return bTestingSamplerate;
}
