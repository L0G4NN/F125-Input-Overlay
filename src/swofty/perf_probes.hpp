#pragma once
#include <cstdint>

namespace input_trace {

// Optional exports from swofty_core.dll. A zero ID is reserved for game code.
struct PerfProbes {
    using Register = std::uint32_t (*)(const char*, const char*);
    using Enter = void (*)(std::uint32_t);
    using Leave = void (*)();

    Enter enter{};
    Leave leave{};
    std::uint32_t udp{}, live_hud{}, review{}, rebuild{};

    static PerfProbes create(Register probe, Enter enter, Leave leave) {
        if (!probe || !enter || !leave) return {};
        constexpr auto owner = "l0g4nn.input-overlay";
        return {enter, leave, probe(owner, "UDP receive and record"),
                probe(owner, "Live HUD draw"), probe(owner, "F6 panel draw"),
                probe(owner, "F6 plot rebuild")};
    }

    const PerfProbes& for_frame(bool enabled) const {
        static constexpr PerfProbes disabled{};
        return enabled ? *this : disabled;
    }

    class Scope {
    public:
        Scope(const PerfProbes& probes, std::uint32_t id) : leave_(id ? probes.leave : nullptr) {
            if (leave_) probes.enter(id);
        }
        ~Scope() { if (leave_) leave_(); }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    private:
        Leave leave_;
    };
};

}
