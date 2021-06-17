#ifndef SRT_POINTER_PATH_HPP
#define SRT_POINTER_PATH_HPP

#include <cstddef>        // std::ptrdiff_t, std::byte
#include <iostream>       // std::cerr
#include <vector>         // std::vector<T>, begin, end

#include <errno.h>        // errno
#include <string.h>       // strerror
#include <sys/uio.h>      // process_vm_readv, struct iovec

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

    auto pid    = pid_t{ 211277 };  // This is just temporary!

    // Chase pointers until we get to the memory address we want.
    auto it = begin(ptroffsets);
    while (it != end(ptroffsets) - 1) {
      remote_addr += *it;

      auto local  = iovec{ &buffer_ptr, len };
      auto remote = iovec{ remote_addr, len };

      auto read_len = process_vm_readv(pid, &local, 1, &remote, 1, 0);
      if (read_len != len)
        std::cerr << "Warning: could not read memory from process\n"
                  << strerror(errno) << '\n';
      ++it;
      remote_addr = buffer_ptr;
    }

    len = sizeof(T);
    remote_addr += *it;

    auto local  = iovec{ &value,      len };
    auto remote = iovec{ remote_addr, len };

    auto read_len = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    if (read_len != len)
      std::cerr << "Warning: could not read memory from process\n"
                << strerror(errno) << '\n';
  }
};

#endif  // #ifndef SRT_POINTER_PATH_HPP
