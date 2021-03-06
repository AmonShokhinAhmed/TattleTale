#include "shared/actor.hpp"
#include "tale/school.hpp"
#include "tale/school.hpp"
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <math.h>
#include <fmt/core.h>

namespace tattletale
{
    std::set<size_t>* Actor::named_actors_ =  nullptr;
    
    Actor::Actor(School &school, size_t id, std::string first_name, std::string last_name)
        : random_(school.GetRandom()),
          setting_(school.GetSetting()),
          chronicle_(school.GetChronicle()),
          school_(school),
          interaction_store_(school.GetInteractionStore()),
          id_(id),
          first_name_(first_name),
          last_name_(last_name),
          name_(first_name + " " + last_name)
    {
        enrolled_courses_id_ = std::vector<int>(setting_.slot_count_per_week(), -1);
    }

    void Actor::SetupRandomValues(size_t tick)
    {
        InitializeRandomWealth(tick);
        InitializeRandomEmotions(tick);
        InitializeRandomRelationships(tick);
        InitializeRandomGoal(tick);
    }

    bool Actor::IsEnrolledInCourse(size_t course_id) const
    {
        return std::count(enrolled_courses_id_.begin(), enrolled_courses_id_.end(), course_id);
    }
    void Actor::EnrollInCourse(size_t course_id, uint32_t slot)
    {
        TATTLETALE_ERROR_PRINT(enrolled_courses_id_[slot] == -1, fmt::format("{} already enrolled in a course during slot {}", name_, slot));
        TATTLETALE_ERROR_PRINT(filled_slots_count_ < setting_.slot_count_per_week(), fmt::format("{} already filled their whole schedule", name_))
        ++filled_slots_count_;
        enrolled_courses_id_[slot] = course_id;
    }
    void Actor::EjectFromCourse(size_t course_id, uint32_t slot)
    {
        TATTLETALE_ERROR_PRINT(enrolled_courses_id_[slot] == course_id, fmt::format("Actor is not enrolled in course with id {} during slot {}", course_id, slot));
        --filled_slots_count_;
        enrolled_courses_id_[slot] = -1;
    }
    size_t Actor::GetFilledSlotsCount() const
    {
        return filled_slots_count_;
    }
    bool Actor::AllSlotsFilled() const
    {
        TATTLETALE_ERROR_PRINT(filled_slots_count_ <= setting_.slot_count_per_week(), fmt::format("{} is enrolled in too may courses", name_));
        return filled_slots_count_ == setting_.slot_count_per_week();
    }
    bool Actor::SlotsEmpty(const std::vector<uint32_t> &slots) const
    {
        for (auto &slot : slots)
        {
            if (enrolled_courses_id_[slot] != -1)
            {
                return false;
            }
        }
        return true;
    }
    bool Actor::SlotEmpty(size_t slot) const
    {
        return (enrolled_courses_id_[slot] == -1);
    }

    int Actor::ChooseInteraction(const std::list<Actor *> &actor_group, ContextType context, std::vector<Kernel *> &out_reasons, std::vector<Actor *> &out_participants, float &out_chance)
    {
        // Finding possible Interactions
        const std::vector<std::shared_ptr<InteractionRequirement>> &requirements = interaction_store_.GetRequirementCatalogue();
        std::vector<size_t> possible_interaction_indices;
        for (size_t i = 0; i < requirements.size(); ++i)
        {
            if (CheckRequirements(*requirements[i], actor_group, context))
            {
                possible_interaction_indices.push_back(i);
            }
        }
        if (possible_interaction_indices.size() == 0)
        {
            return -1;
        }

        // Calculating chances for each Interaction
        const std::vector<std::shared_ptr<InteractionTendency>> &tendencies = interaction_store_.GetTendencyCatalogue();
        std::vector<float> chances;
        std::vector<Kernel *> tendency_reasons;
        std::vector<Kernel *> goal_reasons;
        chances.reserve(possible_interaction_indices.size());
        tendency_reasons.reserve(possible_interaction_indices.size());
        goal_reasons.reserve(possible_interaction_indices.size());
        uint32_t zero_count = 0;
        for (auto &i : possible_interaction_indices)
        {
            std::shared_ptr<InteractionTendency> tendency = tendencies[i];
            Kernel *tendency_reason = nullptr;
            float chance = CalculateInteractionChance(*tendency, context, tendency_reason);
            Kernel *goal_reason = nullptr;
            float modified_chance = ApplyGoalChanceModification(chance, i, goal_reason);
            if (modified_chance == 0.0f)
            {
                ++zero_count;
            }
            chances.push_back(modified_chance);
            tendency_reasons.push_back(tendency_reason);
            goal_reasons.push_back(goal_reason);
        }

        // Picking Interaction
        size_t index = random_.PickIndex(chances, (zero_count == chances.size()));
        if (tendency_reasons[index])
        {
            out_reasons.push_back(tendency_reasons[index]);
        }
        if (goal_reasons[index])
        {
            out_reasons.push_back(goal_reasons[index]);
        }
        out_chance = chances[index];
        size_t interaction_index = possible_interaction_indices[index];

        // Choosing Participants:
        out_participants.push_back(this);
        std::shared_ptr<InteractionRequirement> requirement = requirements[interaction_index];
        std::shared_ptr<InteractionTendency> tendency = tendencies[interaction_index];
        for (size_t i = 1; i < requirement->participant_count; ++i)
        {
            uint32_t participant_zero_count = 0;
            std::vector<float> participant_chances;
            std::vector<Kernel *> participant_reasons;

            for (auto &participant : actor_group)
            {
                Kernel *reason = nullptr;
                float chance = CalculateParticipantChance(participant, i, requirement, tendency, reason);
                for (auto &already_chosen_participant : out_participants)
                {
                    if (participant->id_ == already_chosen_participant->id_)
                    {
                        chance = 0;
                    }
                }
                if (chance == 0.0f)
                {
                    participant_zero_count++;
                }
                participant_chances.push_back(chance);
                participant_reasons.push_back(reason);
            }
            if ((participant_zero_count == participant_chances.size()))
            {
                return -1;
            }
            size_t participant_index = random_.PickIndex(participant_chances);
            auto chosen_actor = std::next(actor_group.begin(), participant_index);
            out_participants.push_back((*chosen_actor));
            if (participant_reasons[participant_index])
            {
                out_reasons.push_back(participant_reasons[participant_index]);
            }
        }

        return interaction_index;
    }
    bool Actor::CheckRequirements(const InteractionRequirement &requirement, const std::list<Actor *> &actor_group, ContextType context) const
    {
        if (requirement.context != ContextType::kLast && requirement.context != context)
        {
            return false;
        }
        if (requirement.participant_count > actor_group.size())
        {
            return false;
        }
        if (requirement.goal_type != GoalType::kLast && requirement.goal_type != goal_->type_)
        {
            return false;
        }
        if (requirement.day > school_.GetCurrentDay())
        {
            return false;
        }
        for (int type_index = 0; type_index < static_cast<int>(EmotionType::kLast); ++type_index)
        {
            float value = requirement.emotions[0][type_index];
            if (value < 0)
            {
                if (emotions_[type_index]->GetValue() > value)
                {
                    return false;
                }
            }
            else if (value > 0)
            {
                if (emotions_[type_index]->GetValue() < value)
                {
                    return false;
                }
            }
        }
        bool match_found = true;
        for (size_t participant_id = 1; participant_id < requirement.participant_count; ++participant_id)
        {
            if (requirement.HasRelationshipRequirement(participant_id))
            {
                for (auto &[actor_id, relationship] : relationships_)
                {
                    match_found = true;
                    for (int type_index = 0; type_index < relationship.size(); ++type_index)
                    {
                        float value = requirement.relationship[participant_id - 1][type_index];
                        if (value < 0)
                        {
                            if (relationship.at(type_index)->GetValue() > value)
                            {
                                match_found = false;
                            }
                        }
                        else if (value > 0)
                        {
                            if (relationship.at(type_index)->GetValue() < value)
                            {
                                match_found = false;
                            }
                        }
                    }
                    if (match_found)
                    {
                        break;
                    }
                }
                if (!match_found)
                {
                    return false;
                }
            }
        }
        return true;
    }

    float Actor::CalculateParticipantChance(const Actor *participant, size_t participant_id, const std::shared_ptr<InteractionRequirement> &requirement, const std::shared_ptr<InteractionTendency> &tendency, Kernel *&out_reason)
    {
        size_t id = participant->id_;
        if (relationships_.count(id))
        {
            std::vector<Relationship *> relationship_vector = relationships_[id];

            float chance = 0.0f;
            float highest_chance_increase = 0.0f;
            bool requirement_failed = false;

            for (int type_index = 0; type_index < static_cast<int>(RelationshipType::kLast); ++type_index)
            {
                Relationship *relationship = relationship_vector[type_index];
                float current_chance_increase = relationship->GetValue() * tendency->relationships[participant_id - 1][type_index];
                chance += current_chance_increase;

                if (current_chance_increase > highest_chance_increase)
                {
                    highest_chance_increase = current_chance_increase;
                    out_reason = relationship;
                }
                float relationship_requirement_value = requirement->relationship[participant_id - 1][type_index];
                if (relationship_requirement_value < 0)
                {
                    if (relationship->GetValue() > relationship_requirement_value)
                    {
                        requirement_failed = true;
                    }
                }
                else if (relationship_requirement_value > 0)
                {
                    if (relationship->GetValue() < relationship_requirement_value)
                    {
                        requirement_failed = true;
                    }
                }
            }

            for (int type_index = 0; type_index < static_cast<int>(EmotionType::kLast); ++type_index)
            {
                float emotional_value = requirement->emotions[participant_id][type_index];
                if (emotional_value < 0)
                {
                    if (participant->emotions_[type_index]->GetValue() > emotional_value)
                    {
                        requirement_failed = true;
                    }
                }
                else if (emotional_value > 0)
                {
                    if (participant->emotions_[type_index]->GetValue() < emotional_value)
                    {
                        requirement_failed = true;
                    }
                }
            }
            if (requirement_failed)
            {
                return 0.0f;
            }
            float tendency_parts = static_cast<float>(RelationshipType::kLast);
            chance += tendency_parts;
            chance /= (tendency_parts * 2);
            return chance;
        }
        else
        {
            if (requirement->HasRelationshipRequirement(participant_id))
            {
                return 0.0f;
            }
            else
            {
                return 0.5f;
            }
        }
        return 0.0f;
    }
    float Actor::CalculateInteractionChance(const InteractionTendency &tendency, const ContextType &context, Kernel *&out_reason)
    {
        // TODO: reasons will never include context
        float chance = 0.0f;
        float highest_chance_influence = 0.0f;
        float current_chance_increase = 0.0f;
        uint16_t chance_parts = 0;

        for (int type_index = 0; type_index < static_cast<int>(ContextType::kLast); ++type_index)
        {
            ContextType type = static_cast<ContextType>(type_index);
            float value = tendency.contexts[type_index];
            current_chance_increase = (value * (context == type ? 1.0f : -1.0f));
            chance += current_chance_increase;
            ++chance_parts;
        }

        current_chance_increase = tendency.wealth * wealth_->GetValue();
        chance += current_chance_increase;
        ++chance_parts;
        if (current_chance_increase > highest_chance_influence)
        {
            highest_chance_influence = current_chance_increase;
            out_reason = wealth_;
        }

        for (int type_index = 0; type_index < static_cast<int>(EmotionType::kLast); ++type_index)
        {
            float value = tendency.emotions[type_index];
            current_chance_increase = (value * emotions_[type_index]->GetValue());
            chance += current_chance_increase;
            ++chance_parts;
            if (current_chance_increase > highest_chance_influence)
            {
                highest_chance_influence = current_chance_increase;
                out_reason = emotions_[type_index];
            }
        }

        chance += static_cast<float>(chance_parts);
        chance /= static_cast<float>(chance_parts * 2);
        return chance;
    }
    float Actor::ApplyGoalChanceModification(float original_chance, size_t interaction_index, Kernel *&out_reason)
    {
        float relevant_effect = 0;
        const auto &effects = interaction_store_.GetRelationshipEffects(interaction_index);
        switch (goal_->type_)
        {
        case GoalType::kWealth:
            relevant_effect = interaction_store_.GetWealthEffects(interaction_index)[0];
            break;
        case GoalType::kAcceptance:
            // skip first as we want the values of other people to this actor
            for (size_t i = 1; i < effects.size(); ++i)
            {
                if (effects[i].count(0))
                {
                    relevant_effect += effects[i].at(0).at(static_cast<int>(RelationshipType::kFriendship));
                }
            }
            break;
        case GoalType::kRelationship:
            // skip first as we want the values of other people to this actor
            for (size_t i = 1; i < effects.size(); ++i)
            {
                // do both sides of the relationship change
                if (effects[i].count(0) && effects[0].count(i))
                {
                    relevant_effect += effects[i].at(0).at(static_cast<int>(RelationshipType::kLove));
                    relevant_effect += effects[0].at(1).at(static_cast<int>(RelationshipType::kLove));
                }
            }
            break;
        case GoalType::kHedonism:
            relevant_effect = interaction_store_.GetEmotionEffects(interaction_index)[0][static_cast<int>(EmotionType::kSatisfied)];
            break;
        case GoalType::kPower:
            // skip first as we want the values of other people to this actor
            for (const auto &[other_actor, effect] : effects[0])
            {
                // using minus here because the actor wants a negative value
                relevant_effect -= effect.at(static_cast<int>(RelationshipType::kProtective));
            }
            if(effects[0].size()>0){
                relevant_effect /= effects[0].size();
            }
            break;
        case GoalType::kLast:
            TATTLETALE_ERROR_PRINT(true, "Trying to apply invalid goal type");
            break;
        }
        relevant_effect = std::clamp(relevant_effect, -1.0f, 1.0f);
        if (relevant_effect > 0)
        {
            out_reason = goal_;
        }
        else
        {
            out_reason = nullptr;
        }
        // seet https://www.desmos.com/calculator/ojqnu2ni4c
        return pow(original_chance, (1.0f - relevant_effect));
    }

    void Actor::ApplyWealthChange(const std::vector<Kernel *> &reasons, size_t tick, float value)
    {
        if (value == 0)
        {
            return;
        }
        float previous_value = wealth_->GetValue();
        float new_value = std::clamp(previous_value + value, -1.0f, 1.0f);
        std::vector<Kernel *> all_reasons(reasons);
        all_reasons.push_back(wealth_);
        wealth_ = chronicle_.CreateResource("wealth", "wealthy", "poor", tick, this, all_reasons, new_value);
    }
    void Actor::ApplyEmotionChange(const std::vector<Kernel *> &reasons, size_t tick, int type_index, float value)
    {
        if (value == 0)
        {
            return;
        }
        float previous_value = emotions_[type_index]->GetValue();
        float new_value = std::clamp(previous_value + value, -1.0f, 1.0f);
        std::vector<Kernel *> all_reasons(reasons);
        all_reasons.push_back(emotions_[type_index]);
        EmotionType type = static_cast<EmotionType>(type_index);
        emotions_[type_index] = chronicle_.CreateEmotion(type, tick, this, all_reasons, new_value);
    }
    void Actor::ApplyRelationshipChange(const std::vector<Kernel *> &reasons, size_t tick, size_t actor_id, std::vector<float> change)
    {
        bool all_zero = true;
        for (auto &value : change)
        {
            if (value != 0)
            {
                all_zero = false;
            }
        }
        if (all_zero)
        {
            return;
        }
        bool already_known = relationships_.count(actor_id);
        auto other_actor = school_.GetActor(actor_id);
        std::vector<Kernel *> all_reasons;
        std::vector<Relationship *> relationship(static_cast<int>(RelationshipType::kLast));
        for (int type_index = 0; type_index < change.size(); ++type_index)
        {
            float value = change[type_index];
            RelationshipType type = static_cast<RelationshipType>(type_index);
            all_reasons.clear();
            all_reasons.insert(all_reasons.end(), reasons.begin(), reasons.end());
            float previous_value = 0;
            if (already_known)
            {
                if (value == 0)
                {
                    relationship[type_index] = relationships_.at(actor_id).at(type_index);
                    continue;
                }
                previous_value = relationships_.at(actor_id).at(type_index)->GetValue();
                all_reasons.push_back(relationships_.at(actor_id).at(type_index));
            }
            float new_value = std::clamp(previous_value + value, -1.0f, 1.0f);
            relationship[type_index] = chronicle_.CreateRelationship(type, tick, this, other_actor, all_reasons, new_value);
        }
        UpdateRelationship(other_actor, relationship, already_known);
    }

    std::string Actor::GetDetailedDescriptionString() const
    {
        std::string detailed_actor_description = fmt::format("{}:", name_);
        detailed_actor_description += fmt::format("\n\t{:o}", *wealth_);
        for (auto &emotion : emotions_)
        {
            detailed_actor_description += fmt::format("\n\t{:o}", *emotion);
        }
        for (auto &[actor_index, vector] : relationships_)
        {
            Actor *other_actor = school_.GetActor(actor_index);
            detailed_actor_description += fmt::format("\n\tWith #{} {}:", other_actor->id_, *other_actor);
            for (auto &relationship : vector)
            {
                detailed_actor_description += fmt::format("\n\t\t{:o}", *relationship);
            }
        }
        detailed_actor_description += fmt::format("\n\t{:o}", *goal_);
        return detailed_actor_description;
    }

    const std::list<Actor *> &Actor::GetAllKnownActors() const
    {
        return known_actors_;
    }
    std::list<Actor *> Actor::GetFreetimeActorGroup() const
    {
        return freetime_group;
    }

    void Actor::InitializeRandomWealth(size_t tick)
    {
        std::vector<Kernel *> no_reasons;
        wealth_ = chronicle_.CreateResource("wealth", "wealthy", "poor", tick, this, no_reasons, random_.GetFloat(-1.0f, 1.0f));
    }
    void Actor::InitializeRandomEmotions(size_t tick)
    {
        std::vector<Kernel *> no_reasons;
        emotions_ = std::vector<Emotion *>(static_cast<int>(EmotionType::kLast));
        for (int type_index = 0; type_index < emotions_.size(); ++type_index)
        {
            EmotionType type = static_cast<EmotionType>(type_index);
            emotions_[type_index] = chronicle_.CreateEmotion(type, tick, this, no_reasons, random_.GetFloat(-1.0f, 1.0f));
        }
    }
    void Actor::InitializeRandomRelationships(size_t tick)
    {
        std::vector<Kernel *> no_reasons;
        uint32_t relationship_count = random_.GetUInt(setting_.min_start_relationships_count(), setting_.max_start_relationships_count());
        for (size_t i = relationships_.size(); i < relationship_count; ++i)
        {
            uint32_t other_actor_id = random_.GetUInt(0, setting_.actor_count - 1);
            auto other_actor = school_.GetActor(other_actor_id);
            size_t tries = 0;
            while ((other_actor_id == id_ || HasRelationshipWith(other_actor_id) || other_actor->GetAllKnownActors().size() > setting_.desired_max_start_relationships_count) &&
                   tries < setting_.actor_count)
            {
                ++tries;
                ++other_actor_id;
                other_actor_id %= setting_.actor_count;
            }
            if (tries == setting_.actor_count)
            {
                return;
            }
            int type_count = static_cast<int>(RelationshipType::kLast);
            std::vector<Relationship *> relationship(type_count);
            std::vector<Relationship *> other_relationship(type_count);
            for (int type_index = 0; type_index < static_cast<int>(RelationshipType::kLast); ++type_index)
            {
                RelationshipType type = static_cast<RelationshipType>(type_index);
                relationship[type_index] = chronicle_.CreateRelationship(type, tick, this, other_actor, no_reasons, random_.GetFloat(-1.0f, 1.0f));
                other_relationship[type_index] = chronicle_.CreateRelationship(type, tick, other_actor, this, no_reasons, random_.GetFloat(-1.0f, 1.0f));
            }
            UpdateRelationship(other_actor, relationship);
            other_actor->UpdateRelationship(this, other_relationship);
        }
    }

    void Actor::UpdateRelationship(Actor *other_actor, std::vector<Relationship *> relationship, bool already_known)
    {
        relationships_[other_actor->id_] = relationship;
        float relationship_strength = CalculateRelationshipStrength(other_actor->id_);
        bool inserted = false;
        freetime_group.clear();
        int count = 0;
        for (auto it = known_actors_.begin(); it != known_actors_.end(); ++it)
        {
            if (CalculateRelationshipStrength((*it)->id_) <= relationship_strength && !inserted)
            {
                if (!already_known)
                {
                    known_actors_.insert(it, other_actor);
                    inserted = true;
                }
                else
                {
                    for (auto other_actor_iterator = known_actors_.begin(); other_actor_iterator != known_actors_.end(); ++other_actor_iterator)
                    {
                        if ((*other_actor_iterator)->id_ == other_actor->id_)
                        {
                            known_actors_.splice(it, known_actors_, other_actor_iterator);
                            inserted = true;
                        }
                    }
                }
                freetime_group.push_back(other_actor);
            }
            else
            {
                if (count < setting_.freetime_actor_count)
                {
                    freetime_group.push_back(*it);
                }
            }
            ++count;
        }
        if (!inserted && !already_known)
        {
            known_actors_.push_back(other_actor);
        }
    }
    void Actor::InitializeRandomGoal(size_t tick)
    {
        std::vector<Kernel *> no_reasons;
        goal_ = chronicle_.CreateGoal(Goal::GetRandomGoalType(random_), tick, this, no_reasons);
    }

    float Actor::CalculateRelationshipStrength(size_t actor_id) const
    {
        float value = 0;
        for (const auto &relationship : relationships_.at(actor_id))
        {
            value += abs(relationship->GetValue());
        }
        return value;
    }

    bool Actor::HasRelationshipWith(size_t actor_id) const
    {
        for (auto &[key, value] : relationships_)
        {
            if (key == actor_id)
            {
                return true;
            }
        }
        return false;
    }
} // namespace tattletale