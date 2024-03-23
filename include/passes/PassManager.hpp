#pragma once

#include "Module.hpp"
#include "logging.hpp"

#include <memory>
#include <vector>

class Pass {
  public:
    Pass(Module *m) : m_(m) {}
    virtual ~Pass() = default;
    virtual void run() = 0;

  protected:
    Module *m_;
};

class PassManager {
  public:
    PassManager(Module *m) : m_(m) {}

    template <typename PassType, typename... Args>
    void add_pass(Args &&...args) {
        passes_.emplace_back(new PassType(m_, std::forward<Args>(args)...));
    }

    void run() {
      int num = 0;
        for (auto &pass : passes_) {
          LOG_DEBUG << "PassManager: run  " << num << " passes";
            pass->run();
            num++;
        }  
        
    }

  private:
    std::vector<std::unique_ptr<Pass>> passes_;
    Module *m_;
};
