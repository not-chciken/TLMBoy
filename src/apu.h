#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * This class implements the Game Boy's APU (Audio Processing Unit).
 ******************************************************************************/

#include "SDL2/SDL.h"
#include "common.h"
#include "debug.h"

struct Apu : public sc_module {
  SC_HAS_PROCESS(Apu);

  explicit Apu(sc_module_name name);
  ~Apu();

  void AudioLoop();

  // APU IO registers.
  // Square 1.
  u8* reg_nr10;
  u8* reg_nr11;
  u8* reg_nr12;
  u8* reg_nr13;
  u8* reg_nr14;

  // Square 2.
  u8* reg_nr21;  // DDLL LLLL Duty, Length load (64-L).
  u8* reg_nr22;  // VVVV APPP Starting volume, Envelope add mode, period.
  u8* reg_nr23;  // FFFF FFFF Frequency LSB.
  u8* reg_nr24;  // TL-- -FFF Trigger, Length enable, Frequency MSB.

  // Wave.
  u8* reg_nr30;
  u8* reg_nr31;
  u8* reg_nr32;
  u8* reg_nr33;
  u8* reg_nr34;

  // Noise.
  u8* reg_nr41;
  u8* reg_nr42;
  u8* reg_nr43;
  u8* reg_nr44;

  // Control/Status.
  u8* reg_nr50;
  u8* reg_nr51;
  u8* reg_nr52;

  // Wavde duty:
  // 0 (12.5%) = _-------
  // 1 (25%)   = __------
  // 2 (50%)   = ____----
  // 3 (75%)   = ______--
  u8 duty;

  // Writing into regX1 loads the counter with (64-length) (256-length for wave).
  // The internal counter decrements this to zero.
  // Clocked at 256Hz. Only counts down with length_enable = true.
  u8 square1_length_load;
  u8 square2_length_load;
  u16 wave_length_load;
  u8 noise_length_load;

  // Current volume, which is not exposed.
  i32 volume_sq1;
  i32 volume_sq2;
  i32 volume_ns;

  // Envelope: true = amplify, false = attenuate;
  bool envelope_add_mode;

  // Number of envelope sweep (n: 0-7) (If zero, stop envelope operation.)
  u8 period;

  u16 frequency;
  u32 real_frequency;

  bool trigger;

  // If true, internal counter starts decreasing.
  bool length_enable;

  // SystemC interfaces.
  void start_of_simulation() override;
  tlm_utils::simple_initiator_socket<Apu, gb_const::kBusDataWidth> init_socket;
  sc_in_clk clk;
  static constexpr i32 kSampleRate = 44000;

  sc_in<bool> sig_reload_length_square1_in;
  sc_in<bool> sig_reload_length_square2_in;
  sc_in<bool> sig_reload_length_wave_in;
  sc_in<bool> sig_reload_length_noise_in;
  sc_in<bool> sig_trigger_square1_in;
  sc_in<bool> sig_trigger_square2_in;
  sc_in<bool> sig_trigger_wave_in;
  sc_in<bool> sig_trigger_noise_in;

  void ReloadLengthSquare1();
  void ReloadLengthSquare2();
  void ReloadLengthWave();
  void ReloadLengthNoise();

  void TriggerEventSquare1();
  void TriggerEventSquare2();
  void TriggerEventWave();
  void TriggerEventNoise();

 protected:
  SDL_AudioSpec audio_spec_;
  SDL_AudioDeviceID audio_device_;

  void DecrementLengths();
  void UpdateEnvelopes();
};
