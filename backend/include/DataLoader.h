#pragma once
#include <vector>
#include <string>
#include "Track.h"

class DataLoader {
public:
    static std::vector<Track> loadTracks(const std::string& filename);
};