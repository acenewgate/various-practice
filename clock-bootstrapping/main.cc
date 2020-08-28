#include <iostream>
#include <random>
#include <thread>

#include "clock_bootstrapping.h"

int main(void) {
  clock_bootstrapping::ClockBootstrapping cb({});

  uint64_t ideal_duration = 1e8;
  uint64_t rand_max = 5e6;

  int num_samples = 40000;

  uint64_t prev_t = 0;
  uint64_t prev_est_t = 0;
  auto systime_offset = std::chrono::steady_clock::now();

  for (int i = 0; i < num_samples; ++i) {
    std::random_device rd;
    std::uniform_int_distribution<uint64_t> dist(0, rand_max);

    auto systime = systime_offset + std::chrono::nanoseconds(ideal_duration * i + dist(rd));

    uint64_t est_time = 0;

    cb.Bootstrap(systime.time_since_epoch().count(), 0, &est_time);

    std::cerr << (systime.time_since_epoch().count() - prev_t) * 1e-9 << " ";
    std::cerr << (est_time - prev_est_t) * 1e-9 << " ";
    std::cerr << (systime.time_since_epoch().count() - est_time) * 1e-9 << std::endl;

    prev_t = systime.time_since_epoch().count();
    prev_est_t = est_time;
  }

  return 0;
}
