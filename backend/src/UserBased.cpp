#include "../include/UserBased.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace {
std::string trackIdentity(const Track& track) {
    if (!track.id.empty()) {
        return track.id;
    }
    return track.name + "|" + track.artist;
}

float jaccardSimilarity(
    const std::unordered_set<std::string>& a,
    const std::unordered_set<std::string>& b
) {
    if (a.empty() || b.empty()) {
        return 0.0f;
    }

    size_t intersection = 0;
    const auto& smaller = (a.size() < b.size()) ? a : b;
    const auto& larger = (a.size() < b.size()) ? b : a;
    for (const auto& item : smaller) {
        if (larger.count(item)) {
            ++intersection;
        }
    }
    const size_t uni = a.size() + b.size() - intersection;
    if (uni == 0) {
        return 0.0f;
    }
    return static_cast<float>(intersection) / static_cast<float>(uni);
}

std::string collaborativeReason(float strongestNeighborSimilarity, size_t neighborVotes) {
    std::stringstream ss;
    if (strongestNeighborSimilarity > 0.35f) {
        ss << "Liked by users very similar to your taste";
    } else if (strongestNeighborSimilarity > 0.2f) {
        ss << "Common among users with overlapping preferences";
    } else {
        ss << "Recommended from collaborative user patterns";
    }
    ss << " (" << neighborVotes << " supporting users)";
    return ss.str();
}

float fallbackProfileScore(const Track& t, const User& user) {
    float diff =
        std::abs(t.energy - user.avg_energy) +
        std::abs(t.danceability - user.avg_danceability) +
        std::abs(t.valence - user.avg_valence) +
        std::abs(t.acousticness - user.avg_acousticness);
    return 1.0f / (1.0f + diff);
}
} // namespace

std::vector<Recommendation> UserBased::recommend(
    const std::vector<Track>& tracks,
    const User& user,
    size_t limit
) {
    if (!interactionStore_ || interactionStore_->users().empty()) {
        return {};
    }

    std::unordered_set<std::string> activeLikedIds;
    activeLikedIds.reserve(user.likedTracks.size() * 2);
    for (const auto& liked : user.likedTracks) {
        activeLikedIds.insert(trackIdentity(liked));
    }
    if (activeLikedIds.empty()) {
        return {};
    }

    std::unordered_map<std::string, const Track*> trackByIdentity;
    trackByIdentity.reserve(tracks.size() * 2);
    for (const auto& track : tracks) {
        trackByIdentity[trackIdentity(track)] = &track;
    }

    struct Neighbor {
        const DummyUserInteractions* user;
        float similarity;
    };
    std::vector<Neighbor> neighbors;
    neighbors.reserve(interactionStore_->users().size());
    for (const auto& dummy : interactionStore_->users()) {
        float sim = jaccardSimilarity(activeLikedIds, dummy.likedTrackIds);
        if (sim > 0.0f) {
            neighbors.push_back(Neighbor{&dummy, sim});
        }
    }
    if (neighbors.empty()) {
        return {};
    }

    std::sort(neighbors.begin(), neighbors.end(), [](const Neighbor& a, const Neighbor& b) {
        return a.similarity > b.similarity;
    });
    const size_t kMaxNeighbors = std::min<size_t>(25, neighbors.size());

    struct CandidateScore {
        float weightedScore = 0.0f;
        float strongestNeighbor = 0.0f;
        size_t supportCount = 0;
    };
    std::unordered_map<std::string, CandidateScore> candidateScores;

    for (size_t i = 0; i < kMaxNeighbors; ++i) {
        const auto* dummy = neighbors[i].user;
        const float neighborSim = neighbors[i].similarity;
        for (const auto& candidateId : dummy->likedTrackIds) {
            if (activeLikedIds.count(candidateId)) {
                continue;
            }
            if (!trackByIdentity.count(candidateId)) {
                continue;
            }
            auto& agg = candidateScores[candidateId];
            agg.weightedScore += neighborSim;
            agg.strongestNeighbor = std::max(agg.strongestNeighbor, neighborSim);
            ++agg.supportCount;
        }
    }

    std::vector<std::pair<std::string, CandidateScore>> ranked;
    ranked.reserve(candidateScores.size());
    for (const auto& [trackId, score] : candidateScores) {
        ranked.emplace_back(trackId, score);
    }
    std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) {
        if (a.second.weightedScore != b.second.weightedScore) {
            return a.second.weightedScore > b.second.weightedScore;
        }
        return a.second.supportCount > b.second.supportCount;
    });

    std::vector<Recommendation> result;
    result.reserve(limit);
    std::unordered_set<std::string> selectedIds;

    if (!ranked.empty()) {
        const float maxScore = std::max(0.0001f, ranked.front().second.weightedScore);
        const size_t collaborativeCount = std::min(limit, ranked.size());
        for (size_t i = 0; i < collaborativeCount; ++i) {
            const auto& trackId = ranked[i].first;
            const auto& aggregate = ranked[i].second;
            const auto* track = trackByIdentity[trackId];
            const float normalized = std::min(1.0f, aggregate.weightedScore / maxScore);
            result.push_back(Recommendation{
                *track,
                normalized,
                collaborativeReason(aggregate.strongestNeighbor, aggregate.supportCount),
            });
            selectedIds.insert(trackId);
        }
    }

    if (result.size() < limit) {
        std::vector<std::pair<const Track*, float>> fallbackCandidates;
        fallbackCandidates.reserve(tracks.size());
        for (const auto& track : tracks) {
            const std::string id = trackIdentity(track);
            if (activeLikedIds.count(id) || selectedIds.count(id)) {
                continue;
            }
            fallbackCandidates.emplace_back(&track, fallbackProfileScore(track, user));
        }
        std::sort(fallbackCandidates.begin(), fallbackCandidates.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
        for (const auto& candidate : fallbackCandidates) {
            if (result.size() >= limit) {
                break;
            }
            result.push_back(Recommendation{
                *candidate.first,
                std::min(1.0f, candidate.second),
                "Collaborative pool was limited, using profile-similar fallback",
            });
        }
    }

    return result;
}