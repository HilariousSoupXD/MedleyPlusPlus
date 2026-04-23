#include "../include/InteractionStore.h"
#include <algorithm>
#include <array>
#include <sstream>

namespace {
std::string trackIdentity(const Track& track) {
    if (!track.id.empty()) {
        return track.id;
    }
    return track.name + "|" + track.artist;
}

struct BucketTrack {
    std::string id;
    int popularity;
    float danceability;
};

size_t toBucketIndex(const Track& track) {
    const size_t energyBucket = track.energy >= 0.67f ? 2 : (track.energy >= 0.34f ? 1 : 0);
    const size_t danceBucket = track.danceability >= 0.5f ? 1 : 0;
    const size_t valenceBucket = track.valence >= 0.5f ? 1 : 0;
    return energyBucket * 4 + danceBucket * 2 + valenceBucket;
}
} // namespace

void InteractionStore::buildFromTracks(const std::vector<Track>& tracks) {
    users_.clear();
    if (tracks.empty()) {
        return;
    }

    constexpr size_t kBucketCount = 12;
    std::array<std::vector<BucketTrack>, kBucketCount> buckets;

    for (const auto& track : tracks) {
        const std::string identity = trackIdentity(track);
        if (identity.empty()) {
            continue;
        }
        buckets[toBucketIndex(track)].push_back(BucketTrack{
            identity,
            track.popularity,
            track.danceability,
        });
    }

    size_t userCounter = 0;
    for (size_t bucketIdx = 0; bucketIdx < buckets.size(); ++bucketIdx) {
        auto& bucketTracks = buckets[bucketIdx];
        if (bucketTracks.size() < 10) {
            continue;
        }

        std::sort(bucketTracks.begin(), bucketTracks.end(), [](const BucketTrack& a, const BucketTrack& b) {
            if (a.popularity != b.popularity) {
                return a.popularity > b.popularity;
            }
            return a.danceability > b.danceability;
        });

        // Create two users per bucket with different strides to diversify neighborhoods.
        for (int variant = 0; variant < 2; ++variant) {
            DummyUserInteractions dummy;
            std::stringstream idBuilder;
            idBuilder << "dummy_" << bucketIdx << "_" << variant;
            dummy.id = idBuilder.str();

            const size_t stride = static_cast<size_t>(variant + 2); // 2 then 3
            const size_t offset = static_cast<size_t>(variant);
            for (size_t i = offset; i < bucketTracks.size() && dummy.likedTrackIds.size() < 80; i += stride) {
                dummy.likedTrackIds.insert(bucketTracks[i].id);
            }

            if (dummy.likedTrackIds.size() >= 12) {
                users_.push_back(std::move(dummy));
                ++userCounter;
            }
        }
    }

    // Fallback: ensure a minimum set of dummy users even for skewed datasets.
    if (users_.size() < 6) {
        std::vector<std::string> identities;
        identities.reserve(tracks.size());
        for (const auto& track : tracks) {
            std::string identity = trackIdentity(track);
            if (!identity.empty()) {
                identities.push_back(std::move(identity));
            }
        }

        const size_t fallbackCount = 6;
        for (size_t u = users_.size(); u < fallbackCount; ++u) {
            DummyUserInteractions dummy;
            std::stringstream idBuilder;
            idBuilder << "dummy_fallback_" << u;
            dummy.id = idBuilder.str();

            const size_t stride = 5 + (u % 4);
            const size_t offset = u % stride;
            for (size_t i = offset; i < identities.size() && dummy.likedTrackIds.size() < 70; i += stride) {
                dummy.likedTrackIds.insert(identities[i]);
            }

            if (!dummy.likedTrackIds.empty()) {
                users_.push_back(std::move(dummy));
            }
        }
    }
}
