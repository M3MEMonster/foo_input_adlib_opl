#pragma once
#include "stdafx.h"

extern cfg_int  cfg_adlib_samplerate;   // index into sample rate table; default = 7 (49716 Hz)
extern cfg_int  cfg_adlib_core;         // 0=Harekiet's  1=Ken Silverman's  2=Jarek Burczynski's  3=Tatsuyuki Satoh's  4=Nuked OPL3
extern cfg_bool cfg_adlib_surround;     // true=enable CSurroundopl stereo harmonic effect

inline constexpr int  default_sample_rate_idx = 7;     // 49716 Hz
inline constexpr int  default_core_idx = 0;     // Harekiet's
inline constexpr bool default_is_surround = false;  // surround off by default

inline const unsigned sample_rate_table[] = { 8000,11025,16000,22050,32000,44100,48000,49716,64000,88200,96000 };
inline constexpr int table_count = (int)sizeof(sample_rate_table) / (int)sizeof(sample_rate_table[0]);
inline const unsigned DOS_codepage_table[] = { 437,866,850,852,855,737,775,860,861,862,863,864,865,867,868,869 };