#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>

namespace Orientation {
constexpr unsigned PacketSize = 10;
enum Status : uint8_t { Unsupported = 0, Unavailable = 1, Ready = 2 };
struct Snapshot {
    Status status = Unavailable;
    float pitch = 0;
    float roll = 0;
};

inline void encode(const Snapshot& sample, uint8_t (&bytes)[PacketSize]) {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559, "Requires IEEE float32");
    bytes[0] = 2;
    bytes[1] = sample.status;
    const float angles[] = {sample.pitch, sample.roll};
    for (unsigned axis = 0; axis < 2; ++axis) {
        uint32_t bits = 0;
        if (sample.status == Ready) memcpy(&bits, &angles[axis], sizeof(bits));
        for (unsigned byte = 0; byte < 4; ++byte) bytes[2 + axis * 4 + byte] = bits >> (byte * 8);
    }
}

#ifndef NDEBUG
inline void checkPacket() {
    uint8_t bytes[PacketSize];
    encode({Ready, -45.0f, 90.0f}, bytes);
    const uint8_t expected[] = {2, 2, 0, 0, 0x34, 0xc2, 0, 0, 0xb4, 0x42};
    static_assert(sizeof(expected) == PacketSize, "Packet length changed");
    assert(memcmp(bytes, expected, PacketSize) == 0);
    const Status statuses[] = {Unsupported, Unavailable};
    for (Status status : statuses) {
        encode({status, -45.0f, 90.0f}, bytes);
        assert(bytes[0] == 2 && bytes[1] == status);
        for (unsigned i = 2; i < PacketSize; ++i) assert(bytes[i] == 0);
    }
}
#endif
}
