#include "shared/chronicle.hpp"
#include "shared/tattletalecore.hpp"
#include "shared/actor.hpp"
#include "tale/school.hpp"

namespace tattletale
{
    Chronicle::Chronicle(Random &random) : random_(random){};

    Chronicle::~Chronicle() { Reset(); }
    void Chronicle::Reset()
    {
        for (size_t i = 0; i < actors_.size(); ++i)
        {
            delete actors_[i];
        }
        for (size_t i = 0; i < all_kernels_.size(); ++i)
        {
            delete all_kernels_[i];
        }
        actors_.clear();
        kernels_by_actor_.clear();
        interactions_by_actor_.clear();
        wealth_by_actor_.clear();
        emotions_by_actor_.clear();
        all_kernels_.clear();
        all_interactions_.clear();
    }

    Actor *Chronicle::CreateActor(School &school, std::string first_name, std::string last_name)
    {
        Actor *actor = new Actor(school, actors_.size(), first_name, last_name);
        actors_.push_back(actor);
        kernels_by_actor_.push_back(std::vector<Kernel *>());
        emotions_by_actor_.push_back(std::vector<Emotion *>());
        wealth_by_actor_.push_back(std::vector<Resource *>());
        interactions_by_actor_.push_back(std::vector<Interaction *>());
        return actor;
    }
    Interaction *Chronicle::CreateInteraction(
        const std::shared_ptr<InteractionPrototype> prototype,
        const std::shared_ptr<InteractionRequirement> requirement,
        const std::shared_ptr<InteractionTendency> tendency,
        float chance,
        size_t tick,
        std::vector<Kernel *> reasons,
        std::vector<Actor *> participants)
    {
        Interaction *interaction = new Interaction(prototype, requirement, tendency, chance, all_kernels_.size(), tick, reasons, participants);
        for (auto &reason : reasons)
        {
            reason->AddConsequence(interaction);
        }
        all_kernels_.push_back(interaction);
        for (auto &owner : participants)
        {
            kernels_by_actor_[owner->id_].push_back(interaction);
            interactions_by_actor_[owner->id_].push_back(interaction);
        }
        all_interactions_.push_back(interaction);
        if (prototype->id > highest_interaction_id)
        {
            highest_interaction_id = prototype->id;
        }
        return interaction;
    }
    Emotion *Chronicle::CreateEmotion(EmotionType type, size_t tick, Actor *owner, std::vector<Kernel *> reasons, float value)
    {
        Emotion *emotion = new Emotion(type, all_kernels_.size(), tick, owner, reasons, value);
        for (auto &reason : reasons)
        {
            reason->AddConsequence(emotion);
        }
        emotions_by_actor_[owner->id_].push_back(emotion);
        all_kernels_.push_back(emotion);
        kernels_by_actor_[owner->id_].push_back(emotion);
        return emotion;
    }
    Relationship *Chronicle::CreateRelationship(RelationshipType type, size_t tick, Actor *owner, Actor *target, std::vector<Kernel *> reasons, float value)
    {
        Relationship *relationship = new Relationship(type, all_kernels_.size(), tick, owner, target, reasons, value);
        for (auto &reason : reasons)
        {
            reason->AddConsequence(relationship);
        }
        all_kernels_.push_back(relationship);
        kernels_by_actor_[owner->id_].push_back(relationship);
        return relationship;
    }
    Resource *Chronicle::CreateResource(std::string name, std::string positive_name_variant, std::string negative_name_variant, size_t tick, Actor *owner, std::vector<Kernel *> reasons, float value)
    {
        Resource *resource = new Resource(name, positive_name_variant, negative_name_variant, all_kernels_.size(), tick, owner, reasons, value);
        for (auto &reason : reasons)
        {
            reason->AddConsequence(resource);
        }
        all_kernels_.push_back(resource);
        kernels_by_actor_[owner->id_].push_back(resource);
        wealth_by_actor_[owner->id_].push_back(resource);
        return resource;
    }
    Goal *Chronicle::CreateGoal(GoalType type, size_t tick, Actor *owner, std::vector<Kernel *> reasons)
    {
        Goal *goal = new Goal(type, all_kernels_.size(), tick, owner, reasons);
        for (auto &reason : reasons)
        {
            reason->AddConsequence(goal);
        }
        all_kernels_.push_back(goal);
        kernels_by_actor_[owner->id_].push_back(goal);
        return goal;
    }
    Trait *Chronicle::CreateTrait(std::string name, size_t tick, Actor *owner, std::vector<Kernel *> reasons)
    {
        Trait *trait = new Trait(name, all_kernels_.size(), tick, owner, reasons);
        for (auto &reason : reasons)
        {
            reason->AddConsequence(trait);
        }
        all_kernels_.push_back(trait);
        kernels_by_actor_[owner->id_].push_back(trait);
        return trait;
    }

    float Chronicle::GetAverageInteractionChance() const
    {
        float sum = 0;
        for (auto interaction : all_interactions_)
        {
            sum += interaction->chance_;
        }
        return (sum / all_interactions_.size());
    }
    float Chronicle::GetAverageInteractionReasonCount() const
    {
        float sum = 0;
        int count = 0;
        for (auto interaction : all_interactions_)
        {
            if (interaction->GetReasons().size() == 0)
            {
                count++;
            }
            sum += interaction->GetReasons().size();
        }
        return (sum / all_interactions_.size());
    }

    std::string Chronicle::GetKnownActorsDescription(size_t actor_id) const
    {
        std::string description;
        Actor *actor = actors_[actor_id];
        std::list<Actor *> known_actors = actor->GetAllKnownActors();
        for (auto &other_actor : known_actors)
        {
            description += fmt::format("{} known with value {}\n", *other_actor, actor->CalculateRelationshipStrength(other_actor->id_));
        }
        return description;
    }
    size_t Chronicle::GetKernelAmount() const
    {
        return all_kernels_.size();
    }

    std::string Chronicle::GetRandomCausalityChainDescription(size_t depth) const
    {
        if (all_kernels_.size() <= 0)
        {
            return "";
        }
        auto kernel = all_kernels_[random_.GetUInt(0, all_kernels_.size() - 1)];
        return GetRecursiveKernelDescription(kernel, 0, depth);
    }

    std::string Chronicle::GetKissingCausalityChainDescription(size_t depth) const
    {
        if (all_kernels_.size() <= 0)
        {
            return "";
        }
        std::vector<Kernel *> possible_kernels;
        for (auto kernel : all_kernels_)
        {
            if (kernel->name_ == "Kiss successfully")
            {
                possible_kernels.push_back(kernel);
            }
        }
        if (possible_kernels.size() > 0)
        {
            auto kernel = possible_kernels[random_.GetUInt(0, possible_kernels.size() - 1)];
            return GetRecursiveKernelDescription(kernel, 0, depth);
        }
        return "Did not find a kiss.";
    }

    std::string Chronicle::GetActorInteractionsDescription(size_t id) const
    {
        if (actors_.size() < id)
        {
            return "No Actor of this ID exists.";
        }
        auto &kernels = kernels_by_actor_[id];
        if (kernels.size() <= 0)
        {
            return "No Kernels were created for this ID.";
        }
        std::string description = fmt::format("Interactions for {}", *actors_[id]);
        for (size_t i = 0; i < kernels.size(); ++i)
        {
            if (dynamic_cast<Interaction *>(kernels[i]))
            {
                description += fmt::format("\n{}", *kernels[i]);
            }
        }
        return description;
    }

    Interaction *Chronicle::FindUnlikeliestInteraction(size_t tick_cutoff) const
    {
        Interaction *unlikeliest_interaction(nullptr);
        for (auto &interaction : all_interactions_)
        {
            if (interaction->tick_ > tick_cutoff)
            {
                break;
            }
            if (interaction->GetReasons().size() > 0)
            {
                if (!unlikeliest_interaction)
                {
                    unlikeliest_interaction = interaction;
                }
                else
                {
                    if (interaction->chance_ < unlikeliest_interaction->chance_)
                    {
                        unlikeliest_interaction = interaction;
                    }
                }
            }
        }
        return unlikeliest_interaction;
    }
    Interaction *Chronicle::FindMostOccuringInteractionPrototypeForActor(size_t actor_id) const
    {
        std::vector<size_t> occurences(highest_interaction_id + 1);
        for (auto &interaction : interactions_by_actor_[actor_id])
        {
            occurences[interaction->GetPrototype()->id] += 1;
        }
        size_t highest = 0;
        for (size_t i = 1; i < occurences.size(); ++i)
        {
            if (occurences[i] > highest)
            {
                highest = i;
            }
        }
        for (auto &interaction : interactions_by_actor_[actor_id])
        {
            auto prototype = interaction->GetPrototype();
            if (prototype->id == highest)
            {
                return interaction;
            };
        }
        return nullptr;
    }

    size_t Chronicle::GetLastTick() const
    {
        return all_kernels_.back()->tick_;
    }
    Emotion *Chronicle::GetLastEmotionOfType(size_t tick, size_t actor_id, EmotionType type) const
    {
        Emotion *last_emotion = nullptr;
        for (auto &emotion : emotions_by_actor_[actor_id])
        {
            if (emotion->type_ != type)
            {
                continue;
            }
            if (emotion->tick_ >= tick)
            {
                break;
            }
            last_emotion = emotion;
        }
        return last_emotion;
    }

    Resource *Chronicle::GetLastWealth(size_t tick, size_t actor_id) const
    {
        Resource *last_wealth = nullptr;
        for (auto &wealth : wealth_by_actor_[actor_id])
        {
            if (wealth->tick_ >= tick)
            {
                break;
            }
            last_wealth = wealth;
        }
        return last_wealth;
    }

    std::string Chronicle::GetGoalCausalityChainDescription(size_t depth) const
    {
        if (all_kernels_.size() <= 0)
        {
            return "";
        }
        std::vector<Kernel *> possible_kernels;
        for (auto kernel : all_kernels_)
        {
            for (auto reason : kernel->GetReasons())
            {
                if (dynamic_cast<Goal *>(reason))
                {
                    possible_kernels.push_back(kernel);
                    break;
                }
            }
        }
        if (possible_kernels.size() > 0)
        {
            auto kernel = possible_kernels[random_.GetUInt(0, possible_kernels.size() - 1)];
            return GetRecursiveKernelDescription(kernel, 0, depth);
        }
        return "Did not find a kernel with a goal as reason.";
    }

    std::string Chronicle::GetRecursiveKernelDescription(Kernel *kernel, size_t current_depth, size_t max_depth) const
    {
        std::string description = fmt::format("D{}", current_depth);
        for (size_t i = 0; i < current_depth; ++i)
        {
            description += "-";
        }
        description += fmt::format(":{} (T{})\n", *kernel, kernel->tick_);
        if (current_depth < max_depth)
        {
            for (size_t i = 0; i < current_depth; ++i)
            {
                description += " ";
            }

            if (kernel->GetReasons().size() > 0)
            {
                description += "   Because: \n";
                for (auto &reason : kernel->GetReasons())
                {
                    description += GetRecursiveKernelDescription(reason, current_depth + 1, max_depth);
                }
            }
            else
            {
                description += "   For no reason.\n";
            }
        }
        return description;
    }
} // namespace tattletale