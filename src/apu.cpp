/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include "apu.h"

#include <algorithm>

Apu::Apu(sc_module_name name)
    : sc_module(name),
      sig_reload_length_square1_in("sig_reload_length_square1_in"),
      sig_reload_length_square2_in("sig_reload_length_square2_in"),
      sig_reload_length_wave_in("sig_reload_length_wave_in"),
      sig_reload_length_noise_in("sig_reload_length_noise_in"),
      sig_trigger_square1_in("sig_trigger_square1_in"),
      sig_trigger_square2_in("sig_trigger_square2_in"),
      sig_trigger_wave_in("sig_trigger_wave_in"),
      sig_trigger_noise_in("sig_trigger_noise_in") {
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

  SC_METHOD(TriggerEventSquare1);
  sensitive << sig_trigger_square1_in;
  dont_initialize();
  SC_METHOD(TriggerEventSquare2);
  sensitive << sig_trigger_square2_in;
  dont_initialize();
  SC_METHOD(TriggerEventWave);
  sensitive << sig_trigger_wave_in;
  dont_initialize();
  SC_METHOD(TriggerEventNoise);
  sensitive << sig_trigger_noise_in;
  dont_initialize();
}

Apu::~Apu() {
  SDL_CloseAudioDevice(audio_device_);
  SDL_Quit();
}

void Apu::Osc::WriteDataIntoStream(Sint16* stream, int length) {
    int period_length = (double)kSampleRate / frequency;  // use real_frequency

    if (length_load == 0)
        return;

    int duty_stop = (period_length / 8);
    switch (duty) {
    case 1:
      duty_stop *= 2;
      break;
    case 2:
      duty_stop *= 4;
      break;
    case 3:
      duty_stop *= 6;
      break;
    }

    for (int j = 0; j < length; ++j, ++sample_cntr) {
      int x = sample_cntr % period_length;
      if (x < duty_stop)
        stream[j] += (float)SDL_MIN_SINT16 * (((float)volume) / 60.f);
      else
        stream[j] += (float)SDL_MAX_SINT16 * (((float)volume) / 60.f);
    }
}

void Apu::Noise::DoLfsrTick(int num_ticks) {
  for (int i = 0; i < num_ticks; ++i) {
    uint val = !((lfsr_bits & 1) ^ ((lfsr_bits >> 1) & 1));
    lfsr_output = lfsr_bits & 1;
    lfsr_bits >>= 1;
    lfsr_bits |= (val << lfsr_width);
  }
}

void Apu::Noise::WriteDataIntoStream(Sint16* stream, int length) {
  u32 stream_sample_length = gb_const::kClockCycleFrequency / kSampleRate;
  lfsr_sample_length = 1000000000ull / lfsr_sample_rate;

  if (length_load == 0)
    return;

  for (int i = 0; i < length; ++i) {
    tick_cntr += stream_sample_length;
    uint lfsr_ticks = tick_cntr / lfsr_sample_length;
    DoLfsrTick(lfsr_ticks);
    tick_cntr -= lfsr_ticks * lfsr_sample_length;

    if (lfsr_output)
       stream[i] += (float)SDL_MIN_SINT16 * (((float)volume) / 60.f);
    else
       stream[i] += (float)SDL_MAX_SINT16 * (((float)volume) / 60.f);
  }
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

    for (int j = 0; j < length; ++j) {
      buffer[j] = 0;
    }

    apu->square1.duty = *(apu->reg_nr11) >> 6;
    u8 frequency_lsb = *(apu->reg_nr13);
    u8 frequency_msb = *(apu->reg_nr14) & 0b111u;
    u16 frequency = static_cast<u16>(frequency_msb) << 8 | static_cast<u16>(frequency_lsb);
    apu->square1.frequency = 131072u / (2048u - frequency);  // Lowest frequency = 64Hz.
    apu->square1.WriteDataIntoStream((Sint16*)buffer, length / 2);


    apu->square2.duty = *(apu->reg_nr21) >> 6;
    frequency_lsb = *(apu->reg_nr23);
    frequency_msb = *(apu->reg_nr24) & 0b111u;
    frequency = static_cast<u16>(frequency_msb) << 8 | static_cast<u16>(frequency_lsb);
    apu->square2.frequency = 131072u / (2048u - frequency);  // Lowest frequency = 64Hz.
    apu->square2.WriteDataIntoStream((Sint16*)buffer, length / 2);

    apu->noise.divisor = apu->noise.divisor_table[*(apu->reg_nr43) >> 4];
    apu->noise.lfsr_width = (((*(apu->reg_nr43)) >> 3) & 1) ? 7 : 15;
    apu->noise.shift = *(apu->reg_nr43) & 111;
    apu->noise.lfsr_sample_rate = gb_const::kClockCycleFrequency / (apu->noise.divisor << apu->noise.shift);
    apu->noise.WriteDataIntoStream((Sint16*)buffer, length / 2);


    // int square2_duty_stop = (period_length / 8);
    // switch (apu->square2.duty) {
    // case 1:
    //   square2_duty_stop *= 2;
    //   break;
    // case 2:
    //   square2_duty_stop *= 4;
    //   break;
    // case 3:
    //   square2_duty_stop *= 6;
    //   break;
    // }

    // Sint16* stream = (Sint16*)buffer;
    // for (int j = 0; j < length / 2; ++j, ++i) {
    //   int x = i % period_length;
    //   Sint16 sample;
    //   if (x < square2_duty_stop)
    //     sample = SDL_MIN_SINT16 / (((float)apu->square2.volume) / 15.f);
    //   else
    //     sample = SDL_MAX_SINT16 / (((float)apu->square2.volume) / 15.f);
    //   stream[j] = sample;
    //   if (apu->square2.length_load == 0)
    //     stream[j] = 0;
    // }
  };

  audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &audio_spec_, nullptr, 0);
  SDL_PauseAudioDevice(audio_device_, 0);

  while (true) {
    DecrementLengths();
    wait(16384);
    DecrementLengths();
    // DoSweep();
    wait(16384);
    DecrementLengths();
    wait(16384);
    DecrementLengths();
    // DoSweep();
    wait(8192);
    UpdateEnvelopes();
    wait(8192);
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
  square1.length_enable = *reg_nr14 & 0b01000000u;
  square2.length_enable = *reg_nr24 & 0b01000000u;
  bool wave_length_enable = *reg_nr34 & 0b01000000u;
  noise.length_enable = *reg_nr44 & 0b01000000u;

  if (square1.length_enable && (square1.length_load > 1u))
    --square1.length_load;
  if (square2.length_enable && (square2.length_load > 1u))
    --square2.length_load;
  if (wave_length_enable && (wave_length_load > 1u))
    --wave_length_load;
  if (noise.length_enable && (noise.length_load > 1u))
    --noise.length_load;
}

void Apu::UpdateEnvelopes() {
  static u32 i = 0;
  square1.envelope_mode = *reg_nr12 & 0b1000u;
  square1.period = *reg_nr12 & 0b111u;

  if (square1.period) {
    if (!(i % square1.period)) {
      if (square1.envelope_mode)
        square1.volume = std::min(square1.volume + 1, 15);
      else
        square1.volume = std::max(square1.volume - 1, 0);
    }
  }

  square2.envelope_mode = *reg_nr22 & 0b1000u;
  square2.period = *reg_nr22 & 0b111u;

  if (square2.period) {
    if (!(i % square2.period)) {
      if (square2.envelope_mode)
        square2.volume = std::min(square2.volume + 1, 15);
      else
        square2.volume = std::max(square2.volume - 1, 0);
    }
  }

  noise.envelope_mode = *reg_nr42 & 0b1000u;
  noise.period = *reg_nr42 & 0b111u;

  if (noise.period) {
    if (!(i % noise.period)) {
      if (noise.envelope_mode)
        noise.volume = std::min(noise.volume + 1, 15);
      else
        noise.volume = std::max(noise.volume - 1, 0);
    }
  }

  ++i;
}

void Apu::ReloadLengthSquare1() {
  square1.length_load = 64u - (*reg_nr11 & 0b111111u);
}

void Apu::ReloadLengthSquare2() {
  square2.length_load = 64u - (*reg_nr21 & 0b111111u);
}

void Apu::ReloadLengthWave() {
  wave_length_load = 256u - (*reg_nr31);
}

void Apu::ReloadLengthNoise() {
  noise.length_load = 64u - (*reg_nr41 & 0b111111u);
}

void Apu::TriggerEventSquare1() {
  if (square1.length_load == 0)
    square1.length_load = 64;

  square1.volume = *reg_nr12 >> 4;
  //  Channel is enabled (see length counter).
  // If length counter is zero, it is set to 64 (256 for wave channel). DONE
  // Frequency timer is reloaded with period.
  // Volume envelope timer is reloaded with period.
  // Channel volume is reloaded from NRx2. DONE
  // Noise channel's LFSR bits are all set to 1.
  // Wave channel's position is set to 0 but sample buffer is NOT refilled.
  // Square 1's sweep does several things (see frequency sweep).
}

void Apu::TriggerEventSquare2() {
  if (square2.length_load == 0)
    square2.length_load = 64;

  square2.volume = *reg_nr22 >> 4;
}

void Apu::TriggerEventWave() {
  if (wave_length_load == 0)
    wave_length_load = 256;
}

void Apu::TriggerEventNoise() {
  if (noise.length_load == 0)
    noise.length_load = 64;

  noise.volume = *reg_nr42 >> 4;
}
