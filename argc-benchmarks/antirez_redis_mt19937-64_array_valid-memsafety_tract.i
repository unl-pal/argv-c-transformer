// SPDX-FileCopyrightText: Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura
// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project

extern void abort();
void reach_error();
extern unsigned long long __VERIFIER_nondet_ulonglong(void);
void __VERIFIER_assert(int cond) {
  if (!cond) {
    reach_error();
    abort();
  }
}
static unsigned long long mt[32];
static int mti = 32 + 1;
void init_genrand64(unsigned long long seed) {
  mt[0] = seed;
  for (mti = 1; mti < 32; mti++)
    mt[mti] =
        (6364136223846793005ULL * (mt[mti - 1] ^ (mt[mti - 1] >> 62)) + mti);
}
void init_by_array64(unsigned long long init_key[],
                     unsigned long long key_length) {
  unsigned long long i, j, k;
  init_genrand64(19650218ULL);
  i = 1;
  j = 0;
  k = (32 > key_length ? 32 : key_length);
  for (; k; k--) {
    mt[i] =
        (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * 3935559000370003845ULL)) +
        init_key[j] + j;
    i++;
    j++;
    if (i >= 32) {
      mt[0] = mt[32 - 1];
      i = 1;
    }
    if (j >= key_length)
      j = 0;
  }
  for (k = 32 - 1; k; k--) {
    mt[i] =
        (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * 2862933555777941757ULL)) -
        i;
    i++;
    if (i >= 32) {
      mt[0] = mt[32 - 1];
      i = 1;
    }
  }
  mt[0] = 1ULL << 63;
}
unsigned long long genrand64_int64(void) {
  int i;
  unsigned long long x;
  static unsigned long long mag01[2] = {0ULL, 0xB5026F5AA96619E9ULL};
  if (mti >= 32) {
    if (mti == 32 + 1)
      init_genrand64(5489ULL);
    for (i = 0; i < 32 - 16; i++) {
      x = (mt[i] & 0xFFFFFFFF80000000ULL) | (mt[i + 1] & 0x7FFFFFFFULL);
      mt[i] = mt[i + 16] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
    }
    for (; i < 32 - 1; i++) {
      x = (mt[i] & 0xFFFFFFFF80000000ULL) | (mt[i + 1] & 0x7FFFFFFFULL);
      mt[i] = mt[i + (16 - 32)] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
    }
    x = (mt[32 - 1] & 0xFFFFFFFF80000000ULL) | (mt[0] & 0x7FFFFFFFULL);
    mt[32 - 1] = mt[16 - 1] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
    mti = 0;
  }
  x = mt[mti++];
  x ^= (x >> 29) & 0x5555555555555555ULL;
  x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
  x ^= (x << 37) & 0xFFF7EEE000000000ULL;
  x ^= (x >> 43);
  return x;
}
long long genrand64_int63(void) { return (long long)(genrand64_int64() >> 1); }
double genrand64_real1(void) {
  return (genrand64_int64() >> 11) * (1.0 / 9007199254740991.0);
}
double genrand64_real2(void) {
  return (genrand64_int64() >> 11) * (1.0 / 9007199254740992.0);
}
double genrand64_real3(void) {
  return ((genrand64_int64() >> 12) + 0.5) * (1.0 / 4503599627370496.0);
}
int main(void) {
  unsigned long long init_key[2];
  for (int i = 0; i < 2; i++) {
    init_key[i] = __VERIFIER_nondet_ulonglong();
  }
  init_by_array64(init_key, 2);
  long long r63 = genrand64_int63();
  unsigned long long r64 = genrand64_int64();
  double real1 = genrand64_real1();
  double real2 = genrand64_real2();
  double real3 = genrand64_real3();
  return 0;
}
