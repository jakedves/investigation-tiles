#include <stdint.h>
#include "dataflow_api.h"


void kernel_main() {
    uint32_t src0_addr = get_arg_val<uint32_t>(0);
    uint32_t src1_addr = get_arg_val<uint32_t>(1);
    uint32_t bank_id = get_arg_val<uint32_t>(2);

    uint64_t src0_noc_addr = get_noc_addr_from_bank_id<true>(bank_id, src0_addr);
    uint64_t src1_noc_addr = get_noc_addr_from_bank_id<true>(bank_id, src1_addr);

    uint32_t block_size_bytes0 = get_tile_size(0);
    uint32_t block_size_bytes1 = get_tile_size(1);

    uint32_t sram_write_address0 = get_write_ptr(0);
    uint32_t sram_write_address1 = get_write_ptr(1);

    // read blocks from src0, src1 into CB0, CB1, then push to compute core
    cb_reserve_back(0, 1);
    noc_async_read(src0_noc_addr, sram_write_address0, block_size_bytes0);
    noc_async_read_barrier();
    cb_push_back(0, 1);

    cb_reserve_back(1, 1);
    noc_async_read(src1_noc_addr, sram_write_address1, block_size_bytes1);
    noc_async_read_barrier();
    cb_push_back(1, 1);
}
