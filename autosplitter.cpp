#include <array>          // std::array<T,N>
#include <codecvt>        // std::codecvt_utf8_utf16
#include <chrono>         // namespace std::chrono_literals;
#include <cstddef>        // std::ptrdiff_t, std::byte
#include <iostream>       // std::cout, std::cerr
#include <locale>         // std::wstring_convert
#include <string>         // std::string
#include <string_view>    // std::u16string_view
#include <thread>         // std::this_thread
#include <unordered_set>  // std::unordered_set<K>
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

    auto pid    = pid_t{ 174359 };  // This is just temporary!

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

struct state {
  // Set to 0 when loading, set to 1 otherwise (foundable as a 4-byte
  // searching for 256)
  pointer_path<std::byte> is_not_loading{ 0x03415F30, 0xF8, 0x4A8, 0xE19 };

  // Set to 0 in game, 1 if in menu, 15 if in graphics submenu
  pointer_path<std::byte> in_menu{ 0x034160D0, 0x20, 0x218, 0x60 };

  // Set to 0 in title screen and main menu, set to 1 everywhere else
  pointer_path<std::byte> in_game{ 0x03415F30, 0xF0, 0x378, 0x564 };

  // Counts Ripto's 3rd phase health (init at 8 from the very
  // beginning of Ripto 1 fight, can be frozen at 0 to end the fight)
  pointer_path<std::byte> health_ripto3{ 0x03415F30, 0x110, 0x50, 0x140, 0x8, 0x1D0, 0x134 };

  // Counts Sorceress's 2nd phase health (init at 10 from the very
  // beginning of Sorc 1 fight, can be frozen at 0 to end the fight)
  //  pointer_path<std::byte> health_sorceress2{ 0x0 };    // TODO

  // ID of the map the player is being in
  pointer_path<std::array<char16_t, 256>> map{ 0x03415F30, 0x138, 0xB0, 0xB0, 0x598, 0x210, 0xB8, 0x148, 0x190, 0x0 };

  void update() {
    std::cerr << "is_not_loading\n";  is_not_loading.update();
    std::cerr << "in_menu\n";         in_menu.update();
    std::cerr << "in_game\n";         in_game.update();
    std::cerr << "health_ripto3\n";   health_ripto3.update();
    std::cerr << "map\n";             map.update();
  }
};

auto current = state{};
auto old     = state{};

auto already_triggered_splits  = std::unordered_set<std::string_view>{};
auto last_level_exit_timestamp = 0;

bool start() {
  if (current.in_game() == std::byte{1} && old.in_game() == std::byte{0}) {
    already_triggered_splits.clear();
    last_level_exit_timestamp = 0;
    return true;
  }
  return false;
}

int main(int argc, char** argv) {
  using namespace std::chrono_literals;

  std::cout << "Spyro Reignited Trilogy Autosplitter for Linux\n";

  while (true) {
    old = current;
    current.update();

    auto map_name_utf16 = std::u16string_view{ current.map().data(), current.map().size() };
    auto map_name = std::wstring_convert<
      std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(map_name_utf16.data());

    std::cout << "is_not_loading: " << static_cast<int>(current.is_not_loading()) << '\n';
    std::cout << "in_menu:        " << static_cast<int>(current.in_menu())        << '\n';
    std::cout << "in_game:        " << static_cast<int>(current.in_game())        << '\n';
    std::cout << "health_ripto3:  " << static_cast<int>(current.health_ripto3())  << '\n';
    std::cout << "map:            " << map_name                                   << '\n';

    if (start())  std::cout << "start\n";

    std::this_thread::sleep_for(33ms);
  }

  return 0;
}
