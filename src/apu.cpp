/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include "apu.h"

Apu::Apu(sc_module_name name)
    : sc_module(name),
      sig_reload_length_square1_in("sig_reload_length_square1_in"),
      sig_reload_length_square2_in("sig_reload_length_square2_in"),
      sig_reload_length_wave_in("sig_reload_length_wave_in"),
      sig_reload_length_noise_in("sig_reload_length_noise_in") {
  SC_CTHREAD(AudioLoop, clk);
  dont_initialize();
  SC_METHOD(ReloadLengthSquare1);
  sensitive << sig_reload_length_square1_in;
  dont_initialize();
  SC_METHOD(ReloadLengthSquare2);
  sensitive << sig_reload_length_square2_in;
  dont_initialize();
  SC_METHOD(ReloadLengthWave);
  sensitive << sig_reload_length_wave_in;
  dont_initialize();
  SC_METHOD(ReloadLengthNoise);
  sensitive << sig_reload_length_noise_in;
  dont_initialize();
}

Apu::~Apu() {
  SDL_CloseAudioDevice(audio_device_);
  SDL_Quit();
}

void Apu::AudioLoop() {
  SDL_Init(SDL_INIT_AUDIO);
  audio_spec_.freq = kSampleRate;
  audio_spec_.format = AUDIO_S16SYS;
  audio_spec_.channels = 1;  // TODO: Has to be 2.
  audio_spec_.samples = 1024;
  audio_spec_.userdata = (void*)this;
  audio_spec_.callback = [](void* userData, Uint8* buffer, int length) {
    Apu* apu = (Apu*)userData;
    static uint64_t i = 0;

    u8 square1_duty = *(apu->reg_nr11) >> 6;
    u8 square2_duty = *(apu->reg_nr21) >> 6;

    u8 frequency_lsb = *(apu->reg_nr23);
    u8 frequency_msb = *(apu->reg_nr24) & 0b111u;
    u16 frequency = static_cast<u16>(frequency_msb) << 8 | static_cast<u16>(frequency_lsb);
    u32 real_frequency = 131072u / (2048u - frequency);  // Lowest frequency = 64Hz.

    int period_length = (double)kSampleRate / real_frequency;  // use real_frequency

    int square1_duty_stop = (period_length / 8);
    switch (square1_duty) {
    case 1:
      square1_duty_stop *= 2;
      break;
    case 2:
      square1_duty_stop *= 4;
      break;
    case 3:
      square1_duty_stop *= 6;
      break;
    }

    Sint16* stream = (Sint16*)buffer;
    for (int j = 0; j < length / 2; ++j, ++i) {
      int x = i % period_length;
      Sint16 sample;
      if (x < square1_duty_stop)
        sample = SDL_MIN_SINT16;
      else
        sample = SDL_MAX_SINT16;
      stream[j] = sample;
      if (apu->square1_length_load == 0)
        stream[j] = 0;
    }
  };

  audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &audio_spec_, nullptr, 0);
  SDL_PauseAudioDevice(audio_device_, 0);

  while (true) {
    DecrementLengths();
    wait(gb_const::kNsPerClkCycle * 16384);
    DecrementLengths();
    // DoSweep();
    wait(gb_const::kNsPerClkCycle * 16384);
    DecrementLengths();
    wait(gb_const::kNsPerClkCycle * 16384);
    DecrementLengths();
    // DoSweep();
    wait(gb_const::kNsPerClkCycle * 8192);
    // Do Envelope();
    wait(gb_const::kNsPerClkCycle * 8192);
  }
}

void Apu::start_of_simulation() {
  tlm::tlm_dmi dmi_data;
  u8* data_ptr;
  uint dummy;
  auto payload = MakeSharedPayloadPtr(tlm::TLM_READ_COMMAND, 0x0FFF, reinterpret_cast<void*>(&dummy));
  payload->set_address(0xFF10);  //  Start of IO registers for sound.

  if (init_socket->get_direct_mem_ptr(*payload, dmi_data)) {
    assert(dmi_data.get_start_address() == 0);
    assert(dmi_data.get_end_address() >= 0x16);
    data_ptr = reinterpret_cast<u8*>(dmi_data.get_dmi_ptr());
    reg_nr10 = &data_ptr[0xFF10 - 0xFF10];
    reg_nr11 = &data_ptr[0xFF11 - 0xFF10];
    reg_nr12 = &data_ptr[0xFF12 - 0xFF10];
    reg_nr13 = &data_ptr[0xFF13 - 0xFF10];
    reg_nr14 = &data_ptr[0xFF14 - 0xFF10];

    reg_nr21 = &data_ptr[0xFF16 - 0xFF10];
    reg_nr22 = &data_ptr[0xFF17 - 0xFF10];
    reg_nr23 = &data_ptr[0xFF18 - 0xFF10];
    reg_nr24 = &data_ptr[0xFF19 - 0xFF10];

    reg_nr30 = &data_ptr[0xFF1A - 0xFF10];
    reg_nr31 = &data_ptr[0xFF1B - 0xFF10];
    reg_nr32 = &data_ptr[0xFF1C - 0xFF10];
    reg_nr33 = &data_ptr[0xFF1D - 0xFF10];
    reg_nr34 = &data_ptr[0xFF1E - 0xFF10];

    reg_nr41 = &data_ptr[0xFF20 - 0xFF10];
    reg_nr42 = &data_ptr[0xFF21 - 0xFF10];
    reg_nr43 = &data_ptr[0xFF22 - 0xFF10];
    reg_nr44 = &data_ptr[0xFF23 - 0xFF10];

    reg_nr50 = &data_ptr[0xFF24 - 0xFF10];
    reg_nr51 = &data_ptr[0xFF25 - 0xFF10];
    reg_nr52 = &data_ptr[0xFF26 - 0xFF10];
  } else {
    throw std::runtime_error("Could not get DMI for IO registers!");
  }
}

void Apu::DecrementLengths() {
  bool square1_length_enable = *reg_nr14 & 0b01000000u;
  bool square2_length_enable = *reg_nr24 & 0b01000000u;
  bool wave_length_enable = *reg_nr34 & 0b01000000u;
  bool noise_length_enable = *reg_nr44 & 0b01000000u;

  if (square1_length_enable && (square1_length_load > 1u))
    --square1_length_load;
  if (square2_length_enable && (square2_length_load > 1u))
    --square2_length_load;
  if (wave_length_enable && (wave_length_load > 1u))
    --wave_length_load;
  if (noise_length_enable && (noise_length_load > 1u))
    --noise_length_load;
}

void Apu::ReloadLengthSquare1() {
  square1_length_load = 64u - (*reg_nr11 & 0b111111u);
}

void Apu::ReloadLengthSquare2() {
  square2_length_load = 64u - (*reg_nr21 & 0b111111u);
}

void Apu::ReloadLengthWave() {
  wave_length_load = 256u - (*reg_nr31);
}

void Apu::ReloadLengthNoise() {
  noise_length_load = 64u - (*reg_nr41 & 0b111111u);
}
