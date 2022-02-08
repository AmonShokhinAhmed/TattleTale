#include "tale/kernels/resource.hpp"

#include <iostream>

namespace tale
{
    Resource::Resource(std::string name, std::vector<std::weak_ptr<Kernel>> reasons) : Kernel(name, reasons){};
    std::string Resource::ToString()
    {
        return "I am a Resource.\n";
    }
} // namespace tale