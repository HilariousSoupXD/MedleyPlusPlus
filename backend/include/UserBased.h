#pragma once
#include "Recommender.h"
#include "InteractionStore.h"

class UserBased : public Recommender {
public:
    explicit UserBased(const InteractionStore* interactionStore = nullptr)
        : interactionStore_(interactionStore) {}

    std::vector<Recommendation> recommend(
        const std::vector<Track>& tracks,
        const User& user,
        size_t limit
    ) override;

private:
    const InteractionStore* interactionStore_;
};