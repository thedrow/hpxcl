// Copyright (c)    2013 Damond Howard
//                  2015 Patrick Diehl
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#ifndef HPX_CUDA_SERVER_PROGRAM_HPP_
#define HPX_CUDA_SERVER_PROGRAM_HPP_

#include <hpx/hpx.hpp>

#include <cuda.h>
#include <nvrtc.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "cuda/fwd_declarations.hpp"
#include "cuda/export_definitions.hpp"
#include "cuda/cuda_error_handling.hpp"

#include <map>

namespace hpx {
namespace cuda {
namespace server {

class HPX_CUDA_EXPORT program: public hpx::components::locking_hook<
		hpx::components::managed_component_base<program> > {
private:

	//boost::shared_ptr<device> parent_device;
	int parent_device_id;
	std::string kernel_source;
	std::string kernel_name;
	nvrtcProgram prog;
	std::map<std::string,CUfunction> kernels;
	std::vector<cudaStream_t> streams;
	CUmodule module;

public:

public:
	struct Dim3 {
		unsigned int x, y, z;
		template<typename Archive>
		void serialize(Archive &ar, unsigned int i) {
			ar & x;
			ar & y;
			ar & z;
		}
	};

	program();

	program(int parent_device_id);

	program(hpx::naming::id_type device_id, std::string code);

	program(hpx::naming::id_type device_id,
			hpx::serialization::serialize_buffer<char> binary);

	~program();

	void build(std::vector<std::string> compilerFlags, std::vector<std::string> modulenames, unsigned int debug = 0);

	void set_source(std::string source);

	void run(std::vector<hpx::naming::id_type> args, std::string modulename,
			Dim3 grid, Dim3 block, unsigned int stream = 0);

	unsigned int get_streams();

	unsigned int create_stream();

	HPX_DEFINE_COMPONENT_ACTION(program, build);
	HPX_DEFINE_COMPONENT_ACTION(program, set_source);
	HPX_DEFINE_COMPONENT_ACTION(program, run);
	HPX_DEFINE_COMPONENT_ACTION(program, get_streams);
	HPX_DEFINE_COMPONENT_ACTION(program, create_stream);

};
}
}
}

HPX_REGISTER_ACTION_DECLARATION(hpx::cuda::server::program::build_action,
		cuda_program_build_action);
HPX_ACTION_USES_MEDIUM_STACK(hpx::cuda::server::program::build_action);
HPX_REGISTER_ACTION_DECLARATION(hpx::cuda::server::program::run_action,
		cuda_program_run_action);
HPX_REGISTER_ACTION_DECLARATION(hpx::cuda::server::program::set_source_action,
		cuda_program_set_source_action);
HPX_REGISTER_ACTION_DECLARATION(hpx::cuda::server::program::get_streams_action,
		cuda_get_streams_action);
HPX_REGISTER_ACTION_DECLARATION(
		hpx::cuda::server::program::create_stream_action,
		cuda_create_stream_action);

#endif //PROGRAM_2_HPP
