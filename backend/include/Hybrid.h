#pragma once
#include "Recommender.h"
#include "ContentBased.h"
#include "UserBased.h"
#include <vector>

// Configuration for hybrid blending
namespace HybridConfig {
    constexpr float CONTENT_WEIGHT = 0.4f;  // Weight of content-based score
    constexpr float USER_WEIGHT = 0.6f;     // Weight of user-based score
    // Sum should equal 1.0
}

class Hybrid : public Recommender {
private:
    ContentBased contentStrategy;
    UserBased userStrategy;

public:
    std::vector<Recommendation> recommend(
        const std::vector<Track>& tracks,
        const User& user,
        size_t limit
    ) override;
};
