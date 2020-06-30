#include <Eigen/Dense>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

struct LowLevelStr {  // CellLimits
  LowLevelStr(int x, int y) : x(x), y(y) {}
  int x;
  int y;
};

class LowLevel {  // MapLimits
 public:
  explicit LowLevel(const LowLevelStr& llstr) : llstr_(llstr), max_(Eigen::Vector2d(10.0, 10.0)) {}
  explicit LowLevel() : llstr_(100, 100), max_(Eigen::Vector2d(10.0, 10.0)) {}
  LowLevelStr llstr() const { return llstr_; }
  void Setllstr(const LowLevelStr& llstr) { llstr_ = llstr; }
  Eigen::Vector2d max() const { return max_; }
  void SetMax(const Eigen::Vector2d& max) { max_ = max; }
  double resolution() const { return resolution_; }
  void SetResolution(double resolution) { resolution_ = resolution; }

  LowLevel operator=(const LowLevel& rhs) {
    llstr_ = rhs.llstr();
    max_ = rhs.max();
    resolution_ = rhs.resolution();
    return (*this);
  }

 private:
  LowLevelStr llstr_;
  Eigen::Vector2d max_;
  double resolution_;
};

class MidLevel {  // ProbabilityGrid
 public:
  explicit MidLevel(const LowLevel& low_level) : low_level_(low_level) {}
  explicit MidLevel() {}
  const LowLevel& low_level() const { return low_level_; }
  const std::vector<int>& ivec() const { ivec_; }
  const std::vector<uint8_t>& uivec() const { uivec_; }
  const std::vector<std::pair<int, bool>>& pvec() const { pvec_; }
  void SetLowLevel(const LowLevel& low_level) { low_level_ = low_level; }
  void SetIvec(const std::vector<int>& ivec) { ivec_ = ivec; }
  void SetUivec(const std::vector<uint8_t>& uivec) { uivec_ = uivec; }
  void SetPvec(const std::vector<std::pair<int, bool>>& pvec) { pvec_ = pvec; }

 private:
  LowLevel low_level_;
  std::vector<int> ivec_;
  std::vector<uint8_t> uivec_;
  std::vector<std::pair<int, bool>> pvec_;
};

class HighLevel {  // Submap2D
 public:
  HighLevel(const LowLevel& low_level) : mid_level_(low_level) {}
  MidLevel mid_level() const { return mid_level_; }
  void SetMidLevel(const MidLevel& mid_level) { mid_level_ = mid_level; }

 private:
  MidLevel mid_level_;
};

class Segv {
 public:
  Segv() {}
  void Make() {
    LowLevel low_level;
    hp_vec_.emplace_back(std::make_shared<HighLevel>(low_level));
    MidLevel mid_level;
    std::vector<int> ivec;
    for (int i = 0; i < 3060500; ++i) ivec.push_back(i);
    std::vector<uint8_t> uivec;
    for (int i = 0; i < 10000; ++i) uivec.push_back(1u);
    std::vector<std::pair<int, bool>> pvec;
    for (int i = 0; i < 1000; ++i) pvec.emplace_back(std::make_pair(i, true));

    mid_level.SetIvec(ivec);
    mid_level.SetLowLevel(low_level);
    mid_level.SetUivec(uivec);
    mid_level.SetPvec(pvec);

    auto* hp = hp_vec_.back().get();
    hp->SetMidLevel(mid_level);
    std::cout << "Problem set\n";

    const HighLevel& high_level = *(hp_vec_.front().get());

    HighLevel high_level_stack(high_level.mid_level().low_level());
    std::cout << "Never reach here?\n";
    flag_ = true;
    std::cout << "flag set true\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    high_level_stack.SetMidLevel(high_level.mid_level());
  }
  void Reset() {
    while (true) {
      if (flag_) {
        std::cout << "Flag true. break\n";
        break;
      } else {
        std::cout << "Flag false forever.";
      }
    }
    std::cout << "hp_vec size: " << hp_vec_.size() << std::endl;
    hp_vec_.back().reset();
    hp_vec_.erase(hp_vec_.begin());
  }

 private:
  std::vector<std::shared_ptr<HighLevel>> hp_vec_;
  bool flag_ = false;
};

class Guard {
 public:
  Guard(std::thread& t) : t_(t) {}
  ~Guard() {
    if (t_.joinable()) t_.join();
  }
  Guard(const Guard& g) = delete;
  Guard& operator=(const Guard& g) = delete;

 private:
  std::thread& t_;
};

int main(void) {
  Segv segv;
  std::thread t1(&Segv::Make, &segv);
  std::thread t2(&Segv::Reset, &segv);

  Guard g1(t1), g2(t2);
  return 0;
}
