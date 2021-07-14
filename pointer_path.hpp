#ifndef SRT_POINTER_PATH_HPP
#define SRT_POINTER_PATH_HPP

#include <array>          // std::array<T,N>
#include <cstddef>        // std::ptrdiff_t, std::byte
#include <iostream>       // std::cerr
#include <vector>         // std::vector<T>, begin, end

#include <errno.h>        // errno
#include <stdio.h>        // popen, pclose, fgets
#include <stdlib.h>       // atoi
#include <string.h>       // strerror
#include <sys/uio.h>      // process_vm_readv, struct iovec

namespace {
  static pid_t pid;
}

void set_pid() {
  constexpr auto length = 20;
  auto line = std::array<char,length>{};

  // This is ugly, but it works.
  FILE* cmd = popen("pgrep Spyro-Win64-Shi","r");
  fgets(line.data(), length, cmd);
  pid = pid_t{ atoi(line.data()) };
  pclose(cmd);

  std::cout << "> pid: " << pid << '\n' << std::flush;
}

template <typename T>  // T is a POD
struct pointer_path {
  T value{};
  std::vector<std::ptrdiff_t> ptroffsets;

public:
  pointer_path(std::initializer_list<std::ptrdiff_t> l)
    : ptroffsets(l)
  {}

  const T& operator()() {
    return value;
  }

  void update() {
    auto remote_addr = reinterpret_cast<std::byte*>(0x140000000); // This is just temporary!
    auto buffer_ptr  = (std::byte*){ nullptr };
    auto len = sizeof remote_addr;

    // Chase pointers until we get to the memory address we want.
    auto it = begin(ptroffsets);
    while (it != end(ptroffsets) - 1) {
      remote_addr += *it;

      auto local  = iovec{ &buffer_ptr, len };
      auto remote = iovec{ remote_addr, len };

      auto read_len = process_vm_readv(pid, &local, 1, &remote, 1, 0);
//      if (read_len != len)
//        std::cerr << "Warning: could not read memory from process\n"
//                  << strerror(errno) << '\n';
      ++it;
      remote_addr = buffer_ptr;
    }

    len = sizeof(T);
    remote_addr += *it;

    auto local  = iovec{ &value,      len };
    auto remote = iovec{ remote_addr, len };

    auto read_len = process_vm_readv(pid, &local, 1, &remote, 1, 0);
//    if (read_len != len)
//      std::cerr << "Warning: could not read memory from process\n"
//                << strerror(errno) << '\n';
  }
};

#endif  // #ifndef SRT_POINTER_PATH_HPP
