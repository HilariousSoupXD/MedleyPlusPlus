#include "../include/ContentBased.h"
#include <algorithm>
#include <cmath>
#include <sstream>

// Feature weights (sum should be close to 1.0 for interpretability)
namespace ContentBasedWeights {
    constexpr float DANCEABILITY = 0.12f;
    constexpr float ENERGY = 0.15f;
    constexpr float VALENCE = 0.12f;
    constexpr float ACOUSTICNESS = 0.08f;  // inverted preference
    constexpr float INSTRUMENTALNESS = 0.08f;  // inverted preference
    constexpr float LIVENESS = 0.06f;
    constexpr float SPEECHINESS = 0.05f;  // inverted preference
    constexpr float TEMPO = 0.09f;  // normalized
    constexpr float POPULARITY = 0.25f;  // normalized
}

// Normalize tempo to 0-1 range (typical range: 50-200 BPM)
static float normalizeTempo(float tempo) {
    return std::min(1.0f, std::max(0.0f, tempo / 200.0f));
}

// Normalize popularity to 0-1 range
static float normalizePopularity(int popularity) {
    return std::min(1.0f, std::max(0.0f, popularity / 100.0f));
}

// Compute weighted content-based score for a track
static float computeContentScore(const Track& t) {
    using namespace ContentBasedWeights;
    
    float score =
        DANCEABILITY * t.danceability +
        ENERGY * t.energy +
        VALENCE * t.valence +
        ACOUSTICNESS * (1.0f - t.acousticness) +  // prefer less acoustic
        INSTRUMENTALNESS * (1.0f - t.instrumentalness) +  // prefer less instrumental
        LIVENESS * t.liveness +
        SPEECHINESS * (1.0f - t.speechiness) +  // prefer less speech
        TEMPO * normalizeTempo(t.tempo) +
        POPULARITY * normalizePopularity(t.popularity);
    
    return score;
}

// Generate explanation based on dominant features
static std::string generateContentExplanation(const Track& t) {
    std::stringstream ss;
    
    // Find dominant features
    float energy_score = 0.15f * t.energy;
    float dance_score = 0.12f * t.danceability;
    float valence_score = 0.12f * t.valence;
    float pop_score = 0.25f * (t.popularity / 100.0f);
    
    // Combine explanations
    bool high_energy = energy_score > 0.10f;
    bool high_dance = dance_score > 0.08f;
    bool high_valence = valence_score > 0.08f;
    bool high_pop = pop_score > 0.15f;
    
    if (high_pop) {
        ss << "Popular track";
    } else {
        ss << "Quality track";
    }
    
    if (high_energy) {
        ss << " with high energy";
    }
    if (high_dance) {
        ss << " and highly danceable";
    }
    if (high_valence && !high_energy) {
        ss << " with positive mood";
    }
    
    return ss.str();
}

std::vector<Recommendation> ContentBased::recommend(
    const std::vector<Track>& tracks,
    const User& user
) {
    std::vector<std::pair<Track, float>> scored;
    
    for (const auto& t : tracks) {
        scored.emplace_back(t, computeContentScore(t));
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
            scored[i].second,
            generateContentExplanation(scored[i].first)
        });
    }
    
    return result;
}