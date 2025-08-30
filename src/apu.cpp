/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko Zurstraßen
 ******************************************************************************/

#include "apu.h"

#include <algorithm>

Apu::Apu(sc_module_name name)
    : sc_module(name),
      init_socket("init_socket"),
      sig_reload_length_square1_in("sig_reload_length_square1_in"),
      sig_reload_length_square2_in("sig_reload_length_square2_in"),
      sig_reload_length_wave_in("sig_reload_length_wave_in"),
      sig_reload_length_noise_in("sig_reload_length_noise_in"),
      sig_trigger_square1_in("sig_trigger_square1_in"),
      sig_trigger_square2_in("sig_trigger_square2_in"),
      sig_trigger_wave_in("sig_trigger_wave_in"),
      sig_trigger_noise_in("sig_trigger_noise_in") {
  SC_THREAD(AudioLoop);

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

void Apu::Square::WriteDataIntoStream(Sint16* stream, int length) {
  if (length_load == 0 && length_enable == 1)
    return;

  if (volume == 0)
    return;

  float periods_per_sample = (131072 / (float)kSampleRate) / (float)(2048 - frequency);

  float fduty;

  switch (duty) {
  case 0:
    fduty = 0.125f;
    break;
  case 1:
    fduty = 0.25f;
    break;
  case 2:
    fduty = 0.5f;
    break;
  case 3:
    fduty = 0.75f;
    break;
  default:
    fduty = 0.5f;
  }

  if (periods_per_sample >= fduty)
    return;

  for (int i = 0; i < length; ++i) {
    float d = phase == 1.f ? fduty : (1.f - fduty);

    if (tick_counter < d) {
      stream[i] += (float)SDL_MAX_SINT16 * phase * (((float)volume) / 60.f);
    } else {
      tick_counter -= d;
      float factor = tick_counter / periods_per_sample;
      phase = -phase;
      stream[i] += (2.f * factor - 1.f) * (float)SDL_MAX_SINT16 * phase * (((float)volume) / 60.f);
    }

    tick_counter += periods_per_sample;
  }
}

float Apu::Noise::DoLfsrTicks(int num_ticks) {
  static float sum = 0.f;

  if (num_ticks == 0)
    return sum;

  sum = 0.f;

  for (int i = 0; i < num_ticks; ++i) {
    int val = !((lfsr_bits & 1) ^ ((lfsr_bits >> 1) & 1));
    lfsr_bits |= (val << lfsr_width);
    lfsr_bits >>= 1;
    sum += (float)(lfsr_bits & 1);
  }

  return sum / (float)num_ticks;
}

void Apu::Noise::WriteDataIntoStream(Sint16* stream, int length) {
  constexpr u32 stream_sample_length =
      gb_const::kClockCycleFrequency / kSampleRate;  // Length of a stream sample in CPU cycles.

  if (length_load == 0 && length_enable == 1)
    return;

  for (int i = 0; i < length; ++i) {
    tick_cntr += stream_sample_length;
    uint lfsr_ticks = tick_cntr / cpu_ticks_per_lfsr_sample;
    float sample = DoLfsrTicks(lfsr_ticks);
    sample = Hipass(sample);
    tick_cntr -= lfsr_ticks * cpu_ticks_per_lfsr_sample;
    stream[i] += (float)SDL_MAX_SINT16 * (2.f * sample - 1.f) * (((float)volume) / 60.f);
  }
}

float Apu::Noise::Hipass(float sample) {
  float out = sample - capacitor;
  capacitor = sample - out * 0.996f;
  return out;
}

void Apu::Wave::WriteDataIntoStream(Sint16* stream, int length, u8* wave_table) {
  if (length_load == 0 && length_enable == 1)
    return;

  if (!dac_enable)
    return;

  float fvolume;

  switch (volume) {
  case 0:
    fvolume = 0.f;
    break;
  case 1:
    fvolume = 1.f;
    break;
  case 2:
    fvolume = 0.5f;
    break;
  case 3:
    fvolume = 0.25f;
    break;
  default:
    fvolume = 0.f;
    break;
  }

  if (fvolume == 0.f)
    return;

  float entries_per_sample = (2097152.f / (float)kSampleRate) / (float)(2048.f - period);

  if (entries_per_sample >= 1.f)
    return;

  for (int i = 0; i < length; ++i) {
    tick_counter += entries_per_sample;
    if (tick_counter >= 1.f) {
      ++sample_index;
      tick_counter -= 1.f;
      sample_index = sample_index > 31 ? 0 : sample_index;
    }
    u8 sample = wave_table[sample_index / 2];
    if (sample_index % 2u) {
      sample &= 0xfu;
    } else {
      sample = (sample & 0xf0u) >> 4;
    }
    stream[i] += (float)SDL_MAX_SINT16 * (2.f * (float)sample / 15.f - 1.f) * (fvolume * 0.25f);
  }
}

void Apu::AudioLoop() {
  SDL_Init(SDL_INIT_AUDIO);
  audio_spec_.freq = kSampleRate;
  audio_spec_.format = AUDIO_S16SYS;
  audio_spec_.channels = 1;  // TODO: Has to be 2.
  audio_spec_.samples = 512;
  audio_spec_.userdata = (void*)this;
  audio_spec_.callback = [](void* userData, Uint8* buffer, int length) {
    Apu* apu = (Apu*)userData;

    std::fill(buffer, buffer + length, 0);

    if (!(*(apu->reg_nr52) >> 7))
      return;  // Nothing to do if APU is turned off.

    apu->square1.duty = *(apu->reg_nr11) >> 6;
    u8 frequency_lsb = *(apu->reg_nr13);
    u8 frequency_msb = *(apu->reg_nr14) & 0b111u;
    apu->square1.frequency = static_cast<u16>(frequency_msb) << 8 | static_cast<u16>(frequency_lsb);
    apu->square1.WriteDataIntoStream((Sint16*)buffer, length / 2);

    apu->square2.duty = *(apu->reg_nr21) >> 6;
    frequency_lsb = *(apu->reg_nr23);
    frequency_msb = *(apu->reg_nr24) & 0b111u;
    apu->square2.frequency = static_cast<u16>(frequency_msb) << 8 | static_cast<u16>(frequency_lsb);
    apu->square2.WriteDataIntoStream((Sint16*)buffer, length / 2);

    apu->wave.dac_enable = *(apu->reg_nr30) & 0x80u;
    apu->wave.volume = (*apu->reg_nr32 >> 5) & 0b11;
    frequency_lsb = *(apu->reg_nr33);
    frequency_msb = *(apu->reg_nr34) & 0b111u;
    apu->wave.period = static_cast<u16>(frequency_msb) << 8 | static_cast<u16>(frequency_lsb);
    apu->wave.WriteDataIntoStream((Sint16*)buffer, length / 2, apu->wave_table);

    apu->noise.divisor = apu->noise.divisor_table[*(apu->reg_nr43) & 0b111];
    apu->noise.lfsr_width = (((*(apu->reg_nr43)) >> 3) & 1) ? 7 : 15;
    apu->noise.shift = *(apu->reg_nr43) >> 4;
    apu->noise.cpu_ticks_per_lfsr_sample = (apu->noise.divisor << apu->noise.shift);  // CPU ticks per LFSR sample.
    apu->noise.WriteDataIntoStream((Sint16*)buffer, length / 2);
  };

  audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &audio_spec_, nullptr, 0);
  SDL_PauseAudioDevice(audio_device_, 0);

  while (true) {
    DecrementLengths();
    wait(16384 * gb_const::kNsPerClkCycle, sc_core::SC_NS);
    DecrementLengths();
    DoSweep();
    wait(16384 * gb_const::kNsPerClkCycle, sc_core::SC_NS);
    DecrementLengths();
    wait(16384 * gb_const::kNsPerClkCycle, sc_core::SC_NS);
    DecrementLengths();
    DoSweep();
    wait(8192 * gb_const::kNsPerClkCycle, sc_core::SC_NS);
    UpdateEnvelopes();
    wait(8192 * gb_const::kNsPerClkCycle, sc_core::SC_NS);
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

    wave_table = &data_ptr[0xFF30 - 0xFF10];
  } else {
    throw std::runtime_error("Could not get DMI for IO registers!");
  }
}

void Apu::DecrementLengths() {
  square1.length_enable = *reg_nr14 & 0b01000000u;
  square2.length_enable = *reg_nr24 & 0b01000000u;
  wave.length_enable = *reg_nr34 & 0b01000000u;
  noise.length_enable = *reg_nr44 & 0b01000000u;

  if (square1.length_enable && (square1.length_load >= 1u)) {
    --square1.length_load;
    if (square1.length_load == 0)
      *reg_nr52 &= ~kSquare1StatusMask;
  }
  if (square2.length_enable && (square2.length_load >= 1u)) {
    --square2.length_load;
    if (square2.length_load == 0)
      *reg_nr52 &= ~kSquare2StatusMask;
  }
  if (wave.length_enable && (wave.length_load >= 1u)) {
    --wave.length_load;
    if (wave.length_load == 0)
      *reg_nr52 &= ~kWaveStatusMask;
  }
  if (noise.length_enable && (noise.length_load >= 1u)) {
    --noise.length_load;
    if (noise.length_load == 0)
      *reg_nr52 &= ~kNoiseStatusMask;
  }
}

void Apu::DoSweep() {
  square1.sweep_step = *(reg_nr10) & 0b111;
  square1.sweep_direction = *(reg_nr10) & 0b1000;
  square1.sweep_period = (*(reg_nr10) >> 4) & 0b111;

  if (square1.sweep_period == 0)
    return;

  if (square1.sweep_counter >= square1.sweep_period) {
    const int dir = square1.sweep_direction ? -1 : +1;
    square1.frequency = square1.frequency + dir * square1.frequency / (1u << square1.sweep_step);

    if (square1.frequency > 2047u) {
      *reg_nr52 &= ~kSquare1StatusMask;  // Turn off if overflow.
    }

    square1.frequency = std::min(2047u, square1.frequency);
    square1.sweep_counter = 0;
    *(reg_nr13) = square1.frequency & 0xffu;
    *(reg_nr14) = (square1.frequency & 0x700u) >> 8;
  } else {
    ++square1.sweep_counter;
  }
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
  wave.length_load = 256u - (*reg_nr31);
}

void Apu::ReloadLengthNoise() {
  noise.length_load = 64u - (*reg_nr41 & 0b111111u);
}

void Apu::TriggerEventSquare1() {
  if (square1.length_load == 0)
    ReloadLengthSquare1();

  square1.volume = *reg_nr12 >> 4;

  if ((*reg_nr12 & 0xF8) != 0)
    *reg_nr52 |= kSquare1StatusMask;
}

void Apu::TriggerEventSquare2() {
  if (square2.length_load == 0)
    ReloadLengthSquare2();

  square2.volume = *reg_nr22 >> 4;

  if ((*reg_nr22 & 0xF8) != 0)
    *reg_nr52 |= kSquare2StatusMask;
}

void Apu::TriggerEventWave() {
  if (wave.length_load == 0)
    ReloadLengthWave();

  wave.volume = (*reg_nr32 >> 5) & 0b11;

  *reg_nr52 |= kWaveStatusMask;
}

void Apu::TriggerEventNoise() {
  if (noise.length_load == 0)
    ReloadLengthNoise();

  noise.volume = *reg_nr42 >> 4;
  noise.lfsr_bits = 0;

  if ((*reg_nr42 & 0xF8) != 0)
    *reg_nr52 |= kNoiseStatusMask;
}
