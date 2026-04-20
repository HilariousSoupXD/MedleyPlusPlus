#pragma once
#include "Recommender.h"

class ContentBased : public Recommender {
public:
    std::vector<Recommendation> recommend(
        const std::vector<Track>& tracks,
        const User& user
    ) override;
};