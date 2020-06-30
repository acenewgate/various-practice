#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <vector>

std::mutex mtx;
std::condition_variable cv;
std::string task_list[4] = {};
std::queue<std::string> task_queue;

class Guard {
 public:
  Guard(std::thread& t) : t_(t) {}
  ~Guard() {
    if (t_.joinable()) t_.join();
  }
  Guard(const Guard& guard) = delete;
  Guard& operator=(const Guard& guard) = delete;

 private:
  std::thread& t_;
};

class BossWorkerExample {
 public:
  BossWorkerExample() {
    std::cout << "Boss Worker Example\n";
    std::thread t1(&BossWorkerExample::Boss, this);
    std::thread t2(&BossWorkerExample::Worker, this);

    Guard g1(t1), g2(t2);
  }

 private:
  void Boss() {
    for (int i = 0; i < 10; i++) {
      AddRandomTask();
      TakeBreak();
    }
    AddTask("Go Home");
  }

  void Worker() {
    while (true) {
      std::unique_lock<std::mutex> lck(mtx_);
      cv_.wait(lck, [this]() { return !task_queue_.empty(); });
      auto task = task_queue_.front();
      std::cout << "Doing task: " << task << std::endl;
      task_queue_.pop();
      if (task == "Go Home") break;
    }
  }
  void AddTask(const std::string& task) {
    std::lock_guard<std::mutex> guard(mtx_);
    std::cout << "Delegating task: " << task << std::endl;
    task_queue_.push(task);
    cv_.notify_one();
  }
  void AddRandomTask() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, 3);
    AddTask(task_list_[distribution(generator)]);
  }
  void TakeBreak() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

  std::condition_variable cv_;
  std::mutex mtx_;
  std::string task_list_[4] = {"Code Review", "Debugging", "Feature Patch", "Integration"};
  std::queue<std::string> task_queue_;
};

class HotelExample {
 public:
  enum class ROOM { CLEAN, DIRTY };
  HotelExample() {
    std::cout << "Hotel Example\n";
    std::thread t1(&HotelExample::Stay, this);
    std::thread t2(&HotelExample::Clean, this);

    Guard g1(t1), g2(t2);
  }

 private:
  void Stay() {
    {
      std::lock_guard<std::mutex> guard(mtx_);
      std::cout << "Stayed one night and room is dirty\n";
      condition_ = ROOM::DIRTY;
    }
    std::cout << "Asking the maid to clean the room\n";
    cv_.notify_one();
    {
      std::unique_lock<std::mutex> lock(mtx_);
      cv_.wait(lock, [this]() { return condition_ == ROOM::CLEAN; });
      std::cout << "Decide to stay for one more night\n";
    }
  }
  void Clean() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]() { return condition_ == ROOM::DIRTY; });
    condition_ = ROOM::CLEAN;
    std::cout << "Room is cleaned by maid\n";
    cv_.notify_one();
  }

  std::condition_variable cv_;
  std::mutex mtx_;
  ROOM condition_ = ROOM::CLEAN;
};

class DeadlockExample {
 public:
  DeadlockExample() {
    //      RunDeadlock();
    //      RunWithoutLockByOrder();
    RunWithoutLockByLock();
  }

 private:
  void RunDeadlock() {
    std::cout << "Run Deadlock\n";
    std::thread t1([this]() {
      std::lock_guard<std::mutex> g1(first_mtx_);
      first_data_ = "thread 1 is exclusively accessing the first data";
      std::lock_guard<std::mutex> g2(second_mtx_);
      second_data_ = "thread 1 is exclusively accessing the second data";
    });
    std::thread t2([this]() {
      std::lock_guard<std::mutex> g1(second_mtx_);
      second_data_ = "thread 2 is exclusively accessing the second data";
      std::lock_guard<std::mutex> g2(first_mtx_);
      first_data_ = "thread 2 is exclusively accessing the first data";
    });
    Guard g1(t1), g2(t2);
  }
  void RunWithoutLockByOrder() {
    std::cout << "Run without deadlock by order\n";
    std::thread t1([this]() {
      std::lock_guard<std::mutex> g1(first_mtx_);
      first_data_ = "thread 1 is exclusively accessing the first data";
      std::lock_guard<std::mutex> g2(second_mtx_);
      second_data_ = "thread 1 is exclusively accessing the second data";
    });
    std::thread t2([this]() {
      std::lock_guard<std::mutex> g1(first_mtx_);
      first_data_ = "thread 2 is exclusively accessing the first data";
      std::lock_guard<std::mutex> g2(second_mtx_);
      second_data_ = "thread 2 is exclusively accessing the second data";
    });
    Guard g1(t1), g2(t2);
  }
  void RunWithoutLockByLock() {
    std::cout << "Run without deadlock by std::lock\n";
    std::thread t1([this]() {
      std::lock(first_mtx_, second_mtx_);
      std::lock_guard<std::mutex> g1(first_mtx_, std::adopt_lock);
      first_data_ = "thread 1 is exclusively accessing the first data";
      std::lock_guard<std::mutex> g2(second_mtx_, std::adopt_lock);
      second_data_ = "thread 1 is exclusively accessing the second data";
    });
    std::thread t2([this]() {
      std::lock(first_mtx_, second_mtx_);
      std::lock_guard<std::mutex> g1(second_mtx_, std::adopt_lock);
      second_data_ = "thread 2 is exclusively accessing the second data";
      std::lock_guard<std::mutex> g2(first_mtx_, std::adopt_lock);
      first_data_ = "thread 2 is exclusively accessing the first data";
    });
    Guard g1(t1), g2(t2);
  }

  std::string first_data_;
  std::string second_data_;

  std::mutex first_mtx_;
  std::mutex second_mtx_;
};

class DataRaceExample {
 public:
  DataRaceExample() {
    for (int i = 0; i < num_run_; i++) RunDataRace();
    for (int i = 0; i < num_run_; i++) RunWithoutRace();
  }

 private:
  void RunDataRace() {
    std::cout << "Run Data Race\n";
    std::cout << "Two threads will race for std::cout\n";
    std::thread t1([this]() {
      for (int i = 0; i < num_print_; i++) std::cout << "*";
      std::cout << std::endl;
    });
    std::thread t2([this]() {
      for (int i = 0; i < num_print_; i++) std::cout << "#";
      std::cout << std::endl;
    });
    Guard g1(t1), g2(t2);
  }
  void RunWithoutRace() {
    std::cout << "Run Without Race\n";
    std::cout << "std::cout will be accessed exculsively\n";
    std::thread t1([this]() {
      std::lock_guard<std::mutex> guard(mtx_);
      for (int i = 0; i < num_print_; i++) std::cout << "*";
      std::cout << std::endl;
    });
    std::thread t2([this]() {
      std::lock_guard<std::mutex> guard(mtx_);
      for (int i = 0; i < num_print_; i++) std::cout << "#";
      std::cout << std::endl;
    });
    Guard g1(t1), g2(t2);
  }
  int num_print_ = 100;
  int num_run_ = 10;
  std::mutex mtx_;
};

void MakeBreak(int miliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(miliseconds));
}

int Temperature() {
  std::cout << "Husband: Hm, is the weather "
            << "forecast in the newspaper?\n"
            << "         Eh, we don't "
            << "have a newspaper at home..." << std::endl;
  MakeBreak(2);
  std::cout << "Husband: I will look it up on the internet!" << std::endl;
  MakeBreak(2);
  std::cout << "Husband: Here it is, "
            << "it says tomorrow will be 40." << std::endl;
  return 40;
}

int main(void) {
  std::cout << "Wife:    Tomorrow, we are going on a picnic.\n"
            << "         What will be the weather...\n"
            << "         \"What will be the "
            << "temperature tomorrow?\"" << std::endl;

  std::future<int> answer = std::async(std::launch::deferred, Temperature);

  MakeBreak(2);

  std::cout << "Wife:    I should pack for tomorrow." << std::endl;

  MakeBreak(2);

  std::cout << "Wife:    Hopefully my husband can figure out the weather soon." << std::endl;

  int temp = answer.get();

  std::cout << "Wife:    Finally, tomorrow will be " << temp << "... Em...\n"
            << "         \"In which units is the answer?\"" << std::endl;

  return 0;
}
