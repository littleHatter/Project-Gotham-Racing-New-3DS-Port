#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

class PgrRaceBlur {
public:
    const uint16_t* Composite(const uint16_t* clean, unsigned width, bool racing) {
        if (!racing) {
            historyWidth = 0;
            return clean;
        }
        const std::size_t count = std::size_t(width) * 240;
        const bool valid = historyWidth == width;
        if (valid) {
            output.resize(count);
            for (std::size_t i = 0; i < count; ++i) {
                // Average each RGB565 channel without carrying into its neighbors.
                const uint16_t previous = history[i];
                const uint16_t current = clean[i];
                output[i] = (previous & current) + (((previous ^ current) & 0xf7de) >> 1);
            }
        }
        history.assign(clean, clean + count);
        historyWidth = width;
        return valid ? output.data() : clean;
    }

private:
    unsigned historyWidth = 0;
    std::vector<uint16_t> history;
    std::vector<uint16_t> output;
};
