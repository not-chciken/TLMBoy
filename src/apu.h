#pragma once
/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2025 chciken/Niko Zurstraßen
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

  static constexpr uint kSampleRate = 44000u;

  // APU IO registers.
  // Square 1.
  u8* reg_nr10;  // -PPP NSSS Sweep period, negate, shift
  u8* reg_nr11;  // DDLL LLLL Duty, Length load (64-L).
  u8* reg_nr12;  // VVVV APPP Starting volume, Envelope add mode, period.
  u8* reg_nr13;  // FFFF FFFF Frequency LSB.
  u8* reg_nr14;  // TL-- -FFF Trigger, Length enable, Frequency MSB.

  // Square 2.
  u8* reg_nr21;  // DDLL LLLL Duty, Length load (64-L).
  u8* reg_nr22;  // VVVV APPP Starting volume, Envelope add mode, period.
  u8* reg_nr23;  // FFFF FFFF Frequency LSB.
  u8* reg_nr24;  // TL-- -FFF Trigger, Length enable, Frequency MSB.

  // Wave.
  u8* reg_nr30;  // E--- ---- DAC power
  u8* reg_nr31;  // LLLL LLLL Length load (256-L)
  u8* reg_nr32;  // -VV- ---- Volume code (00=0%, 01=100%, 10=50%, 11=25%)
  u8* reg_nr33;  // FFFF FFFF Frequency LSB
  u8* reg_nr34;  // TL-- -FFF Trigger, Length enable, Frequency MSB

  // Noise.
  u8* reg_nr41;  // --LL LLLL Length load (64-L)
  u8* reg_nr42;  // VVVV APPP Starting volume, Envelope add mode, period
  u8* reg_nr43;  // SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
  u8* reg_nr44;  // TL-- ---- Trigger, Length enable

  // Control/Status.
  u8* reg_nr50;  //  ALLL BRRR Vin L enable, Left vol, Vin R enable, Right vol
  u8* reg_nr51;  //  NW21 NW21 Left enables, Right enables
  u8* reg_nr52;  //  P--- NW21 Power control/status, Channel length statuses

  u8* wave_table;  // 32 4-bit samples from 0xFF30 to 0xFF3F.

  struct Channel {
    bool length_enable;
    int volume;
    uint length_load;
  };

  struct Square : public Channel {
    bool envelope_mode;
    bool sweep_direction;
    uint duty;
    uint frequency;
    uint period;
    uint sweep_counter;
    uint sweep_step;
    uint sweep_period;

    void WriteDataIntoStream(Sint16* stream, int length);

   private:
    float tick_counter;
    float phase = 1.f;
  };

  Square square1;
  Square square2;

  struct Noise : public Channel {
    const uint divisor_table[8] = {8, 16, 32, 48, 64, 80, 96, 112};

    bool envelope_mode;
    uint divisor;
    uint frequency;
    uint lfsr_bits;
    uint lfsr_width;
    uint period;
    uint shift;
    u32 cpu_ticks_per_lfsr_sample;
    u32 lfsr_sample_length;  // In ns.
    u32 tick_cntr;

    void WriteDataIntoStream(Sint16* stream, int length);

   private:
    float capacitor;
    float Hipass(float sample);
    float DoLfsrTicks(int num_ticks);
  } noise;

  struct Wave : public Channel {
    bool dac_enable;
    uint period;
    uint sample_index;
    float tick_counter;

    void WriteDataIntoStream(Sint16* stream, int length, u8* wave_table);
  } wave;

  // SystemC interfaces.
  void start_of_simulation() override;
  tlm_utils::simple_initiator_socket<Apu, gb_const::kBusDataWidth> init_socket;

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
  SDL_AudioDeviceID audio_device_;
  SDL_AudioSpec audio_spec_;

  void DecrementLengths();
  void DoSweep();
  void UpdateEnvelopes();
};
