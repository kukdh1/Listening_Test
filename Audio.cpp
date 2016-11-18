#include "Audio.h"

AudioSystem::AudioSystem() {
  Pa_Initialize();
  av_register_all();

  srand(time(NULL));
}

AudioSystem::~AudioSystem() {
  Pa_Terminate();
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
        if (avf_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
          audio_id = i;
          break;
        }
      }

      if (audio_id != UINT_MAX) {
        samplingrate = (uint32_t)avf_context->streams[audio_id]->codecpar->sample_rate;
        bitdepth = (uint8_t)avf_context->streams[audio_id]->codecpar->bits_per_raw_sample;
      }
      else {
        result = false;
      }
    }

    avformat_close_input(&avf_context);
    avformat_free_context(avf_context);
  }

  return result;
}

SongSession::SongSession(AudioSystem *_pSystem) {
  pSystem = _pSystem;

  bBitDepthTest = false;
  bSampleRateTest = false;

  avf_context = NULL;
  current_stream = NULL;
  stream_id = UINT_MAX;
  current_freq = 0;
  audio_index = 0;
}

SongSession::~SongSession() {
  if (avf_context) {
    avformat_close_input(&avf_context);
    avformat_free_context(avf_context);
  }
  if (current_stream) {
    Pa_StopStream(current_stream);
    Pa_CloseStream(current_stream);
  }
}

void SongSession::sineWaveTest() {
  // Create sine wave (1sec)
  current_freq = 192000;
  memset(&spec, 0, sizeof(PaStreamParameters));
  spec.device = Pa_GetDefaultOutputDevice();
  spec.channelCount = 1;
  spec.suggestedLatency = Pa_GetDeviceInfo(spec.device)->defaultLowOutputLatency;
  spec.sampleFormat = paUInt8;
  current_data = &data_192_24;
  byte_per_sample = 1;
  uint32_t buffersize = current_freq * spec.channelCount / 10;

  // Create buffer
  data_192_24.resize(current_freq);

  for (uint32_t i = 0; i < current_freq; i++) {
    data_192_24.at(i) = i % 2 ? 0x40 : 0xC0;
  }

  // Play sinewave for 1 sec
  if (Pa_OpenStream(&current_stream, NULL, &spec, current_freq, buffersize, paClipOff, fill_audio, this) == paNoError) {
    Pa_StartStream(current_stream);
    Pa_Sleep(1000);
    Pa_StopStream(current_stream);
    Pa_CloseStream(current_stream);
    current_stream = NULL;
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
        if (avf_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
          stream_id = i;
          break;
        }
      }

      if (stream_id != UINT_MAX) {
        samplingrate = (uint32_t)avf_context->streams[stream_id]->codecpar->sample_rate;
        bits = avf_context->streams[stream_id]->codecpar->bits_per_raw_sample;
        channel_count = (uint32_t)avf_context->streams[stream_id]->codecpar->channels;

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
      AVCodecParameters *cpar;
      AVCodec *codec;
      std::string buffer;
      std::string *data;

      buffer.resize(MAX_AUDIO_FRAME_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);

      if (bSampleRateTest)
        data = &data_192_24;
      else
        data = &data_48_24;

      cpar = avf_context->streams[stream_id]->codecpar;
      ctx = avcodec_alloc_context3(NULL);
      
      if (ctx == NULL) {
        goto READ_SOUND_CLEANUP;
      }
      
      if (avcodec_parameters_to_context(ctx, cpar) < 0) {
        avcodec_free_context(&ctx);
        goto READ_SOUND_CLEANUP;
      }
      
      codec = avcodec_find_decoder(ctx->codec_id);

      if (codec == NULL) {
        goto READ_SOUND_CLEANUP;
      }

      if (avcodec_open2(ctx, codec, NULL) < 0) {
        goto READ_SOUND_CLEANUP;
      }

      frame = av_frame_alloc();

      packet.data = (uint8_t *)buffer.c_str();
      packet.size = buffer.size();
      
      int len;
      int frame_ptr;
      while (true) {
        av_init_packet(&packet);

        if (av_read_frame(avf_context, &packet) < 0) {
          break;
        }

        if (packet.stream_index == stream_id) {
          frame_ptr = 0;

          if (avcodec_send_packet(ctx, &packet) < 0) {
            break;
          }

          len = avcodec_receive_frame(ctx, frame);

          if (len < 0 && len != AVERROR(EAGAIN) && len != AVERROR_EOF) {
            break;
          }
          if (len >= 0) {
            frame_ptr = 1;
          }

          if (frame_ptr) {
            // libavcodec provide 32bit sample for 24bit audio
            int sample_size = frame->linesize[0] / 4;
            int beginidx = data->size();
            data->append(sample_size * 3, NULL);

            for (int i = 0; i < sample_size; i++) {
              memcpy((char *)data->c_str() + beginidx + i * 3, frame->extended_data[0] + i * 4 + 1, 3);
            }
          }

          av_frame_unref(frame);
        }

        av_packet_unref(&packet);  // av_init_packet
      }

      av_frame_free(&frame);        // av_frame_alloc
      avcodec_close(ctx);           // avcodec_open2
      avcodec_free_context(&ctx);   // avcodec_alloc_context3
    }
    else {
      goto READ_SOUND_CLEANUP;
    }

    result = true;
  }

READ_SOUND_CLEANUP:
  avformat_close_input(&avf_context);   // avformat_open_input
  avformat_free_context(avf_context);   // avformat_alloc_context

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

bool SongSession::startPlaying(bool bFirst) {
  bool result;

  if (isInited())
    return false;
  
  // fill default specs
  memset(&spec, 0, sizeof(PaStreamParameters));
  spec.device = Pa_GetDefaultOutputDevice();
  spec.channelCount = channel_count;
  spec.suggestedLatency = Pa_GetDeviceInfo(spec.device)->defaultLowOutputLatency;

  if (bFirstSoundIsBetter ^ bFirst) { // play low quality
    if (bTestingSamplerate) {
      current_data = &data_48_24;
      byte_per_sample = 3;
      current_freq = 48000;
      spec.sampleFormat = paInt24;
    }
    else {
      current_data = &data_48_8;
      byte_per_sample = 1;
      current_freq = 48000;
      spec.sampleFormat = paUInt8;
    }
  }
  else {
    if (bTestingSamplerate) {
      current_data = &data_192_24;
      byte_per_sample = 3;
      current_freq = samplingrate;
      spec.sampleFormat = paInt24;
    }
    else {
      current_data = &data_48_24;
      byte_per_sample = 3;
      current_freq = 48000;
      spec.sampleFormat = paInt24;
    }
  }

  // Buffer as 0.1sec
  uint32_t buffersize = current_freq * spec.channelCount / 10;

  // Open audio
  audio_index = 0;
  result = Pa_OpenStream(&current_stream, NULL, &spec, current_freq, buffersize, paClipOff, fill_audio, this) == paNoError;

  return result;
}

bool SongSession::isInited() {
  return current_freq != 0;
}

bool SongSession::isPlaying() {
  if (current_stream) {
    return Pa_IsStreamStopped(current_stream) == 0;
  }

  return false;
}

void SongSession::togglePlaying() {
  if (isPlaying()) {
    Pa_StopStream(current_stream);
  }
  else {
    Pa_StartStream(current_stream);
  }
}

void SongSession::stopPlaying() {
  Pa_StopStream(current_stream);
  Pa_CloseStream(current_stream);
  current_stream = NULL;
  current_freq = 0;
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
  float samples_per_second = current_freq * spec.channelCount;
  return (uint32_t)(samples_per_second / 1000.f * ms + 0.5f);
}

uint32_t SongSession::sampleToMs(uint32_t sample) {
  float samples_per_second = current_freq * spec.channelCount;
  return (uint32_t)(sample / samples_per_second * 1000.f + 0.5f);
}

bool SongSession::isCorrectAnswer(bool bSelected) {
  return bFirstSoundIsBetter == bSelected;
}

bool SongSession::isSamplingrateTest() {
  return bTestingSamplerate;
}

int SongSession::fill_audio(const void *inbuf, void *outbuf, unsigned long frames_per_buf, const PaStreamCallbackTimeInfo* time, PaStreamCallbackFlags flags, void *userdata) {
  SongSession *pThis = (SongSession *)userdata;
  int total_frame_count = pThis->current_data->size() / pThis->byte_per_sample / pThis->spec.channelCount;
  int played_frame_count = pThis->audio_index / pThis->byte_per_sample / pThis->spec.channelCount;

  int frame_left = total_frame_count - played_frame_count;

  if (frame_left <= 0) {
    return paComplete;
  }

  int frame_to_copy = FFMIN(frames_per_buf, frame_left);
  int byte_to_copy = frame_to_copy * pThis->byte_per_sample * pThis->spec.channelCount;

  memcpy(outbuf, pThis->current_data->c_str() + pThis->audio_index, byte_to_copy);
  pThis->audio_index += byte_to_copy;

  return paContinue;
}
