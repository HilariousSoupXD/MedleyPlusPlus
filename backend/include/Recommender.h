#pragma once
#include <vector>
#include "Track.h"
#include "User.h"
#include "Recommendation.h"

class Recommender {
public:
    virtual std::vector<Recommendation> recommend(
        const std::vector<Track>& tracks,
        const User& user,
        size_t limit
    ) = 0;

    virtual ~Recommender() {}
};