#include "../include/Hybrid.h"
#include <algorithm>
#include <cmath>
#include <sstream>

// Helper: compute content-based score (normalized 0-1)
static float getContentScore(const Track& t) {
    // Must match ContentBased.cpp weights
    float score =
        0.12f * t.danceability +
        0.15f * t.energy +
        0.12f * t.valence +
        0.08f * (1.0f - t.acousticness) +
        0.08f * (1.0f - t.instrumentalness) +
        0.06f * t.liveness +
        0.05f * (1.0f - t.speechiness) +
        0.09f * (t.tempo / 200.0f) +
        0.25f * (t.popularity / 100.0f);
    
    return std::min(1.0f, std::max(0.0f, score));
}

// Helper: compute user-based score (normalize to 0-1)
static float getUserScore(const Track& t, const User& user) {
    auto normalizeTempo = [](float tempo) {
        return std::min(1.0f, std::max(0.0f, tempo / 200.0f));
    };
    
    auto normalizePopularity = [](int popularity) {
        return std::min(1.0f, std::max(0.0f, popularity / 100.0f));
    };
    
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
    float diff_popularity = std::abs(norm_pop_track - 0.5f);
    
    float variance_factor_energy = 1.0f / (1.0f + user.var_energy);
    float variance_factor_danceability = 1.0f / (1.0f + user.var_danceability);
    float variance_factor_valence = 1.0f / (1.0f + user.var_valence);
    
    float distance =
        0.14f * diff_energy * variance_factor_energy +
        0.12f * diff_danceability * variance_factor_danceability +
        0.12f * diff_valence * variance_factor_valence +
        0.10f * diff_acousticness +
        0.10f * diff_instrumentalness +
        0.08f * diff_liveness +
        0.08f * diff_speechiness +
        0.13f * diff_tempo +
        0.13f * diff_popularity;
    
    // Invert distance to score (lower distance = higher score)
    return std::max(0.0f, 1.0f - distance);
}

// Generate hybrid explanation combining both perspectives
static std::string generateHybridExplanation(
    float content_score, 
    float user_score
) {
    std::stringstream ss;
    
    bool strong_content = content_score > 0.75f;
    bool strong_user = user_score > 0.7f;
    
    if (strong_content && strong_user) {
        ss << "Popular and matches your taste";
    } else if (strong_content) {
        ss << "Highly rated track";
    } else if (strong_user) {
        ss << "Close to your listening profile";
    } else {
        ss << "Balanced recommendation";
    }
    
    return ss.str();
}

std::vector<Recommendation> Hybrid::recommend(
    const std::vector<Track>& tracks,
    const User& user
) {
    struct ScoredTrack {
        Track track;
        float content_score;
        float user_score;
        float hybrid_score;
    };
    
    std::vector<ScoredTrack> scored;
    
    for (const auto& t : tracks) {
        float cs = getContentScore(t);
        float us = getUserScore(t, user);
        float hs = HybridConfig::CONTENT_WEIGHT * cs + 
                   HybridConfig::USER_WEIGHT * us;
        
        scored.emplace_back(ScoredTrack{t, cs, us, hs});
    }
    
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) {
            return a.hybrid_score > b.hybrid_score;
        }
    );
    
    std::vector<Recommendation> result;
    
    size_t limit = std::min(size_t(5), scored.size());
    for (size_t i = 0; i < limit; i++) {
        result.emplace_back(Recommendation{
            scored[i].track,
            scored[i].hybrid_score,
            generateHybridExplanation(scored[i].content_score, scored[i].user_score)
        });
    }
    
    return result;
}
