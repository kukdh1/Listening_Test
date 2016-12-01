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

  avf_context = NULL;
  current_stream = NULL;
  stream_id = UINT_MAX;
  current_freq = 0;
  audio_index = 0;
  bitdepth = 0;
  samplingrate = 0;
  channel_count = 0;
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

void SongSession::getTestTypes(std::vector<std::string> &data) {
  data.clear();

  data.push_back(STRING_COMBO_TESTTYPE);
  data.push_back(STRING_LIST_SAMPLINGRATE);
  data.push_back(STRING_LIST_BITDEPTH);
}

void SongSession::getHQFactors(std::vector<std::string> &data) {
  data.clear();

  data.push_back(STRING_COMBO_HQ_AUDIO);

  if (bTestingSamplerate) {
    for (uint32_t i = samplingrate; i >= 24000; i /= 2) {
      data.push_back(std::to_string(i));
    }
  }
  else {
    data.push_back("24");
    data.push_back("16");
  }
}

void SongSession::getLQFactors(std::vector<std::string> &data) {
  data.clear();

  data.push_back(STRING_COMBO_LQ_AUDIO);

  if (bTestingSamplerate) {
    for (uint32_t i = samplingrate / 2; i >= 12000; i /= 2) {
      data.push_back(std::to_string(i));
    }
  }
  else {
    data.push_back("16");
    data.push_back("8");
  }
}

bool SongSession::setTestType(std::string testtype) {
  if (testtype.compare(STRING_LIST_SAMPLINGRATE) == 0) {
    bTestingSamplerate = true;

    return true;
  }
  else if (testtype.compare(STRING_LIST_BITDEPTH) == 0) {
    bTestingSamplerate = false;

    return true;
  }

  return false;
}

void SongSession::getTestInfo(bool &testtype, uint32_t &hqFactor, uint32_t &lqFactor) {
  testtype = bTestingSamplerate;
  hqFactor = uiFactorHQ;
  lqFactor = uiFactorLQ;
}

bool SongSession::setTestInfo(std::string hqFactor, std::string lqFactor) {
  uiFactorHQ = atol(hqFactor.c_str());
  uiFactorLQ = atol(lqFactor.c_str());

  return uiFactorHQ > uiFactorLQ;
}

void SongSession::sineWaveTest(int targetFrequency) {
  int time = 2;
  
  // Create sine wave (1sec)
  current_freq = 192000;
  memset(&spec, 0, sizeof(PaStreamParameters));
  spec.device = Pa_GetDefaultOutputDevice();
  spec.channelCount = 1;
  spec.suggestedLatency = Pa_GetDeviceInfo(spec.device)->defaultLowOutputLatency;
  spec.sampleFormat = paInt16;
  current_data = &data_original;
  byte_per_sample = 2;
  uint32_t length = current_freq * spec.channelCount * byte_per_sample;
  uint32_t buffersize = length / 10;
  
  length *= time;

  // Create buffer
  data_original.resize(length);

  for (uint32_t i = 0; i < length; i += byte_per_sample) {
    int16_t sample = 0x7FFF * cosf(2 * 3.1415926f * targetFrequency * i / byte_per_sample / current_freq);
    
    memcpy((char *)data_original.c_str() + i, &sample, byte_per_sample);
  }

  // Play sinewave for 1 sec
  if (Pa_OpenStream(&current_stream, NULL, &spec, current_freq, buffersize, paClipOff, fill_audio, this) == paNoError) {
    Pa_StartStream(current_stream);
    Pa_Sleep(1000 * time);
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
    result = avformat_find_stream_info(avf_context, NULL) >= 0;

    if (result) {
      for (unsigned int i = 0; i < avf_context->nb_streams; i++) {
        if (avf_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
          stream_id = i;
          break;
        }
      }

      if (stream_id != UINT_MAX) {
        samplingrate = (uint32_t)avf_context->streams[stream_id]->codecpar->sample_rate;
        bitdepth = (uint32_t)avf_context->streams[stream_id]->codecpar->bits_per_raw_sample;
        channel_count = (uint32_t)avf_context->streams[stream_id]->codecpar->channels;
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

  if (avf_context && bitdepth == 24) {
    AVPacket packet;
    AVFrame *frame;
    AVCodecParameters *cpar;
    AVCodecContext *ctx;
    AVCodec *codec;
    std::string buffer;

    buffer.resize(MAX_AUDIO_FRAME_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);

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
      avcodec_free_context(&ctx);
      goto READ_SOUND_CLEANUP;
    }

    if (avcodec_open2(ctx, codec, NULL) < 0) {
      avcodec_free_context(&ctx);
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

      if ((uint32_t)packet.stream_index == stream_id) {
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
          int beginidx = data_original.size();
          data_original.append(sample_size * 3, NULL);

          for (int i = 0; i < sample_size; i++) {
            memcpy((char *)data_original.c_str() + beginidx + i * 3, frame->extended_data[0] + i * 4 + 1, 3);
          }
        }

        av_frame_unref(frame);  // av_read_frame
      }

      av_packet_unref(&packet); // av_init_packet
    }

    av_frame_free(&frame);      // av_frame_alloc
    avcodec_close(ctx);         // avcodec_open2
    avcodec_free_context(&ctx); // avcodec_alloc_context3

    // Make data_hq and data_lq
    bFirstSoundIsBetter = rand() % 2;

    if (bTestingSamplerate) {
      if (uiFactorHQ != samplingrate) {
        convertSamplingRate(data_original, data_hq, samplingrate, uiFactorHQ, channel_count);
      }
      else {
        data_hq = data_original;
      }

      convertSamplingRate(data_original, data_lq, samplingrate, uiFactorLQ, channel_count);
    }
    else {
      if (uiFactorHQ != bitdepth) {
        convertBitdepth(data_original, data_hq, bitdepth, uiFactorHQ);
      }
      else {
        data_hq = data_original;
      }

      convertBitdepth(data_original, data_lq, bitdepth, uiFactorLQ);
    }

    data_original.clear();

    result = true;
  }

READ_SOUND_CLEANUP:
  avformat_close_input(&avf_context);   // avformat_open_input
  avformat_free_context(avf_context);   // avformat_alloc_context

  return result;
}

uint32_t SongSession::getSamplingrate() {
  return samplingrate;
}

uint8_t SongSession::getBitdepth() {
  return 24;
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
    current_data = &data_lq;
    byte_per_sample = bTestingSamplerate ? 3 : (uiFactorLQ >> 3);
    current_freq = bTestingSamplerate ? uiFactorLQ : samplingrate;
    spec.sampleFormat = byte_per_sample == 3 ? paInt24 : (byte_per_sample == 2 ? paInt16 : paUInt8);
  }
  else {
    current_data = &data_hq;
    byte_per_sample = bTestingSamplerate ? 3 : (uiFactorHQ >> 3);
    current_freq = bTestingSamplerate ? uiFactorHQ : samplingrate;
    spec.sampleFormat = byte_per_sample == 3 ? paInt24 : (byte_per_sample == 2 ? paInt16 : paUInt8);
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

void SongSession::getTestResult(bool &answer) {
  answer = bFirstSoundIsBetter;
}

int SongSession::fill_audio(const void *inbuf, void *outbuf, unsigned long frames_per_buf, const PaStreamCallbackTimeInfo* time, PaStreamCallbackFlags flags, void *userdata) {
  Q_UNUSED(inbuf);
  Q_UNUSED(time);
  Q_UNUSED(flags);
  Q_UNUSED(userdata);
  
  SongSession *pThis = (SongSession *)userdata;
  int total_frame_count = pThis->current_data->size() / pThis->byte_per_sample / pThis->spec.channelCount;
  int played_frame_count = pThis->audio_index / pThis->byte_per_sample / pThis->spec.channelCount;

  int frame_left = total_frame_count - played_frame_count;

  if (frame_left <= 0) {
    return paComplete;
  }

  int frame_to_copy = FFMIN((int)frames_per_buf, frame_left);
  int byte_to_copy = frame_to_copy * pThis->byte_per_sample * pThis->spec.channelCount;

  memcpy(outbuf, pThis->current_data->c_str() + pThis->audio_index, byte_to_copy);
  pThis->audio_index += byte_to_copy;

  return paContinue;
}

void SongSession::convertSamplingRate(std::string &src, std::string &dst, uint32_t src_freq, uint32_t dst_freq, uint32_t channel) {
  uint32_t step = src_freq / dst_freq;    // Always integer
  uint32_t count = src.size();
  uint32_t samplesize = 3 * channel;

  dst.resize(count / step);
  count = dst.size() / samplesize; // samples

  for (uint32_t i = 0; i < count; i++) {
    memcpy((char *)dst.c_str() + i * samplesize, src.c_str() + i * step * samplesize, samplesize);
  }
}

void SongSession::convertBitdepth(std::string &src, std::string &dst, uint32_t src_bits, uint32_t dst_bits) {
  uint32_t count = src.size();
  uint32_t dst_samplesize = dst_bits >> 3;
  uint32_t src_samplesize = src_bits >> 3;

  dst.resize(src.size() * dst_samplesize / src_samplesize);
  count = dst.size() / dst_samplesize; // samples

  for (uint32_t i = 0; i < count; i++) {
    memcpy((char *)dst.c_str() + i * dst_samplesize, src.c_str() + i * src_samplesize + (src_samplesize - dst_samplesize), dst_samplesize);
  }

  if (dst_samplesize == 1) {
    for (uint32_t i = 0; i < count; i++) {
      dst.at(i) += 0x80;
    }
  }
}
