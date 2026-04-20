#pragma once
#include "Track.h"
#include <string>

struct Recommendation {
    Track track;
    float score;
    std::string reason;
};
