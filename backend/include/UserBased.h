#pragma once
#include "Recommender.h"

class UserBased : public Recommender {
public:
    std::vector<Recommendation> recommend(
        const std::vector<Track>& tracks,
        const User& user
    ) override;
};