/// @file tale.hpp Collects everything that is necessary so user only have to include this one file.
#ifndef TALE_TALE_H
#define TALE_TALE_H
#include "shared/kernels/kernel.hpp"
#include "shared/kernels/goal.hpp"
#include "shared/kernels/interactions/interaction.hpp"
#include "shared/kernels/interactions/interactionprototype.hpp"
#include "shared/kernels/interactions/interactionrequirement.hpp"
#include "shared/kernels/interactions/interactionrequirement.hpp"
#include "shared/kernels/interactions/interactioncontexttype.hpp"
#include "shared/kernels/resourcekernels/resource.hpp"
#include "shared/kernels/resourcekernels/emotion.hpp"
#include "shared/kernels/resourcekernels/relationship.hpp"
#include "shared/kernels/trait.hpp"
#include "tale/interactionstore.hpp"
#include "shared/random.hpp"
#include "shared/chronicle.hpp"
#include "shared/actor.hpp"
#include "tale/school.hpp"
#include "tale/course.hpp"
#include "tale/setting.hpp"
/**
 * @brief Responsible for the event generation of the TattleTale project.
 */
namespace tale
{
    // TODO: should return events and needs to be parameterizable
    void Run(const Setting &setting);
} // namespace tale
#endif // TALE_TALE_H