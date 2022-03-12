#include "shared/tattletalecore.hpp"
#include "shared/kernels/kernel.hpp"

#include <iostream>

namespace tattletale
{
    Kernel::Kernel(
        std::string name, size_t id,
        size_t tick,
        std::weak_ptr<Actor> owner,
        std::vector<std::weak_ptr<Kernel>> reasons)
        : name_(name),
          id_(id),
          tick_(tick),
          owner_(owner),
          reasons_(reasons) {}
    void Kernel::AddConsequence(std::weak_ptr<Kernel> consequence)
    {
        consequences_.push_back(consequence);
    }

} // namespace tattletale