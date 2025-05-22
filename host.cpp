#include <vector>
#include <cstdint>

#include <tt-metalium/host_api.hpp>
#include <tt-metalium/device.hpp>

using namespace tt;
using namespace tt::tt_metal;

void print_grid(int rows, int elems_per_row, std::vector<uint32_t> v) {
    for (int row_index = 0; row_index < rows; row_index++) {
	for (int elem = 0; elem < elems_per_row; elem++) {
		std::cout << v[row_index * elems_per_row + elem] << " ";
	}
	std::cout << "\n";
    }
}

int main() {
    constexpr std::int32_t rows = 32;
    constexpr std::int32_t elems_per_row = 32;
    constexpr std::int32_t elems = rows * elems_per_row;

    // elems, default_value
    std::vector<std::uint32_t> ones(elems, 1);
    std::vector<std::uint32_t> twos(elems, 2);
    std::vector<std::uint32_t> outp(elems, 8); // random value to help spot bugs

    std::cout << "Initialisations:\n";
    print_grid(rows, elems_per_row, ones);
    std::cout << "\n ------ \n";
    print_grid(rows, elems_per_row, twos);
    std::cout << "\n ------ \n";
    print_grid(rows, elems_per_row, outp);
    std::cout << "\n ------ \n";
    std::cout << "Now running program...\n";

    // standard configuration
    IDevice* device = CreateDevice(0);
    CommandQueue& cq = device->command_queue();
    Program program = CreateProgram();
    constexpr CoreCoord core = {0, 0};

    // setup device DRAM buffers
    constexpr uint32_t dt_size_bytes = 4; // four bytes in a uint32_t
    constexpr uint32_t tile_size_bytes = rows * elems_per_row * dt_size_bytes;
    tt_metal::InterleavedBufferConfig dram_config {
	.device = device,
	.size = tile_size_bytes,
	.page_size = tile_size_bytes,
	.buffer_type = tt_metal::BufferType::DRAM
    };

    std::shared_ptr<tt::tt_metal::Buffer> src0_dram_buffer = CreateBuffer(dram_config);
    std::shared_ptr<tt::tt_metal::Buffer> src1_dram_buffer = CreateBuffer(dram_config);
    std::shared_ptr<tt::tt_metal::Buffer> dest_dram_buffer = CreateBuffer(dram_config);

    // issue copying from host DRAM to device DRAM (blocking)
    EnqueueWriteBuffer(cq, src0_dram_buffer, ones, true);
    EnqueueWriteBuffer(cq, src1_dram_buffer, twos, true);

    // no round-robin alloc as only a single page
    uint32_t bank_id = 0;

    // setup SRAM circular buffers
    constexpr uint32_t num_tiles = 1;
    CircularBufferConfig cb_src0_config = CircularBufferConfig(
	num_tiles * tile_size_bytes,
	{{0, tt::DataFormat::UInt32}}
    ).set_page_size(0, tile_size_bytes);
    CBHandle cb0 = tt_metal::CreateCircularBuffer(program, core, cb_src0_config);

    CircularBufferConfig cb_src1_config = CircularBufferConfig(
	num_tiles * tile_size_bytes,
	{{1, tt::DataFormat::UInt32}}
    ).set_page_size(1, tile_size_bytes);
    CBHandle cb1 = tt_metal::CreateCircularBuffer(program, core, cb_src1_config);

    CircularBufferConfig cb_dest_config = CircularBufferConfig(
	num_tiles * tile_size_bytes,
	{{16, tt::DataFormat::UInt32}}
    ).set_page_size(16, tile_size_bytes);
    CBHandle cbo = tt_metal::CreateCircularBuffer(program, core, cb_dest_config);


    // tell Tenstorrent runtime where to find kernel code
    KernelHandle reader = CreateKernel(
	program,
	"/home/jakedves/tenstorrent-projects/investigation-tiles/reader.cpp",
	core,
	DataMovementConfig { .processor = DataMovementProcessor::RISCV_1, .noc = NOC::RISCV_1_default });

    KernelHandle writer = CreateKernel(
	program,
	"/home/jakedves/tenstorrent-projects/investigation-tiles/writer.cpp",
	core,
	DataMovementConfig { .processor = DataMovementProcessor::RISCV_0, .noc = NOC::RISCV_0_default }
    );

    KernelHandle compute = CreateKernel(
	program,
	"/home/jakedves/tenstorrent-projects/investigation-tiles/compute.cpp",
	core,
	ComputeConfig {
	    .math_fidelity = MathFidelity::LoFi,
	    .fp32_dest_acc_en = false,
	    .math_approx_mode = false,
	    .compile_args = {},
	}
    );

    // configure arguments for kernels
    SetRuntimeArgs(
	program,
	reader,
	core,
	{
	    src0_dram_buffer->address(),
	    src1_dram_buffer->address(),
	    bank_id,
	}
    );

    SetRuntimeArgs(
	program,
	writer,
	core,
	{
	    dest_dram_buffer->address(),
	    bank_id,
	}
    );

    SetRuntimeArgs(program, compute, core, {});

    // launch the program, and await completion
    EnqueueProgram(cq, program, true);
    Finish(cq);

    // gather results from output CB
    EnqueueReadBuffer(cq, dest_dram_buffer, outp, true);

    print_grid(rows, elems_per_row, outp);

    CloseDevice(device);

    return 0;
}

