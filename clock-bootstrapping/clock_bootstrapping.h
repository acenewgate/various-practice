#ifndef CLOCK_BOOTSTRAPPING_H
#define CLOCK_BOOTSTRAPPING_H
#define MAX_TIMEPOINT 0xffffffffffffffff
#include <cassert>
#include <cinttypes>
#include <deque>
#include <iostream>
namespace clock_bootstrapping {
struct Parameters {
  bool use_fixed_duration = true;
  uint64_t duration = 100000000;  // nanosec
  int queue_length = 100;
};

class ClockBootstrapping {
 public:
  ClockBootstrapping(const Parameters& parameters) : parameters_(parameters) {}

  void Bootstrap(uint64_t wall_clock_time, uint64_t duration, uint64_t* const estimated_time) {
    if (parameters_.use_fixed_duration)
      monotonic_sensor_time_ = parameters_.duration * sequence_;
    else
      monotonic_sensor_time_ += duration;
    assert(wall_clock_time > monotonic_sensor_time_);
    uint64_t clock_domain_offset = wall_clock_time - monotonic_sensor_time_;
    queue_.push_back(clock_domain_offset);
    if (clock_domain_offset < min_domain_offset_) {
      min_domain_offset_ = clock_domain_offset;
    }
    if (queue_.size() == parameters_.queue_length + 1) {
      bool need_to_find_min = (queue_.front() == min_domain_offset_);
      queue_.pop_front();
      if (need_to_find_min) {
        min_domain_offset_ = MAX_TIMEPOINT;
        for (const auto& e : queue_) min_domain_offset_ = std::min(min_domain_offset_, e);
      }
    }

    *estimated_time = monotonic_sensor_time_ + min_domain_offset_;
    sequence_++;
  }

 private:
  const Parameters parameters_;
  std::deque<uint64_t> queue_;
  uint64_t min_domain_offset_ = MAX_TIMEPOINT;
  uint64_t monotonic_sensor_time_ = 0;
  int sequence_ = 0;
};

}  // namespace clock_bootstrapping

#endif
