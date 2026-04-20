#pragma once
#include <vector>
#include "Track.h"

struct User {
    // Interaction history
    std::vector<Track> likedTracks;

    // Feature profile (mean values)
    float avg_danceability = 0;
    float avg_energy = 0;
    float avg_loudness = 0;
    float avg_speechiness = 0;
    float avg_acousticness = 0;
    float avg_instrumentalness = 0;
    float avg_liveness = 0;
    float avg_valence = 0;
    float avg_tempo = 0;

    // Variance (important for understanding preference spread)
    float var_energy = 0;
    float var_danceability = 0;
    float var_valence = 0;

    // Behavioral metadata
    int total_likes = 0;
};