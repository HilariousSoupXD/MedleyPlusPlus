#pragma once
#include <string>

struct Track {
    // Identity
    std::string id;
    std::string name;
    std::string artist;

    // Core audio features
    float danceability;
    float energy;
    float loudness;
    float speechiness;
    float acousticness;
    float instrumentalness;
    float liveness;
    float valence;

    // Tempo & rhythm
    float tempo;
    int key;
    int mode;

    // Metadata
    int popularity;
    int duration_ms;
};