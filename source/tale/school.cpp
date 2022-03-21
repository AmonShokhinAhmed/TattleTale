#include "shared/tattletalecore.hpp"
#include "tale/school.hpp"
#include "shared/kernels/interactions/interactionrequirement.hpp"
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <fstream>
#include <fmt/core.h>

namespace tattletale
{
    School::School(Chronicle &chronicle, Random &random, const Setting &setting) : setting_(setting), random_(random), chronicle_(chronicle), interaction_store_(random_)
    {
        size_t actor_count = setting_.actor_count;
        chronicle_.Reset();
        size_t tick = 0;

        std::vector<std::string> firstnames = GetRandomFirstnames(actor_count);
        std::vector<std::string> surnames = GetRandomSurames(actor_count);
        std::string actor_creation_description = "CREATED ALL ACTORS:";
        for (size_t i = 0; i < actor_count; ++i)
        {
            Actor *actor = chronicle_.CreateActor(*this, firstnames[i], surnames[i]);
            actor_creation_description += fmt::format("\n{}: {}", i, *actor);
            actors_.push_back(actor);
            freetime_group_.push_back(actor);
        }
        // needs to happen after every actor is created bc of relationships
        for (auto &actor : actors_)
        {
            actor->SetupRandomValues(tick);
        }
        TATTLETALE_VERBOSE_PRINT(actor_creation_description);
        std::string detailed_actor_descriptions = "DETAILED ACTOR DESCRIPTIONS:";
        for (size_t i = 0; i < actor_count; ++i)
        {
            detailed_actor_descriptions += fmt::format("\n{:o}", *actors_[i]);
        }
        TATTLETALE_VERBOSE_PRINT(detailed_actor_descriptions);

        size_t course_count = setting_.course_count();
        size_t slot_count_per_week = setting_.slot_count_per_week();
        std::vector<uint32_t> random_slot_order;
        for (size_t i = 0; i < course_count; ++i)
        {
            // TODO: create better names
            courses_.push_back(Course(random_, setting_, i, fmt::format("Course {}", i)));

            // Filling courses:
            TATTLETALE_VERBOSE_PRINT(fmt::format("CREATED COURSE {}", courses_[i].id_));
            random_slot_order.clear();
            // randomly order all slots of the course
            // TODO: optimize this by shuffling an already filled vector everytime instead of this
            while (random_slot_order.size() < slot_count_per_week)
            {
                uint32_t random_slot = random_.GetUInt(0, slot_count_per_week - 1);
                while (std::count(random_slot_order.begin(), random_slot_order.end(), random_slot))
                {
                    random_slot += 1;
                    random_slot %= slot_count_per_week;
                }
                random_slot_order.push_back(random_slot);
            }

            size_t slot_index = 0;
            // Go over every slot
            while (slot_index < random_slot_order.size() && setting_.same_course_per_week > 0)
            {
                std::vector<uint32_t> slots;
                // Because Actor have the same course a few times per week use the same group a few times
                for (size_t j = 0; j < setting_.same_course_per_week && slot_index < random_slot_order.size(); ++j)
                {
                    slots.push_back(random_slot_order[slot_index]);
                    ++slot_index;
                }
                std::list<Actor *> course_group = FindRandomCourseGroup(courses_[i].id_, slots);

                for (size_t j = 0; j < slots.size(); ++j)
                {
                    courses_[i].AddToSlot(course_group, slots[j]);
                }
            }
        }
        for (auto &actor : actors_)
        {
            if (actor->AllSlotsFilled())
            {
                continue;
            }
            for (size_t slot = 0; slot < slot_count_per_week; ++slot)
            {
                if (actor->SlotEmpty(slot))
                {
                    for (auto &course : courses_)
                    {
                        if (course.SpaceInSlot(slot))
                        {
                            course.AddToSlot(actor, slot);
                            break;
                        }
                    }
                }
            }
        }

        for (size_t course_index = 0; course_index < course_count; ++course_index)
        {
            for (size_t slot_index = 0; slot_index < slot_count_per_week; ++slot_index)
            {
                for (size_t other_course_index = course_index + 1; other_course_index < course_count; ++other_course_index)
                {
                    size_t actors_count_in_slot = courses_[course_index].GetActorCount(slot_index);
                    if (actors_count_in_slot > setting_.actors_per_course || actors_count_in_slot == 0)
                    {
                        break;
                    }
                    size_t other_actors_count_in_slot = courses_[other_course_index].GetActorCount(slot_index);
                    size_t summarized_actor_count = actors_count_in_slot + other_actors_count_in_slot;
                    if (summarized_actor_count < setting_.actors_per_course && other_actors_count_in_slot > 0)
                    {
                        std::list<Actor *> course_group = courses_[other_course_index].ClearSlot(slot_index);
                        courses_[course_index].AddToSlot(course_group, slot_index);
                    }
                }
            }
        }
    }

    void School::SimulateDays(size_t days)
    {
        for (size_t i = 0; i < days; ++i)
        {
            SimulateDay(current_day_, current_weekday_);
            ++current_day_;
            current_weekday_ = static_cast<Weekday>((static_cast<int>(current_weekday_) + 1) % 7);
        }
        TATTLETALE_DEBUG_PRINT(fmt::format("TALE CREATED {} KERNELS", chronicle_.GetKernelAmount()));
        TATTLETALE_VERBOSE_PRINT(fmt::format("RANDOM KERNEL CHAIN:\n{}", chronicle_GetRandomCausalityChainDescription(3)));
        TATTLETALE_VERBOSE_PRINT(fmt::format("AVERAGE INTERACTION CHANCE: {}", chronicle_.GetAverageInteractionChance()));
    }
    Actor *School::GetActor(size_t actor_id)
    {
        TATTLETALE_ERROR_PRINT(actor_id < actors_.size(), fmt::format("Actor with id {} does not exist", actor_id));
        return actors_[actor_id];
    }
    Course &School::GetCourse(size_t course_id)
    {
        TATTLETALE_ERROR_PRINT(course_id < courses_.size(), fmt::format("Course with id {} does not exist", course_id));
        return courses_[course_id];
    }

    size_t School::GetCurrentDay() const
    {
        return current_day_;
    }
    Weekday School::GetCurrentWeekday() const
    {
        return current_weekday_;
    }
    const Setting &School::GetSetting() const
    {
        return setting_;
    }
    InteractionStore &School::GetInteractionStore()
    {
        return interaction_store_;
    }
    Random &School::GetRandom()
    {
        return random_;
    }
    Chronicle &School::GetChronicle()
    {
        return chronicle_;
    }
    void School::SimulateDay(size_t day, Weekday weekday)
    {
        TATTLETALE_VERBOSE_PRINT(fmt::format("{}. {}", day, weekday_string[static_cast<int>(weekday)]));
        if (IsWorkday(weekday))
        {
            for (size_t i = 0; i < setting_.courses_per_day; ++i)
            {
                size_t slot = WeekdayAndDailyTickToSlot(weekday, i);
                for (auto &course : courses_)
                {
                    std::list<Actor *> course_group = course.GetCourseGroupForSlot(slot);
                    for (auto &actor : course_group)
                    {
                        LetActorInteract(actor, course_group, ContextType::kCourse, fmt::format("During Slot {} in {}", i, course.name_));
                    }
                }
                ++current_tick_;
            }
            FreeTimeTick();
            ++current_tick_;
        }
        else
        {
            for (size_t i = 0; i < setting_.courses_per_day + 1; ++i)
            {
                FreeTimeTick();
                ++current_tick_;
            }
        }
    }

    void School::FreeTimeTick()
    {
        for (auto &actor : actors_)
        {
            LetActorInteract(actor, actor->GetFreetimeActorGroup(), ContextType::kFreetime, "Freetime");
        }
    }

    void School::LetActorInteract(Actor *&actor, const std::list<Actor *> &group, ContextType type, std::string context_description)
    {
        std::vector<Kernel *> reasons;
        std::vector<Actor *> participants;
        float chance;
        int interaction_index = actor->ChooseInteraction(group, type, reasons, participants, chance);
        std::string interaction_description = "did nothing.";
        if (interaction_index != -1)
        {
            Interaction *interaction = interaction_store_.CreateInteraction(chronicle_, interaction_index, chance, current_tick_, reasons, participants);
            interaction->Apply();
            interaction_description = fmt::format("{}", *interaction);
        }
        TATTLETALE_VERBOSE_PRINT(fmt::format("During {} {}", context_description, interaction_description));
    }
    bool School::IsWorkday(Weekday weekday) const
    {
        if (weekday == Weekday::Saturday || weekday == Weekday::Sunday)
        {
            return false;
        }
        return true;
    }

    bool School::ActorIsInCourseGroup(const Actor *actor, const std::list<Actor *> &course_group) const
    {
        for (auto &actor_in_course_group : course_group)
        {
            if (actor_in_course_group == actor)
            {
                return true;
            }
        }
        return false;
    }

    size_t School::WeekdayAndDailyTickToSlot(Weekday weekday, size_t daily_tick) const
    {
        return static_cast<size_t>(weekday) * setting_.courses_per_day + daily_tick;
    }

    std::list<Actor *> School::FindRandomCourseGroup(size_t course_id, const std::vector<uint32_t> &slots)
    {
        std::list<Actor *> course_group;
        if (actors_.size() > 0)
        {
            for (size_t i = 0; i < setting_.actors_per_course; ++i)
            {
                size_t random_actor_index = random_.GetUInt(0, actors_.size() - 1);

                size_t current_index_search_try = 0;
                while (
                    (actors_[random_actor_index]->AllSlotsFilled() || actors_[random_actor_index]->IsEnrolledInCourse(course_id) || ActorIsInCourseGroup(actors_[random_actor_index], course_group) || !actors_[random_actor_index]->SlotsEmpty(slots)) && current_index_search_try < setting_.actor_count)
                {
                    random_actor_index += -1;
                    random_actor_index %= actors_.size();
                    ++current_index_search_try;
                }
                if (current_index_search_try != actors_.size())
                {
                    course_group.push_back(actors_[random_actor_index]);
                }
            }
        }
        return course_group;
    }
    std::vector<std::string> School::GetRandomFirstnames(size_t count)
    {
        return GetRandomNames(count, "resources/firstname.txt");
    }
    std::vector<std::string> School::GetRandomSurames(size_t count)
    {
        return GetRandomNames(count, "resources/surname.txt");
    }

    std::vector<std::string> School::GetRandomNames(size_t count, std::string path)
    {
        std::vector<std::string> all_names;
        std::string firstname;
        std::ifstream firstname_file(path);
        TATTLETALE_ERROR_PRINT(firstname_file.is_open(), fmt::format("Could not open {}", path));
        while (getline(firstname_file, firstname))
        {
            all_names.push_back(firstname);
        }
        std::vector<std::string> random_names;
        for (size_t i = 0; i < count; ++i)
        {
            random_names.push_back(all_names[random_.GetUInt(0, all_names.size() - 1)]);
        }
        return random_names;
    }

} // namespace tattletale