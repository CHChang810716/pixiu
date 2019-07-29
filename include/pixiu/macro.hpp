#pragma once
#include <type_traits>
#include <utility>

#ifndef PIXIU_VMEM_GET
# define PIXIU_VMEM_GET(T, var) \
private: T var##_;\
private: void var(T&& o) { var##_ = std::move(o); } \
public:  const T& var() const { return var##_; }
#endif

#ifndef PIXIU_PMEM_GET
# define PIXIU_PMEM_GET(PtrT, var) \
private: PtrT var##_;\
public:  const std::remove_pointer_t<PtrT>& var() const { return *var##_; }
#endif