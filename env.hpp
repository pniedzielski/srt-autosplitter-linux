#ifndef LIVESPLIT_ONE_AUTOSPLITTER_ENV_HPP
#define LIVESPLIT_ONE_AUTOSPLITTER_ENV_HPP

#include <cstdint>        // std::uint{8,32,64}_t, std::int{32,64}_t,
                          // std::size_t

using Address = uint64_t;

using NonZeroAddress = uint64_t;  // nonzero, zero means failure

using ProcessId = uint64_t;       // nonzero, zero means failure

using TimerState = uint32_t;
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
  auto timer_get_state() -> TimerState;

  // Starts the timer.
  void timer_start();
  // Splits the current segment.
  void timer_split();
  // Resets the timer.
  void timer_reset();
  // Sets a custom key value pair.  This may be arbitrary information
  // that the auto splitter wants to provide for visualization.
  void timer_set_variable(const uint8_t* key_ptr,
                          size_t         key_len,
                          const uint8_t* value_ptr,
                          size_t         value_len);

  // Sets the game time.
  void timer_set_game_time(int64_t secs, int32_t nanos);
  // Pauses the game time.  This does not pause the timer, only the
  // automatic flow of time for the game time.
  void timer_pause_game_time();
  // Resumes the game time.  This does not resume the timer, only the
  // automatic flow of time for the game time.
  void timer_resume_game_time();

  // Attaches to a process based on its name.  (Returns 0 on failure.)
  auto process_attach(const uint8_t* name_ptr,
                      size_t         name_len)
    -> ProcessId;
  // Detaches from a process.
  void process_detach(ProcessId process);
  // Checks whether is a process is still open.  You should detach
  // from a process and stop using it if this returns `false`.
  auto process_is_open(ProcessId process) -> bool;
  // Reads memory from a process at the address given.  This will
  // write the memory to the buffer given.  Returns `false` if this
  // fails.
  auto process_read(ProcessId process,
                    Address   address,
                    uint8_t*  buf_ptr,
                    size_t    buf_len)
    -> bool;
  // Gets the address of a module in a process.  (Returns 0 on failure.)
  auto process_get_module_address(ProcessId      process,
                                  const uint8_t* name_ptr,
                                  size_t         name_len)
    -> NonZeroAddress;

  // Sets the tick rate of the runtime.  This influences the amount of
  // times the `update` function is called per second.
  void runtime_set_tick_rate(double ticks_per_second);
  // Prints a log message for debugging purposes.
  void runtime_print_message(const uint8_t* text_ptr,
                             size_t         text_len);
}

#endif  // #ifndef LIVESPLIT_ONE_AUTOSPLITTER_ENV_HPP
