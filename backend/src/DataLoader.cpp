#include "../include/DataLoader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>

std::vector<Track> DataLoader::loadTracks(const std::string& filename) {
    std::vector<Track> tracks;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open dataset");
    }

    std::string line;
    getline(file, line); // header

    std::unordered_map<std::string, int> colIndex;
    std::stringstream headerStream(line);
    std::string col;
    int index = 0;

    while (getline(headerStream, col, ',')) {
        colIndex[col] = index++;
    }

    while (getline(file, line)) {
        std::stringstream ss(line);
        std::vector<std::string> tokens;
        std::string token;

        while (getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        Track t;

        auto get = [&](const std::string& name) -> std::string {
            return tokens[colIndex[name]];
        };

        try {
            t.name = get("track_name");
            t.artist = get("artist_name");

            t.danceability = std::stof(get("danceability"));
            t.energy = std::stof(get("energy"));
            t.loudness = std::stof(get("loudness"));
            t.speechiness = std::stof(get("speechiness"));
            t.acousticness = std::stof(get("acousticness"));
            t.instrumentalness = std::stof(get("instrumentalness"));
            t.liveness = std::stof(get("liveness"));
            t.valence = std::stof(get("valence"));

            t.tempo = std::stof(get("tempo"));
            t.key = std::stoi(get("key"));
            t.mode = std::stoi(get("mode"));

            t.popularity = std::stoi(get("popularity"));
            t.duration_ms = std::stoi(get("duration_ms"));

            tracks.push_back(t);
        } catch (...) {
            continue;
        }
    }

    return tracks;
}