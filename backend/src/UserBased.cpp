#include "../include/UserBased.h"
#include <algorithm>
#include <cmath>
#include <sstream>

// Feature weights for user-similarity calculation
namespace UserBasedWeights {
    constexpr float ENERGY = 0.14f;
    constexpr float DANCEABILITY = 0.12f;
    constexpr float VALENCE = 0.12f;
    constexpr float ACOUSTICNESS = 0.10f;
    constexpr float INSTRUMENTALNESS = 0.10f;
    constexpr float LIVENESS = 0.08f;
    constexpr float SPEECHINESS = 0.08f;
    constexpr float TEMPO = 0.13f;  // normalized
    constexpr float POPULARITY = 0.13f;  // normalized
}

// Normalize tempo to 0-1 range
static float normalizeTempo(float tempo) {
    return std::min(1.0f, std::max(0.0f, tempo / 200.0f));
}

// Normalize popularity to 0-1 range
static float normalizePopularity(int popularity) {
    return std::min(1.0f, std::max(0.0f, popularity / 100.0f));
}

// Compute weighted Euclidean distance between user profile and track
// Lower distance = more similar to user preference
// Returns negative distance (for sorting: higher score = better)
static float computeUserSimilarityScore(const Track& t, const User& user) {
    using namespace UserBasedWeights;
    
    // Compute normalized differences
    float diff_energy = std::abs(t.energy - user.avg_energy);
    float diff_danceability = std::abs(t.danceability - user.avg_danceability);
    float diff_valence = std::abs(t.valence - user.avg_valence);
    float diff_acousticness = std::abs(t.acousticness - user.avg_acousticness);
    float diff_instrumentalness = std::abs(t.instrumentalness - user.avg_instrumentalness);
    float diff_liveness = std::abs(t.liveness - user.avg_liveness);
    float diff_speechiness = std::abs(t.speechiness - user.avg_speechiness);
    
    float norm_tempo_track = normalizeTempo(t.tempo);
    float norm_tempo_user = normalizeTempo(user.avg_tempo);
    float diff_tempo = std::abs(norm_tempo_track - norm_tempo_user);
    
    float norm_pop_track = normalizePopularity(t.popularity);
    float norm_pop_user = 0.5f;  // neutral baseline for popularity
    float diff_popularity = std::abs(norm_pop_track - norm_pop_user);
    
    // Variance-weighted distance (features with higher variance get lower weight penalty)
    float variance_factor_energy = 1.0f / (1.0f + user.var_energy);
    float variance_factor_danceability = 1.0f / (1.0f + user.var_danceability);
    float variance_factor_valence = 1.0f / (1.0f + user.var_valence);
    
    // Weighted distance (lower is better, so negate for sorting)
    float distance =
        ENERGY * diff_energy * variance_factor_energy +
        DANCEABILITY * diff_danceability * variance_factor_danceability +
        VALENCE * diff_valence * variance_factor_valence +
        ACOUSTICNESS * diff_acousticness +
        INSTRUMENTALNESS * diff_instrumentalness +
        LIVENESS * diff_liveness +
        SPEECHINESS * diff_speechiness +
        TEMPO * diff_tempo +
        POPULARITY * diff_popularity;
    
    return -distance;  // Negate so higher score = better match
}

// Generate explanation based on matching features
static std::string generateUserExplanation(const Track& t, const User& user) {
    std::stringstream ss;
    
    // Find dominant matching features
    float energy_diff = std::abs(t.energy - user.avg_energy);
    float dance_diff = std::abs(t.danceability - user.avg_danceability);
    float valence_diff = std::abs(t.valence - user.avg_valence);
    
    // Rank by similarity (lower diff = better match)
    if (energy_diff < 0.1f && dance_diff < 0.1f) {
        ss << "Matches your energy and mood";
    } else if (energy_diff < 0.1f) {
        ss << "Matches your energy preference";
    } else if (dance_diff < 0.1f) {
        ss << "Matches your danceability taste";
    } else if (valence_diff < 0.1f) {
        ss << "Similar mood to your preference";
    } else if (energy_diff < 0.15f || dance_diff < 0.15f) {
        ss << "Close to your listening profile";
    } else {
        ss << "Aligned with your taste";
    }
    
    return ss.str();
}

std::vector<Recommendation> UserBased::recommend(
    const std::vector<Track>& tracks,
    const User& user
) {
    std::vector<std::pair<Track, float>> scored;
    
    for (const auto& t : tracks) {
        scored.emplace_back(t, computeUserSimilarityScore(t, user));
    }

    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        }
    );

    std::vector<Recommendation> result;
    size_t limit = std::min(size_t(5), scored.size());
    
    for (size_t i = 0; i < limit; i++) {
        result.emplace_back(Recommendation{
            scored[i].first,
            std::max(0.0f, scored[i].second + 1.0f),  // Convert to 0-1 scale
            generateUserExplanation(scored[i].first, user)
        });
    }
    
    return result;
}