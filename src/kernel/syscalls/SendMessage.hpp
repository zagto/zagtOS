#pragma once

#include <common/common.hpp>
#include <processes/Process.hpp>

size_t SendMessage(const shared_ptr<Process> &process,
           uint64_t handle,
           uint64_t messageTypeAddress,
           uint64_t messageAddress,
           uint64_t messageSize,
           uint64_t numMessageHandles);
