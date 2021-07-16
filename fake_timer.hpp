#ifndef SRT_FAKE_TIMER_HPP
#define SRT_FAKE_TIMER_HPP

#include <chrono>

// This is an unfortunate hack, but one I can't see an easy way out
// of.  LiveSplit's server has gives you the ability to query the
// current game time, but LiveSplit One's server is too anemic to let
// us do that.  So we have to do it manually.

class fake_timer {
  private:
    std::chrono::steady_clock::time_point rta_start;
    std::chrono::steady_clock::time_point game_seg_start;
    std::chrono::steady_clock::duration   game_time;
    std::chrono::steady_clock::duration   last_level_exit_game_time;

  public:
    void start() {
      rta_start                 = std::chrono::steady_clock::now();
      game_seg_start            = std::chrono::steady_clock::now();
      last_level_exit_game_time = std::chrono::steady_clock::duration::zero();
      game_time                 = std::chrono::steady_clock::duration::zero();
    }

    void pause() {
      auto game_seg_end = std::chrono::steady_clock::now();
      game_time += game_seg_end - game_seg_start;
    }

    void resume() {
      game_seg_start = std::chrono::steady_clock::now();
    }

    // It would be nice if we could access the LiveSplit One timers,
    // but the simple websocket API doesn't let us see this.  So we
    // have to sorta implement it ourselves.
    std::chrono::steady_clock::duration rta() {
      auto rta_current = std::chrono::steady_clock::now();
      return rta_current - rta_start;
    }

    // Same with this.
    std::chrono::steady_clock::duration loadless() {
      // Set game_time so it's correct.
      auto game_seg_end = std::chrono::steady_clock::now();
      game_time += game_seg_end - game_seg_start;
      game_seg_start = game_seg_end;

      return game_time;
    }
};

#endif  // #ifndef SRT_FAKE_TIMER_HPP
