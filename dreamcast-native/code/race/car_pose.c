/* Car pose publish (0x2BC08) and peer slots (0x2CC78). */
#include "race/car.h"
#include "sr_mem.h"

/* Node fields → its pose block, which the camera, the HUD and the other cars read. */
void sr_pose_publish(uint32_t node) {
    static const uint8_t copy[][2] = {
        /* pose offset, node offset */
        {0x00, 0x14}, {0x04, 0x18}, {0x08, 0x1c}, /* position */
        {0x0c, 0x2c}, {0x10, 0x30}, {0x14, 0x34}, /* velocity */
        {0x18, 0x38}, {0x1c, 0x3c}, {0x20, 0x40}, /* angles */
        {0x2c, 0x84}, {0x28, 0x80}, {0x24, 0x74},
        {0x30, 0x64}, {0x34, 0x68}, {0x38, 0x6c}, {0x3c, 0x70},
        {0x40, 0xb4}, {0x44, 0xb8}, {0x48, 0xbc}, {0x4c, 0xc0},
    };
    uint32_t pose, i;
    if(node == 0) return;
    pose = sr_ld32(node + CAR_POSE);
    if(pose == 0) return;
    for(i = 0; i < sizeof(copy) / sizeof(copy[0]); i++) sr_st32(pose + copy[i][0], sr_ld32(node + copy[i][1]));
    /* The node keeps its own low 6 flag bits and takes the pose's top 2. */
    sr_st8(node + CAR_FLAGS11, (sr_ld8(node + CAR_FLAGS11) & 0x3fu) | (sr_ld8(pose + 0x51) & 0xc0u));
    sr_st32(pose + 0x50, sr_ld32(node + CAR_FLAGS10));
    sr_st32(pose + 0x54, sr_ld32(node + CAR_SPAN_ACC));
    sr_st8(pose + 0x50, sr_ld8(pose + 0x50) | 0x80u);
}

/* The cars before and after this one in the running order → 0x20cac0 / 0x20cac4. */
void sr_car_peer(uint32_t node) {
    uint32_t pose, slot, count, prev, next;
    if(node == 0) return;
    pose = sr_ld32(node + CAR_POSE);
    slot = sr_ld32(0x214180u + (sr_ld8(pose + 0x51) & 0x3cu));
    count = sr_ld32(0x213978u);
    prev = slot - 1u;
    if((int32_t)prev < 0) prev = count - 1u;
    next = slot + 1u;
    if(next >= count) next = 0;
    sr_st32(0x20cac0u, sr_ld32(0x213b00u + (prev << 2)));
    sr_st32(0x20cac4u, sr_ld32(0x213b00u + (next << 2)));
}
