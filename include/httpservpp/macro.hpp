#pragma once
#include <type_traits>
#include <utility>

#ifndef VMEM_GET
# define VMEM_GET(T, var) \
private: T var##_;\
private: void var(T&& o) { var##_ = std::move(o); } \
public:  const T& var() const { return var##_; }
#endif

#ifndef PMEM_GET
# define PMEM_GET(PtrT, var) \
private: PtrT var##_;\
public:  const std::remove_pointer_t<PtrT>& var() const { return *var##_; }
#endif