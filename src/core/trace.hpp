#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace input_trace {
enum class State { waiting, live, paused, unavailable };
struct Sample {
    double time{};
    float throttle{}, brake{};
    bool connected{};
};
struct Point { double x{}, y{}; };

// Call from one owner thread. Copy the whole object under an explicit handoff
// protocol before reading it on a different thread; this class is not synchronized.
template<std::size_t Capacity = 8192> class Trace {
    static_assert(Capacity > 1);
public:
    explicit Trace(double history = 5) { set_history(history); }
    void set_history(double seconds) {
        history_ = std::isfinite(seconds) ? (seconds < 2 ? 2 : seconds > 15 ? 15 : seconds) : 5;
        evict();
    }
    void clear() {
        head_ = count_ = 0;
        state_ = State::waiting;
        clock_ = 0;
        connected_ = false;
        has_clock_ = false;
    }
    // The adapter must establish freshness. Never call this just because a
    // render callback fired, or use input-value changes as a freshness signal.
    bool append(double time, float throttle, float brake) {
        if (state_ == State::paused) return false;
        if (!std::isfinite(time) || time < 0 || !valid(throttle) || !valid(brake)) {
            unavailable();
            return false;
        }
        if (has_clock_ && time < clock_) clear(); // rewind/restart
        if (has_clock_ && time == clock_) return false; // duplicate source timestamp
        // A long source interval is a break, even if its latest value is valid.
        const bool join = connected_ && has_clock_ && time - clock_ <= 0.5;
        clock_ = time;
        has_clock_ = true;
        if (count_ == Capacity) { head_ = (head_ + 1) % Capacity; --count_; }
        data_[(head_ + count_) % Capacity] = {time, throttle, brake, join};
        ++count_;
        connected_ = true;
        state_ = State::live;
        evict();
        return true;
    }
    void unavailable() { state_ = State::unavailable; connected_ = false; }
    // Pause must come from a verified source signal. Silence alone is not pause.
    void pause() { if (state_ == State::live) state_ = State::paused; }
    void resume() { if (state_ == State::paused) { state_ = State::waiting; connected_ = false; } }
    State state() const { return state_; }
    std::size_t size() const { return count_; }
    double time() const { return clock_; }
    double history() const { return history_; }
    const Sample& operator[](std::size_t index) const { return data_[(head_ + index) % Capacity]; }
    const Sample* latest() const { return state_ == State::live && count_ ? &(*this)[count_ - 1] : nullptr; }
    // Coordinates are relative to the graph. All retained samples lie within
    // its time range; the caller supplies a finite positive viewport.
    Point point(const Sample& sample, bool brake, double width, double height) const {
        return {width * (1 - (clock_ - sample.time) / history_),
                height * (1 - (brake ? sample.brake : sample.throttle))};
    }
private:
    static bool valid(float value) { return std::isfinite(value) && value >= 0 && value <= 1; }
    void evict() {
        while (count_ && (*this)[0].time < clock_ - history_) {
            head_ = (head_ + 1) % Capacity;
            --count_;
        }
        if (count_) data_[head_].connected = false;
    }
    std::array<Sample, Capacity> data_{};
    std::size_t head_{}, count_{};
    State state_{State::waiting};
    double history_{5}, clock_{};
    bool connected_{}, has_clock_{};
};
}
