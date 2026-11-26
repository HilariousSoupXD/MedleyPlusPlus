#include "../include/ContentBased.h"
#include "../include/DataLoader.h"
#include "../include/Hybrid.h"
#include "../include/InteractionStore.h"
#include "../include/Recommendation.h"
#include "../include/UserBased.h"
#include "../include/UserProfiler.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
constexpr size_t kRecommendationLimit = 10;
constexpr size_t kLikedTrackCount = 10;
constexpr size_t kRelevantTrackCount = 20;
constexpr size_t kSyntheticNeighborCount = 10;
constexpr size_t kDefaultRepeats = 3;

struct EvaluationProfile {
    std::string name;
    User user;
    std::vector<Track> candidates;
    std::unordered_set<std::string> relevantIds;
    InteractionStore interactions;
};

struct Metrics {
    double precision = 0.0;
    double recall = 0.0;
    double ndcg = 0.0;
};

struct Timing {
    double averageMs = 0.0;
    double p95Ms = 0.0;
    double throughput = 0.0;
};

size_t bucketIndex(const Track& track) {
    const size_t energyBucket = track.energy >= 0.67f ? 2 : (track.energy >= 0.34f ? 1 : 0);
    const size_t danceBucket = track.danceability >= 0.5f ? 1 : 0;
    const size_t valenceBucket = track.valence >= 0.5f ? 1 : 0;
    return energyBucket * 4 + danceBucket * 2 + valenceBucket;
}

std::vector<EvaluationProfile> makeProfiles(const std::vector<Track>& tracks) {
    std::vector<Track> uniqueTracks;
    uniqueTracks.reserve(tracks.size());
    std::unordered_set<std::string> seenTrackIds;
    for (const auto& track : tracks) {
        const std::string identity = track.id.empty() ? track.name + "|" + track.artist : track.id;
        if (seenTrackIds.insert(identity).second) {
            uniqueTracks.push_back(track);
        }
    }

    std::vector<std::vector<Track>> buckets(12);
    for (const auto& track : uniqueTracks) {
        buckets[bucketIndex(track)].push_back(track);
    }

    std::vector<EvaluationProfile> profiles;
    for (size_t bucket = 0; bucket < buckets.size(); ++bucket) {
        auto& cohort = buckets[bucket];
        if (cohort.size() < kLikedTrackCount + kRelevantTrackCount) {
            continue;
        }
        std::sort(cohort.begin(), cohort.end(), [](const Track& left, const Track& right) {
            return left.id < right.id;
        });
        std::mt19937 generator(static_cast<unsigned int>(20261125 + bucket));
        std::shuffle(cohort.begin(), cohort.end(), generator);

        EvaluationProfile profile;
        profile.name = "audio_bucket_" + std::to_string(bucket);
        profile.user.likedTracks.insert(
            profile.user.likedTracks.end(), cohort.begin(), cohort.begin() + kLikedTrackCount
        );
        UserProfiler::buildProfile(profile.user);

        for (size_t index = 0; index < kRelevantTrackCount; ++index) {
            const Track& relevant = cohort[kLikedTrackCount + index];
            profile.relevantIds.insert(relevant.id);
        }

        profile.candidates.reserve(uniqueTracks.size() - kLikedTrackCount);
        for (const auto& track : uniqueTracks) {
            const bool isLiked = std::any_of(
                profile.user.likedTracks.begin(), profile.user.likedTracks.end(),
                [&track](const Track& liked) { return liked.id == track.id; }
            );
            if (!isLiked) {
                profile.candidates.push_back(track);
            }
        }

        for (size_t neighborIndex = 0; neighborIndex < kSyntheticNeighborCount; ++neighborIndex) {
            DummyUserInteractions neighbor;
            neighbor.id = profile.name + "_neighbor_" + std::to_string(neighborIndex);
            for (const auto& liked : profile.user.likedTracks) {
                neighbor.likedTrackIds.insert(liked.id);
            }
            for (size_t relevantIndex = 0; relevantIndex < kRelevantTrackCount; ++relevantIndex) {
                if ((relevantIndex + neighborIndex) % kSyntheticNeighborCount < kSyntheticNeighborCount / 2) {
                    neighbor.likedTrackIds.insert(cohort[kLikedTrackCount + relevantIndex].id);
                }
            }
            profile.interactions.addUser(std::move(neighbor));
        }

        profiles.push_back(std::move(profile));
    }

    if (profiles.empty()) {
        throw std::runtime_error("Dataset has no audio-feature cohort large enough for evaluation");
    }
    return profiles;
}

Metrics scoreRecommendations(
    const std::vector<Recommendation>& recommendations,
    const std::unordered_set<std::string>& relevantIds
) {
    size_t relevantRetrieved = 0;
    double discountedGain = 0.0;
    for (size_t rank = 0; rank < std::min(kRecommendationLimit, recommendations.size()); ++rank) {
        if (relevantIds.count(recommendations[rank].track.id)) {
            ++relevantRetrieved;
            discountedGain += 1.0 / std::log2(static_cast<double>(rank) + 2.0);
        }
    }

    double idealGain = 0.0;
    const size_t idealCount = std::min(kRecommendationLimit, relevantIds.size());
    for (size_t rank = 0; rank < idealCount; ++rank) {
        idealGain += 1.0 / std::log2(static_cast<double>(rank) + 2.0);
    }

    return Metrics{
        static_cast<double>(relevantRetrieved) / kRecommendationLimit,
        static_cast<double>(relevantRetrieved) / relevantIds.size(),
        idealGain == 0.0 ? 0.0 : discountedGain / idealGain,
    };
}

std::vector<Recommendation> popularityRecommendations(
    const std::vector<Track>& candidates,
    size_t limit
) {
    std::vector<const Track*> ranked;
    ranked.reserve(candidates.size());
    for (const auto& track : candidates) {
        ranked.push_back(&track);
    }
    std::sort(ranked.begin(), ranked.end(), [](const Track* left, const Track* right) {
        if (left->popularity != right->popularity) {
            return left->popularity > right->popularity;
        }
        return left->id < right->id;
    });

    std::vector<Recommendation> result;
    result.reserve(std::min(limit, ranked.size()));
    for (size_t index = 0; index < std::min(limit, ranked.size()); ++index) {
        result.push_back(Recommendation{*ranked[index], static_cast<float>(ranked[index]->popularity), "Popularity"});
    }
    return result;
}

Metrics evaluateStrategy(
    const std::string& strategyName,
    std::vector<EvaluationProfile>& profiles,
    size_t repeats
) {
    std::vector<double> latenciesMs;
    latenciesMs.reserve(profiles.size() * repeats);
    double precision = 0.0;
    double recall = 0.0;
    double ndcg = 0.0;

    for (auto& profile : profiles) {
        Metrics profileMetrics;
        for (size_t repeat = 0; repeat < repeats; ++repeat) {
            const auto requestStart = std::chrono::steady_clock::now();
            std::vector<Recommendation> recommendations;
            if (strategyName == "Popularity") {
                recommendations = popularityRecommendations(profile.candidates, kRecommendationLimit);
            } else if (strategyName == "Content-based") {
                ContentBased recommender;
                recommendations = recommender.recommend(profile.candidates, profile.user, kRecommendationLimit);
            } else if (strategyName == "User-based") {
                UserBased recommender(&profile.interactions);
                recommendations = recommender.recommend(profile.candidates, profile.user, kRecommendationLimit);
            } else {
                Hybrid recommender;
                recommendations = recommender.recommend(profile.candidates, profile.user, kRecommendationLimit);
            }
            const auto requestEnd = std::chrono::steady_clock::now();
            latenciesMs.push_back(
                std::chrono::duration<double, std::milli>(requestEnd - requestStart).count()
            );
            profileMetrics = scoreRecommendations(recommendations, profile.relevantIds);
        }
        precision += profileMetrics.precision;
        recall += profileMetrics.recall;
        ndcg += profileMetrics.ndcg;
    }

    std::sort(latenciesMs.begin(), latenciesMs.end());
    const size_t p95Index = static_cast<size_t>(
        std::ceil(0.95 * static_cast<double>(latenciesMs.size()))
    ) - 1;
    const double latencySum = std::accumulate(latenciesMs.begin(), latenciesMs.end(), 0.0);

    const Metrics averageMetrics{
        precision / profiles.size(),
        recall / profiles.size(),
        ndcg / profiles.size(),
    };
    const Timing timing{
        latencySum / latenciesMs.size(),
        latenciesMs[p95Index],
        latencySum == 0.0 ? 0.0 : 1000.0 * latenciesMs.size() / latencySum,
    };

    std::cout << "| " << strategyName << " | "
              << std::fixed << std::setprecision(4)
              << averageMetrics.precision << " | " << averageMetrics.recall << " | "
              << averageMetrics.ndcg << " | " << std::setprecision(3)
              << timing.averageMs << " | " << timing.p95Ms << " | "
              << std::setprecision(1) << timing.throughput << " |\n";
    return averageMetrics;
}
} // namespace

int main(int argc, char** argv) {
    const std::string datasetPath = argc > 1 ? argv[1] : "./data/dataset.csv";
    const size_t repeats = argc > 2 ? std::stoul(argv[2]) : kDefaultRepeats;
    if (repeats == 0) {
        std::cerr << "Repeat count must be greater than zero\n";
        return EXIT_FAILURE;
    }
    if (argc > 3) {
        std::cerr << "Usage: evaluate [dataset.csv] [repeats]\n";
        return EXIT_FAILURE;
    }

    const std::vector<Track> tracks = DataLoader::loadTracks(datasetPath);
    std::vector<EvaluationProfile> profiles = makeProfiles(tracks);

    std::cout << "Dataset: " << datasetPath << "\n"
              << "Tracks loaded: " << tracks.size() << "\n"
              << "Unique tracks evaluated: " << profiles.front().candidates.size() + kLikedTrackCount << "\n"
              << "Synthetic profiles: " << profiles.size() << "\n"
              << "Requests per strategy: " << profiles.size() * repeats << "\n\n"
              << "| Strategy | Precision@10 | Recall@10 | NDCG@10 | Avg latency (ms) | p95 latency (ms) | Throughput (req/s) |\n"
              << "|---|---:|---:|---:|---:|---:|---:|\n";

    evaluateStrategy("Popularity", profiles, repeats);
    evaluateStrategy("Content-based", profiles, repeats);
    evaluateStrategy("User-based", profiles, repeats);
    evaluateStrategy("Hybrid", profiles, repeats);

    std::cout << "\nSynthetic test setup: each profile likes 10 tracks from one audio-feature bucket; "
                 "20 other tracks from the same bucket are held out as positives and appear in "
                 "synthetic peer interactions.\n";
    return EXIT_SUCCESS;
}
