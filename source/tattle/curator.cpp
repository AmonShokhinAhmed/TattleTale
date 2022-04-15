#include "tattle/curator.hpp"
#include "shared/actor.hpp"
#include <fmt/core.h>
#include <math.h>
#include <map>
#include <set>
#include <queue>
#include "shared/tattletalecore.hpp"
#include "tattle/curations/raritycuration.hpp"
#include "tattle/curations/absoluteinterestcuration.hpp"
#include "tattle/curations/tagcuration.hpp"
#include "tattle/curations/catcuration.hpp"
#include "tattle/curations/randomcuration.hpp"

namespace tattletale
{
    Curator::Curator(const Chronicle &chronicle, const Setting &setting) : chronicle_(chronicle), setting_(setting) {}

    Kernel *Curator::RecursivelyFindUnlikeliestReason(Kernel *to_check, Kernel *current_best)
    {
        if (to_check->GetChance() < current_best->GetChance())
        {
            current_best = to_check;
        }
        auto &reasons = to_check->GetReasons();
        for (auto &reason : reasons)
        {
            current_best = RecursivelyFindUnlikeliestReason(reason, current_best);
        }
        return current_best;
    }

    Kernel *Curator::RecursivelyFindUnlikeliestConsequence(Kernel *to_check, Kernel *current_best, size_t depth) const
    {
        if (!current_best || to_check->GetChance() < current_best->GetChance())
        {
            current_best = to_check;
        }
        if (depth > 0)
        {
            auto &consequences = to_check->GetConsequences();
            for (auto &consequence : consequences)
            {
                current_best = RecursivelyFindUnlikeliestConsequence(consequence, current_best, depth - 1);
            }
        }
        return current_best;
    }

    Resource *Curator::FindBlockingResource(Kernel *kernel) const
    {
        if (kernel->type_ != KernelType::kInteraction)
        {
            return nullptr;
        }
        auto interaction = dynamic_cast<Interaction *>(kernel);
        auto tendency = interaction->GetTendency();
        float lowest_influence = 1.0f;
        Resource *blocking_resource = nullptr;
        for (int type_index = 0; type_index < static_cast<int>(EmotionType::kLast); ++type_index)
        {
            float value = tendency->emotions[type_index];
            EmotionType type = static_cast<EmotionType>(type_index);
            if (value == 0)
            {
                continue;
            }
            auto last_emotion = chronicle_.GetLastEmotionOfType(interaction->tick_, interaction->GetOwner()->id_, type);
            float current_influence = 0;
            if (last_emotion)
            {
                current_influence = last_emotion->GetValue() * value;
            }
            if (current_influence < lowest_influence)
            {
                blocking_resource = last_emotion;
                lowest_influence = current_influence;
            }
        }
        if (tendency->wealth != 0)
        {
            auto last_wealth = chronicle_.GetLastWealth(interaction->tick_, interaction->GetOwner()->id_);
            float current_influence = last_wealth->GetValue() * tendency->wealth;
            if (current_influence < lowest_influence)
            {
                blocking_resource = last_wealth;
                lowest_influence = current_influence;
            }
        }
        return blocking_resource;
    }

    bool Curator::HasCausalConnection(Kernel *start, Kernel *end) const
    {
        std::queue<Kernel *> queue;
        queue.push(start);
        while (queue.size() > 0)
        {
            Kernel *current = queue.front();
            queue.pop();
            for (auto &reason : current->GetReasons())
            {
                if (reason->id_ == end->id_)
                {
                    return true;
                }

                if (reason->tick_ <= end->tick_)
                {
                    queue.push(reason);
                }
            }
        }
        return false;
    }

    std::vector<Kernel *> Curator::FindCausalConnection(Kernel *start, Kernel *end) const
    {
        std::vector<Kernel *> causal_chain;
        RecursivelyFindCausalConnectionBackwards(end, start, causal_chain);
        return causal_chain;
    }

    bool Curator::RecursivelyFindCausalConnectionBackwards(Kernel *root, Kernel *start, std::vector<Kernel *> &out_causal_chain) const
    {
        if (root->id_ == start->id_)
        {
            out_causal_chain.push_back(root);
            return true;
        }
        if (root->tick_ < start->tick_)
        {
            return false;
        }
        for (auto &reason : root->GetReasons())
        {
            if (RecursivelyFindCausalConnectionBackwards(reason, start, out_causal_chain))
            {
                out_causal_chain.push_back(root);
                return true;
            }
        }
        return false;
    }

    std::string Curator::GetTimeDescription(Kernel *start, Kernel *end, bool first_letter_uppercase) const
    {
        bool reversed = (start->tick_ > end->tick_);
        size_t tick_distance = 0;
        if (reversed)
        {
            tick_distance = start->tick_ - end->tick_;
        }
        else
        {
            tick_distance = end->tick_ - start->tick_;
        }
        std::string description = (reversed ? "quite a lot of time later" : "quite a lot of time earlier");

        if (tick_distance == 0)
        {
            description = "at the same time";
        }
        if (tick_distance < (setting_.courses_per_day + 1) / 2)
        {
            description = (reversed ? "shortly after" : "shortly before");
        }
        if (tick_distance < setting_.courses_per_day + 1)
        {
            description = (reversed ? "later that day" : "earlier that day");
        }
        size_t days = tick_distance / (setting_.courses_per_day + 1);
        if (days <= 1)
        {
            description = (reversed ? "the following day" : "the previous day");
        }
        if (days <= 7)
        {
            description = "during the same week";
        }
        if (days <= 14)
        {
            description = (reversed ? "about two weeks later" : "about two weeks earlier");
        }
        if (days <= 31)
        {
            description = "during the same month";
        }
        if (first_letter_uppercase)
        {
            description[0] = toupper(description[0]);
        }
        return description;
    }

    std::string Curator::GetChanceDescription(float chance) const
    {
        if (chance < 0.1)
        {
            return "impossibly rare";
        }
        if (chance < 0.2)
        {
            return "rare";
        }
        if (chance < 0.3)
        {
            return "highly unlikely";
        }
        if (chance < 0.4)
        {
            return "unlikely";
        }
        if (chance < 0.5)
        {
            return "slightly unlikely";
        }
        if (chance < 0.6)
        {
            return "normal";
        }
        if (chance < 0.7)
        {
            return "somewhat common";
        }
        if (chance < 0.8)
        {
            return "common";
        }
        if (chance < 0.9)
        {
            return "very common";
        }
        return "completely banal";
    }

    std::string Curator::GenerateScoreDescription(float score) const
    {
        if (score < 0.1)
        {

            return "very boring";
        }
        if (score < 0.2)
        {
            return "boring";
        }
        if (score < 0.3)
        {
            return "somewhat boring";
        }
        if (score < 0.4)
        {
            return "generic";
        }
        if (score < 0.5)
        {
            return "slightly interesting";
        }
        if (score < 0.6)
        {
            return "interesting";
        }
        if (score < 0.7)
        {
            return "highly interesting";
        }
        if (score < 0.8)
        {
            return "fascinating";
        }
        if (score < 0.9)
        {
            return "highly fascinating";
        }
        return "mindblowingly fascinating";
    }

    std::string Curator::GetResourceReasonDescription(Resource *resource) const
    {
        float value = abs(resource->GetValue());
        if (value > 0.9f)
        {
            return fmt::format("resigning to their fate of {:p}, they just had to", *resource);
        }
        if (value > 0.7f)
        {
            return fmt::format("{:p} compelled them to", *resource);
        }
        if (value > 0.5f)
        {
            return fmt::format("them {:p} led them to", *resource);
        }
        if (value > 0.4f)
        {
            return fmt::format("because they were {:p} they had a feeling that they needed to", *resource);
        }
        if (value > 0.2f)
        {
            return fmt::format("{:p} gave them a slight excuse to", *resource);
        }
        return fmt::format("{:p} gave them a tiny nudge to", *resource);
    }

    Actor *Curator::FindMostOccuringActor(const std::vector<Kernel *> &kernels, bool &out_more_actors_present) const
    {
        std::set<size_t> checked_actors;
        Actor *highest_actor = nullptr;
        size_t highest_occurence = 0;
        for (auto kernel : kernels)
        {
            auto actors = kernel->GetAllParticipants();
            for (auto &actor : actors)
            {
                size_t occurence = 0;
                if (checked_actors.find(actor->id_) == checked_actors.end())
                {
                    checked_actors.insert(actor->id_);
                    for (auto kernel_to_check : kernels)
                    {
                        auto actors_to_check = kernel_to_check->GetAllParticipants();
                        for (auto &actor_to_check : actors_to_check)
                        {
                            if (actor_to_check->id_ == actor->id_)
                            {
                                ++occurence;
                            }
                        }
                    }
                    if (occurence > highest_occurence)
                    {
                        highest_occurence = occurence;
                        highest_actor = actor;
                    }
                }
            }
        }
        out_more_actors_present = (checked_actors.size() > 1);
        return highest_actor;
    }

    std::string Curator::UseAllCurations()
    {
        TATTLETALE_DEBUG_PRINT("START CURATION");

        size_t max_chain_size = 5;
        const auto &chains = chronicle_.GetEveryPossibleChain(max_chain_size);

        std::string preamble = "{} Curation:\n\n{}\n\n-------------------------------------------------------------------------------------------------------------------------------------------------\n\n";

        std::string narrative = "";

        std::vector<Curation *> curations;
        curations.push_back(new RarityCuration(max_chain_size));
        curations.push_back(new AbsoluteInterestCuration(max_chain_size));
        curations.push_back(new TagCuration(max_chain_size));
        // curations.push_back(new CatCuration(max_chain_size));
        curations.push_back(new RandomCuration(max_chain_size, chronicle_.GetRandom()));

        for (auto &curation : curations)
        {
            TATTLETALE_DEBUG_PRINT(fmt::format("{} Curation...", curation->name_));
            narrative += fmt::format(preamble, curation->name_, Curate(chains, curation));
        }
        for (auto &curation : curations)
        {
            delete curation;
        }
        curations.clear();

        return narrative;
    }

    std::string Curator::Narrativize(const std::vector<Kernel *> &chain, const Curation *curation) const
    {
        // TODO: give current status of players
        // TODO: track which actors were alread named and only use firstnames for those
        // TODO: repeated interactions should be combined
        // TODO: resource are summarized even when they change sign

        bool more_than_one_actor_present = false;
        auto protagonist = FindMostOccuringActor(chain, more_than_one_actor_present);
        auto normal_interaction = chronicle_.FindMostOccuringInteractionPrototypeForActor(protagonist->id_);

        std::string acquaintance_description = (more_than_one_actor_present ? " and their acquaintances" : "");

        float score = curation->CalculateScore(chain);
        std::string score_description = GenerateScoreDescription(score);
        Kernel *first_noteworthy_event = curation->GetFirstNoteworthyEvent(chain);
        Kernel *second_noteworthy_event = curation->GetSecondNoteworthyEvent(chain);
        if (!first_noteworthy_event || !second_noteworthy_event)
        {
            return "Narrativization failed. No noteworthy events were created.";
        }
        bool only_one_noteworthy_event = first_noteworthy_event->id_ == second_noteworthy_event->id_;
        std::string description = "";
        if (normal_interaction)
        {
            description += fmt::format("Normally one would find {} {:p} but this time s", *protagonist, *normal_interaction);
        }
        else
        {
            description += "S";
        }
        std::string event_count_description = fmt::format((only_one_noteworthy_event ? "omething {}" : "ome {} events"), score_description);
        description += fmt::format("{} happened around them{}.\n", event_count_description, acquaintance_description);
        description += fmt::format("One of those events would be when {}", *first_noteworthy_event);
        if (!only_one_noteworthy_event)
        {
            auto time_description = GetTimeDescription(first_noteworthy_event, second_noteworthy_event, false);

            auto blocking_resource = FindBlockingResource(second_noteworthy_event);
            std::string blocking_description = "";
            if (blocking_resource)
            {
                blocking_description = fmt::format(" despite {:p},", *blocking_resource);
            }
            description += fmt::format(", but this was not even the most noteworthy thing that happened because {}{} {} was found {:p}{}.\n\n",
                                       time_description,
                                       blocking_description,
                                       *second_noteworthy_event->GetOwner(),
                                       *second_noteworthy_event,
                                       (second_noteworthy_event->IsSameSpecificType(first_noteworthy_event) ? " again" : ""));
        }
        else
        {
            description += ".\n\n";
        }
        description += "So what else happend?\n";
        auto previous_kernel = chain[0];
        description += fmt::format("The whole sequence of events started when {}", *previous_kernel);
        auto previous_reasons = previous_kernel->GetReasons();
        if (previous_reasons.size() > 0)
        {
            description += fmt::format(", because {:f} was {:p}", *(previous_kernel->GetOwner()), *previous_reasons[0]);
            for (size_t reason_index = 1; reason_index < previous_reasons.size(); ++reason_index)
            {
                auto &reason = previous_reasons[reason_index];
                description += fmt::format(" and was also {:p}", *reason);
            }
        }
        for (size_t index = 1; index < chain.size(); ++index)
        {
            Kernel *kernel = chain[index];
            if (kernel->IsSameSpecificType(previous_kernel))
            {
                previous_reasons = kernel->GetReasons();
                previous_kernel = kernel;
                continue;
            }
            if (kernel->type_ != KernelType::kInteraction)
            {

                description += " which in turn let ";
            }
            else
            {
                description += ".\nThis ";
                for (auto &reason : kernel->GetReasons())
                {
                    if (reason->id_ != previous_kernel->id_ && !reason->IsSameSpecificType(kernel))
                    {
                        if (reason->GetOwner()->id_ == kernel->GetOwner()->id_)
                        {
                            description += fmt::format("and them {:p} ", *reason);
                        }
                        else
                        {
                            description += fmt::format("and {:f} {:p} ", *(reason->GetOwner()), *reason);
                        }
                    }
                }
                description += "made ";
            }
            bool compound_reason = false;
            for (auto &previous_reason : previous_reasons)
            {
                if (previous_reason->IsSameSpecificType(kernel) && previous_reason->GetOwner()->id_ == kernel->GetOwner()->id_)
                {
                    compound_reason = true;
                }
            }

            description += fmt::format("{} {:a}{}", *(kernel->GetOwner()), *kernel, (compound_reason ? " even more" : ""));
            previous_reasons = kernel->GetReasons();
            previous_kernel = kernel;
        }
        return description;
    }

    std::string Curator::Curate(const std::vector<std::vector<Kernel *>> &chains, Curation *curation) const
    {
        auto chain = FindBestScoringChain(chains, curation);
        if (chain.size() < 0)
        {
            return fmt::format("{} Curation failed. No valid Kernels were created.", curation->name_);
        }
        return Narrativize(chain, curation);
    }

    std::vector<Kernel *> Curator::FindBestScoringChain(const std::vector<std::vector<Kernel *>> chains, Curation *curation) const
    {
        float highest_score = 0.0f;
        std::vector<Kernel *> highest_chain;
        for (auto &chain : chains)
        {
            float score = curation->CalculateScore(chain);
            if (score > highest_score)
            {
                highest_score = score;
                highest_chain = chain;
            }
        }
        return highest_chain;
    }
} // namespace tattletale
