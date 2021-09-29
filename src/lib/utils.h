// Author: Holodome
// Date: 25.09.2021 
// File: pkby/src/lib/utils.h
// Version: 0
// 
// Collection of functions that don't have special use cases, and jsut may come in handy in some cases
#pragma once
#include "general.h"
#define KB(_b) ((uptr)(_b) << 10)
#define MB(_b) (KB(_b) << 10)
#define IS_POW2(_n) ( ( (_n) & ((_n) - 1) ) == 0 )
u32 align_to_next_pow2(u32 v);
u64 align_forward(u64 value, u64 align);
u64 align_forward_pow2(u64 value, u64 align);
u64 align_backward_pow2(u64 value, u64 align);
