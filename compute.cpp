#include <cstdint>
#include "compute_kernel_api/eltwise_binary.h"
#include "compute_kernel_api/tile_move_copy.h"

namespace NAMESPACE {
void MAIN {
    binary_op_init_common(0, 1, 16);
    add_tiles_init(0, 1);

    cb_wait_front(0, 1);
    cb_wait_front(1, 1);

    tile_regs_acquire();

    // args: cb0, cb1, tile_index0, tile_index1, dst_register_index
    add_tiles(0, 1, 0, 0, 0);

    tile_regs_commit();
    tile_regs_wait();
    pack_tile(0, 16);
    tile_regs_release();

    cb_pop_front(0, 1);
    cb_pop_front(1, 1);

    cb_push_back(16, 1);
}
}

