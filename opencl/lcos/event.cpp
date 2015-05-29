// Copyright (c)       2013 Martin Stumpf
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "event.hpp"

#include "../server/device.hpp"

void
hpx::opencl::lcos::detail::unregister_event( hpx::naming::id_type device_id,
                                             boost::uint64_t event_gid_msb,
                                             boost::uint64_t event_gid_lsb )
{

    // TODO change to hpx::naming::gid_type
    typedef hpx::opencl::server::device::release_event_action func;
    hpx::apply<func>( device_id, event_gid_msb, event_gid_lsb );

}

