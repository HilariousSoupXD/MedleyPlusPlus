#include "../include/ContentBased.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_set>

// Feature weights for user-to-item content similarity.
namespace ContentBasedWeights {
    constexpr float DANCEABILITY = 0.16f;
    constexpr float ENERGY = 0.16f;
    constexpr float VALENCE = 0.13f;
    constexpr float ACOUSTICNESS = 0.09f;
    constexpr float INSTRUMENTALNESS = 0.07f;
    constexpr float LIVENESS = 0.07f;
    constexpr float SPEECHINESS = 0.07f;
    constexpr float TEMPO = 0.12f;
    constexpr float POPULARITY = 0.13f;
}

static float normalizeTempo(float tempo) {
    return std::min(1.0f, std::max(0.0f, tempo / 200.0f));
}

static float normalizePopularity(int popularity) {
    return std::min(1.0f, std::max(0.0f, popularity / 100.0f));
}

struct FeatureVector {
    float danceability = 0.0f;
    float energy = 0.0f;
    float valence = 0.0f;
    float acousticness = 0.0f;
    float instrumentalness = 0.0f;
    float liveness = 0.0f;
    float speechiness = 0.0f;
    float tempo = 0.0f;
    float popularity = 0.0f;
};

static std::string trackIdentity(const Track& track) {
    if (!track.id.empty()) {
        return track.id;
    }
    return track.name + "|" + track.artist;
}

static FeatureVector toVector(const Track& t) {
    return FeatureVector{
        t.danceability,
        t.energy,
        t.valence,
        t.acousticness,
        t.instrumentalness,
        t.liveness,
        t.speechiness,
        normalizeTempo(t.tempo),
        normalizePopularity(t.popularity),
    };
}

static FeatureVector computeCentroid(const std::vector<Track>& likedTracks) {
    FeatureVector centroid;
    if (likedTracks.empty()) {
        return centroid;
    }
    for (const auto& track : likedTracks) {
        FeatureVector v = toVector(track);
        centroid.danceability += v.danceability;
        centroid.energy += v.energy;
        centroid.valence += v.valence;
        centroid.acousticness += v.acousticness;
        centroid.instrumentalness += v.instrumentalness;
        centroid.liveness += v.liveness;
        centroid.speechiness += v.speechiness;
        centroid.tempo += v.tempo;
        centroid.popularity += v.popularity;
    }
    const float inv = 1.0f / static_cast<float>(likedTracks.size());
    centroid.danceability *= inv;
    centroid.energy *= inv;
    centroid.valence *= inv;
    centroid.acousticness *= inv;
    centroid.instrumentalness *= inv;
    centroid.liveness *= inv;
    centroid.speechiness *= inv;
    centroid.tempo *= inv;
    centroid.popularity *= inv;
    return centroid;
}

static float weightedDistance(const FeatureVector& a, const FeatureVector& b) {
    using namespace ContentBasedWeights;

    return
        DANCEABILITY * std::abs(a.danceability - b.danceability) +
        ENERGY * std::abs(a.energy - b.energy) +
        VALENCE * std::abs(a.valence - b.valence) +
        ACOUSTICNESS * std::abs(a.acousticness - b.acousticness) +
        INSTRUMENTALNESS * std::abs(a.instrumentalness - b.instrumentalness) +
        LIVENESS * std::abs(a.liveness - b.liveness) +
        SPEECHINESS * std::abs(a.speechiness - b.speechiness) +
        TEMPO * std::abs(a.tempo - b.tempo) +
        POPULARITY * std::abs(a.popularity - b.popularity);
}

static std::string generateContentExplanation(const Track& t, const FeatureVector& centroid) {
    std::stringstream ss;

    const float energyDiff = std::abs(t.energy - centroid.energy);
    const float danceDiff = std::abs(t.danceability - centroid.danceability);
    const float valenceDiff = std::abs(t.valence - centroid.valence);

    if (energyDiff < 0.08f && danceDiff < 0.08f) {
        ss << "Strong content match to your liked tracks";
    } else if (energyDiff < 0.1f) {
        ss << "Very close to your preferred energy profile";
    } else if (danceDiff < 0.1f) {
        ss << "Very close to your danceability preferences";
    } else if (valenceDiff < 0.1f) {
        ss << "Similar mood profile to your liked songs";
    } else {
        ss << "Content-similar to your selected songs";
    }

    return ss.str();
}

std::vector<Recommendation> ContentBased::recommend(
    const std::vector<Track>& tracks,
    const User& user,
    size_t limit
) {
    std::vector<std::pair<Track, float>> scored;
    const FeatureVector centroid = computeCentroid(user.likedTracks);
    std::unordered_set<std::string> likedIds;
    likedIds.reserve(user.likedTracks.size() * 2);
    for (const auto& liked : user.likedTracks) {
        likedIds.insert(trackIdentity(liked));
    }

    for (const auto& t : tracks) {
        if (likedIds.count(trackIdentity(t))) {
            continue;
        }
        const float distance = weightedDistance(toVector(t), centroid);
        const float score = 1.0f / (1.0f + distance);
        scored.emplace_back(t, score);
    }

    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        }
    );

    std::vector<Recommendation> result;
    limit = std::min(limit, scored.size());
    
    for (size_t i = 0; i < limit; i++) {
        result.emplace_back(Recommendation{
            scored[i].first,
            scored[i].second,
            generateContentExplanation(scored[i].first, centroid)
        });
    }
    
    return result;
}