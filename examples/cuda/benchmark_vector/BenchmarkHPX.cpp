// Copyright (c)       2015 Patrick Diehl
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "utils.hpp"

#include <hpx/hpx_main.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/lcos/future.hpp>

#include "examples/opencl/benchmark_vector/timer.hpp"

#include "hpx_cuda.hpp"

#include "config.hpp"

using namespace hpx::cuda;

//###########################################################################
//Main
//###########################################################################

int main(int argc, char*argv[]) {

	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " #elements" << std::endl;
		exit(1);
	}

	size_t* count;
	cudaMallocHost((void**)&count,sizeof(size_t));
	count[0]= atoi(argv[1]);
	std::vector<double> timeKernel;

	std::cout << count[0] << " ";



	double timeData = 0;
	double timeCompile = 0;

	//Vector for all futures for the data management
	std::vector<hpx::lcos::future<void>> data_futures;

	// Get list of available Cuda Devices.
	std::vector<device> devices = get_all_devices(1, 0).get();

	// Check whether there are any devices
	if (devices.size() < 1) {
		hpx::cerr << "No CUDA devices found!" << hpx::endl;
		return hpx::finalize();
	}

	timer_start();

	//Pointer
	TYPE* out;
	TYPE* in1;
	TYPE* in2;

	timer_start();
	//Malloc Host
	cudaMallocHost((void**) &out, count[0] * sizeof(TYPE));
	cudaMallocHost((void**) &in1, count[0] * sizeof(TYPE));
	cudaMallocHost((void**) &in2, count[0] * sizeof(TYPE));

	//Initialize the data
	fillRandomVector(in1, count[0]);
	fillRandomVector(in2, count[0]);

	// Create a device component from the first device found
	device cudaDevice = devices[0];

	// Create a buffer
	buffer in1Buffer = cudaDevice.create_buffer_sync(count[0] * sizeof(TYPE));
	buffer in2Buffer = cudaDevice.create_buffer_sync(count[0] * sizeof(TYPE));
	buffer outBuffer = cudaDevice.create_buffer_sync(count[0] * sizeof(TYPE));
	buffer lengthbuffer = cudaDevice.create_buffer_sync(sizeof(size_t));

	// Copy input data to the buffer
	data_futures.push_back(in1Buffer.enqueue_write(0, count[0] * sizeof(TYPE),
					in1));
	data_futures.push_back(in2Buffer.enqueue_write(0, count[0] * sizeof(TYPE),
					in2));
	data_futures.push_back(outBuffer.enqueue_write(0, count[0] * sizeof(TYPE),
					in1));
	data_futures.push_back(lengthbuffer.enqueue_write(0,sizeof(size_t), count));

	hpx::wait_all(data_futures);

	timeData = timer_stop();

	timer_start();

	// Create the hello_world device program
	program prog = cudaDevice.create_program_with_file("kernel.cu").get();

	//Add compiler flags for compiling the kernel
	std::vector<std::string> flags;
	std::string mode = "--gpu-architecture=compute_";
	mode.append(
			std::to_string(cudaDevice.get_device_architecture_major().get()));

	mode.append(
			std::to_string(cudaDevice.get_device_architecture_minor().get()));

	flags.push_back(mode);
	flags.push_back("-use_fast_math");

	std::vector<std::string> kernels;
	kernels.push_back("logn");
	kernels.push_back("expn");
	kernels.push_back("dbl");
	kernels.push_back("add");
	kernels.push_back("mul");

	// Compile the program
	prog.build_sync(flags,kernels);

	timeCompile = timer_stop();

	//Generate the grid and block dim
	hpx::cuda::server::program::Dim3 grid;
	hpx::cuda::server::program::Dim3 block;

	//Set the values for the grid dimension
	grid.x = 1;
	grid.y = 1;
	grid.z = 1;

	//Set the values for the block dimension
	block.x = 32;
	block.y = 1;
	block.z = 1;

	//######################################################################
	//Launch kernels
	//######################################################################

	std::vector<hpx::cuda::buffer>args;
	args.push_back(lengthbuffer);
	args.push_back(in1Buffer);
	args.push_back(outBuffer);

	// 1. logn kernel
	timer_start();
	auto kernel_future = prog.run(args,"logn",grid,block);

	hpx::wait_all(kernel_future);

	TYPE* res = outBuffer.enqueue_read_sync<TYPE>(0,count[0] *sizeof(TYPE));
	timeKernel.push_back(timer_stop());
	for (size_t i = 0; i < count[0]; i++) {
		if (!(std::abs(std::log(in1[i]) - res[i]) < EPS))
		std::cout << "Error for logn at " << i << std::endl;
	}

	// 2. expn kernel
	timer_start();
	kernel_future = prog.run(args,"expn",grid,block);

	hpx::wait_all(kernel_future);

	res = outBuffer.enqueue_read_sync<TYPE>(0,count[0] *sizeof(TYPE));
	timeKernel.push_back(timer_stop());
	for (size_t i = 0; i < count[0]; i++) {
		if (!(std::abs(std::exp(in1[i]) - res[i]) < EPS))
		std::cout << "Error for expn at " << i << std::endl;
	}

	// 4. add kernel
	timer_start();
	kernel_future = prog.run(args,"dbl",grid,block);

	hpx::wait_all(kernel_future);

	res = outBuffer.enqueue_read_sync<TYPE>(0,count[0] *sizeof(TYPE));
	timeKernel.push_back(timer_stop());
	for (size_t i = 0; i < count[0]; i++) {
		if (!(std::abs(in1[i] * 2.0 - res[i]) < EPS))
		std::cout << "Error for dbl at " << i << std::endl;
	}

	args.push_back(in2Buffer);

	// 4. add kernel
	timer_start();
	kernel_future = prog.run(args,"add",grid,block);

	hpx::wait_all(kernel_future);

	res = outBuffer.enqueue_read_sync<TYPE>(0,count[0] *sizeof(TYPE));
	timeKernel.push_back(timer_stop());
	for (size_t i = 0; i < count[0]; i++) {
		if (!(std::abs(in1[i] + in2[i] - res[i]) < EPS))
		std::cout << "Error for add at " << i << std::endl;
	}

	// 5. mul kernel
	timer_start();
	kernel_future = prog.run(args,"mul",grid,block);

	hpx::wait_all(kernel_future);

	res = outBuffer.enqueue_read_sync<TYPE>(0,count[0] *sizeof(TYPE));
	timeKernel.push_back(timer_stop());
	for (size_t i = 0; i < count[0]; i++) {
		if (!(std::abs(in1[i] * in2[i] - res[i]) < EPS))
		std::cout << "Error for mul at " << i << std::endl;
	}

    //Cleanup
	timer_start();
	cudaFreeHost(in1);
	cudaFreeHost(in2);
	cudaFreeHost(out);
	cudaFreeHost(count);
	timeData += timer_stop();

	//Output of the results

	for ( auto time : timeKernel)
	std::cout << time << " ";

	std::cout << timeData << " " << timeCompile <<  std::endl;

	return EXIT_SUCCESS;
}
