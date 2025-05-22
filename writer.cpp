#include <stdint.h>
#include "dataflow_api.h"

void kernel_main() {
    uint32_t dst_addr = get_arg_val<uint32_t>(0);
    uint32_t bank_id = get_arg_val<uint32_t>(1);

    uint64_t dst_noc_addr = get_noc_addr_from_bank_id<true>(bank_id, dst_addr);
    uint32_t read_ptr = get_read_ptr(16);
    uint32_t block_size = get_tile_size(16);

    // wait for the tile to be ready, then move it into DRAM
    cb_wait_front(16, 1);
    noc_async_write(read_ptr, dst_noc_addr, block_size);
    noc_async_write_barrier();
    cb_pop_front(16, 1);
}

