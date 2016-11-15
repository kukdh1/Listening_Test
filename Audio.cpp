#include "Audio.h"

AudioSystem::AudioSystem() {
  SDL_Init(SDL_INIT_AUDIO);
  av_register_all();

  srand(time(NULL));
}

AudioSystem::~AudioSystem() {
  SDL_Quit();
}

bool AudioSystem::getInfo(std::string &path, uint32_t &samplingrate, uint8_t &bitdepth) {
  AVFormatContext *avf_context;
  bool result;

  avf_context = avformat_alloc_context();
  result = avformat_open_input(&avf_context, path.c_str(), NULL, NULL) >= 0;

  if (result) {
    float freq;
    int bits;

    result = avformat_find_stream_info(avf_context, NULL) >= 0;

    if (result) {
      unsigned int audio_id = UINT_MAX;

      for (unsigned int i = 0; i < avf_context->nb_streams; i++) {
        if (avf_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
          audio_id = i;
          break;
        }
      }

      if (audio_id != UINT_MAX) {
        samplingrate = (uint32_t)avf_context->streams[audio_id]->codec->sample_rate;
        bitdepth = (uint8_t)avf_context->streams[audio_id]->codec->bits_per_raw_sample;
      }
      else {
        result = false;
      }
    }

    avformat_close_input(&avf_context);
  }

  return result;
}

SongSession::SongSession(AudioSystem *_pSystem) {
  pSystem = _pSystem;

  bBitDepthTest = false;
  bSampleRateTest = false;

  avf_context = NULL;
  stream_id = UINT_MAX;
}

SongSession::~SongSession() {
  if (avf_context) {
    avformat_close_input(&avf_context);
  }
}

bool SongSession::openSound(const char *filepath) {
  bool result;

  avf_context = avformat_alloc_context();
  result = avformat_open_input(&avf_context, filepath, NULL, NULL) >= 0;

  if (result) {
    float freq;
    int bits;

    result = avformat_find_stream_info(avf_context, NULL) >= 0;

    if (result) {
      int bits;

      for (unsigned int i = 0; i < avf_context->nb_streams; i++) {
        if (avf_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
          stream_id = i;
          break;
        }
      }

      if (stream_id != UINT_MAX) {
        samplingrate = (uint32_t)avf_context->streams[stream_id]->codec->sample_rate;
        bits = avf_context->streams[stream_id]->codec->bits_per_raw_sample;
        channel_count = (uint32_t)avf_context->streams[stream_id]->codec->channels;

        if (bits == 24) {
          bBitDepthTest = true;

          if (samplingrate == 96000 || samplingrate == 192000)
            bSampleRateTest = true;
        }
      }
      else {
        result = false;
      }
    }
  }

  return result;
}

bool SongSession::readSound() {
  bool result = false;

  if (avf_context) {
    if (bBitDepthTest || bSampleRateTest) {
      AVPacket packet;
      AVFrame *frame;
      AVCodecContext *ctx;
      AVCodec *codec;
      std::string buffer;
      std::string *data;

      buffer.resize(MAX_AUDIO_FRAME_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);

      if (bSampleRateTest)
        data = &data_192_32;
      else
        data = &data_48_32;

      ctx = avf_context->streams[stream_id]->codec;
      codec = avcodec_find_decoder(ctx->codec_id);

      if (codec == NULL) {
        goto READ_SOUND_CLEANUP;
      }

      if (avcodec_open2(ctx, codec, NULL) < 0) {
        goto READ_SOUND_CLEANUP;
      }

      av_init_packet(&packet);
      frame = av_frame_alloc();

      packet.data = (uint8_t *)buffer.c_str();
      packet.size = buffer.size();

      int len;
      int frame_ptr = 0;
      while (av_read_frame(avf_context, &packet) >= 0) {
        if (packet.stream_index == stream_id) {
          len = avcodec_decode_audio4(ctx, frame, &frame_ptr, &packet);

          if (frame_ptr) {
            // libavcodec provide 32bit sample for 24bit audio
            data->append((char *)frame->extended_data[0], frame->linesize[0]);
          }
        }
      }

      av_frame_free(&frame);
    }
    else {
      goto READ_SOUND_CLEANUP;
    }

    result = true;
  }

READ_SOUND_CLEANUP:
  avformat_close_input(&avf_context);

  return result;
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

  if (bSampleRateTest && data_48_32.size() == 0) {
    uint32_t step = samplingrate / 48000;
    uint32_t count = data_192_32.size();
    uint32_t samplesize = 4 * channel_count;

    data_48_32.resize(count / step);
    count = data_48_32.size() / samplesize; // samples

    for (uint32_t i = 0; i < count; i ++) {
      memcpy((char *)data_48_32.c_str() + i * samplesize, data_192_32.c_str() + i * step * samplesize, samplesize);
    }
  }
}

void SongSession::convertBitdepth() {
  bTestingSamplerate = false;

  if (bBitDepthTest && data_48_8.size() == 0) {
    uint32_t count = data_48_32.size();

    data_48_8.resize(data_48_32.size() / 4);
    count = data_48_8.size(); // samples

    for (uint32_t i = 0; i < count; i++) {
      data_48_8.at(i) = data_48_32.at(i * 4 + 3) + 0x80;
    }
  }
}

bool SongSession::startPlaying(bool bFirst) {
  bool result;

  if (isInited())
    return false;
  
  // fill default specs
  spec.channels = channel_count;
  spec.userdata = this;
  spec.callback = fill_audio;
  spec.samples = 4096;

  if (bFirstSoundIsBetter ^ bFirst) { // play low quality
    if (bTestingSamplerate) {
      current_data = &data_48_32;
      byte_per_sample = 4;
      spec.freq = 48000;
      spec.format = AUDIO_S32;
    }
    else {
      current_data = &data_48_8;
      byte_per_sample = 1;
      spec.freq = 48000;
      spec.format = AUDIO_U8;
    }
  }
  else {
    if (bTestingSamplerate) {
      current_data = &data_192_32;
      byte_per_sample = 4;
      spec.freq = samplingrate;
      spec.format = AUDIO_S32;
    }
    else {
      current_data = &data_48_32;
      byte_per_sample = 4;
      spec.freq = 48000;
      spec.format = AUDIO_S32;
    }
  }

  // Open audio
  audio_index = 0;
  result = SDL_OpenAudio(&spec, NULL) >= 0;

  return result;
}

bool SongSession::isInited() {
  return spec.userdata == this;
}

bool SongSession::isPlaying() {
  return SDL_GetAudioStatus() == SDL_AUDIO_PLAYING;
}

void SongSession::togglePlaying() {
  SDL_PauseAudio(isPlaying());
}

void SongSession::stopPlaying() {
  SDL_CloseAudio();
  memset(&spec, 0, sizeof(SDL_AudioSpec));
}

void SongSession::getTimeInfo(uint32_t &current, uint32_t &max) {
  if (isPlaying()) {
    current = sampleToMs(audio_index / byte_per_sample);
    max = sampleToMs(current_data->size() / byte_per_sample);
  }
}

void SongSession::setTime(uint32_t current) {
  audio_index = msToSample(current) * byte_per_sample;
}

uint32_t SongSession::msToSample(uint32_t ms) {
  float samples_per_second = spec.freq * spec.channels;
  return (uint32_t)(samples_per_second / 1000.f * ms + 0.5f);
}

uint32_t SongSession::sampleToMs(uint32_t sample) {
  float samples_per_second = spec.freq * spec.channels;
  return (uint32_t)(sample / samples_per_second * 1000.f + 0.5f);
}

bool SongSession::isCorrectAnswer(bool bSelected) {
  return bFirstSoundIsBetter == bSelected;
}

bool SongSession::isSamplingrateTest() {
  return bTestingSamplerate;
}

void SongSession::fill_audio(void *udata, uint8_t *stream, int len) {
  SongSession *pThis = (SongSession *)udata;

  int bytes_left = pThis->current_data->size() - pThis->audio_index;

  if (bytes_left <= 0)
    return;

  len = SDL_min(len, bytes_left);
  SDL_MixAudio(stream, (uint8_t *)pThis->current_data->c_str() + pThis->audio_index, len, SDL_MIX_MAXVOLUME);
  pThis->audio_index += len;
}
