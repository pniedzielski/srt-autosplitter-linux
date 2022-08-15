#ifndef LIVESPLIT_ONE_AUTOSPLITTER_ENV_HPP
#define LIVESPLIT_ONE_AUTOSPLITTER_ENV_HPP

#define ENV __attribute__((import_module("env")))

#include <cstddef>        // std::byte
#include <cstdint>        // std::uint{32,64}_t, std::int{32,64}_t,
                          // std::size_t

using Address = std::uint64_t;

using NonZeroAddress = std::uint64_t;  // nonzero, zero means failure

using ProcessId = std::uint64_t;       // nonzero, zero means failure

using TimerState = std::uint32_t;
// The timer is not running.
const auto NOT_RUNNING = TimerState{ 0 };
// The timer is running.
const auto RUNNING     = TimerState{ 1 };
// The timer started but got paused.  This is separate from the game
// time being paused.  Game time may even always be paused.
const auto PAUSED      = TimerState{ 2 };
// The timer has ended, but didn't get reset yet.
const auto ENDED       = TimerState{ 3 };

extern "C" {
  // Gets the state that the timer currently is in.
  auto ENV timer_get_state() -> TimerState;

  // Starts the timer.
  void ENV timer_start();
  // Splits the current segment.
  void ENV timer_split();
  // Resets the timer.
  void ENV timer_reset();
  // Sets a custom key value pair.  This may be arbitrary information
  // that the auto splitter wants to provide for visualization.
  void ENV timer_set_variable(const std::byte* key_ptr,
                              std::size_t      key_len,
                              const std::byte* value_ptr,
                              std::size_t      value_len);

  // Sets the game time.
  void ENV timer_set_game_time(std::int64_t secs, std::int32_t nanos);
  // Pauses the game time.  This does not pause the timer, only the
  // automatic flow of time for the game time.
  void ENV timer_pause_game_time();
  // Resumes the game time.  This does not resume the timer, only the
  // automatic flow of time for the game time.
  void ENV timer_resume_game_time();

  // Attaches to a process based on its name.  (Returns 0 on failure.)
  auto ENV process_attach(const std::byte* name_ptr,
                          std::size_t      name_len)
    -> ProcessId;
  // Detaches from a process.
  void ENV process_detach(ProcessId process);
  // Checks whether is a process is still open.  You should detach
  // from a process and stop using it if this returns `false`.
  auto ENV process_is_open(ProcessId process) -> bool;
  // Reads memory from a process at the address given.  This will
  // write the memory to the buffer given.  Returns `false` if this
  // fails.
  auto ENV process_read(ProcessId   process,
                        Address     address,
                        std::byte*  buf_ptr,
                        std::size_t buf_len)
    -> bool;
  // Gets the address of a module in a process.  (Returns 0 on failure.)
  auto ENV process_get_module_address(ProcessId        process,
                                      const std::byte* name_ptr,
                                      std::size_t      name_len)
    -> NonZeroAddress;

  // Sets the tick rate of the runtime.  This influences the amount of
  // times the `update` function is called per second.
  void ENV runtime_set_tick_rate(double ticks_per_second);
  // Prints a log message for debugging purposes.
  void ENV runtime_print_message(const std::byte* text_ptr,
                                 std::size_t      text_len);
}

#undef ENV

#endif  // #ifndef LIVESPLIT_ONE_AUTOSPLITTER_ENV_HPP
