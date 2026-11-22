#pragma once
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace input_trace::review {
// All lanes belong to the same lap. Switching stamps invalidates the whole
// block by changing its mask, without clearing or reading stale float values.
constexpr unsigned block_width = 32;
constexpr std::size_t block_count(std::size_t bins) {
    return (bins + block_width - 1) / block_width;
}
template<class Extrema>
struct BinBlock {
    struct Sample : Extrema { std::uint16_t link; };
    std::uint32_t stamp, occupied;
    Extrema values[block_width];
    std::uint16_t links[block_width];

    std::optional<Sample> read(unsigned lap_stamp, std::size_t index) const {
        const unsigned lane = index % block_width;
        if (stamp != lap_stamp || !(occupied & (1u << lane)) || (links[lane] & 1u))
            return std::nullopt;
        return Sample{values[lane], links[lane]};
    }
    int record(unsigned lap_stamp, std::size_t index, Extrema sample,
                unsigned predecessor_link, bool interrupted) {
        if (stamp != lap_stamp) {
            stamp = lap_stamp;
            occupied = 0;
        }
        const unsigned lane = index % block_width;
        const auto bit = 1u << lane;
        if (occupied & bit) {
            values[lane].include(sample);
            if (!interrupted || (links[lane] & 1u)) return 0;
            links[lane] |= 1u;
            return -1;
        }
        occupied |= bit;
        values[lane] = sample;
        links[lane] = static_cast<std::uint16_t>(predecessor_link);
        return 1;
    }
    int sample_count(unsigned lap_stamp) const {
        int count=0;
        for(auto lanes=stamp==lap_stamp?occupied:0u;lanes;lanes&=lanes-1)
            count+=!(links[std::countr_zero(lanes)] & 1u);
        return count;
    }
    // Drop the rewind boundary and suffix. Full blocks need just one mask write.
    static void truncate(BinBlock* blocks, std::size_t count, unsigned lap_stamp, std::size_t first) {
        for (std::size_t i = first / block_width; i < count; ++i) {
            if (blocks[i].stamp == lap_stamp) {
                const unsigned keep = i == first / block_width ? first % block_width : 0;
                blocks[i].occupied &= (1u << keep) - 1u;
            }
        }
    }
};
}
