#pragma once

#include "Track.h"
#include <string>
#include <unordered_set>
#include <vector>

struct DummyUserInteractions {
    std::string id;
    std::unordered_set<std::string> likedTrackIds;
};

class InteractionStore {
public:
    void buildFromTracks(const std::vector<Track>& tracks);

    const std::vector<DummyUserInteractions>& users() const {
        return users_;
    }

private:
    std::vector<DummyUserInteractions> users_;
};
