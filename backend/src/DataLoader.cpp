#include "../include/DataLoader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <cctype>
#include <algorithm>

namespace {
std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            // Handle escaped quote ("")
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }

    fields.push_back(current);
    return fields;
}

bool containsAlpha(const std::string& value) {
    for (unsigned char c : value) {
        if (std::isalpha(c)) {
            return true;
        }
    }
    return false;
}

std::string trim(std::string value) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string stripTrailingDashNumber(std::string value) {
    value = trim(value);
    // Remove trailing " - <digits>" (common in some noisy exports).
    auto dashPos = value.rfind(" - ");
    if (dashPos == std::string::npos) {
        return value;
    }

    const std::string suffix = value.substr(dashPos + 3);
    if (suffix.empty()) {
        return value;
    }

    bool allDigits = true;
    for (unsigned char c : suffix) {
        if (!std::isdigit(c)) {
            allDigits = false;
            break;
        }
    }
    if (!allDigits) {
        return value;
    }

    return trim(value.substr(0, dashPos));
}
} // namespace

std::vector<Track> DataLoader::loadTracks(const std::string& filename) {
    std::vector<Track> tracks;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open dataset");
    }

    std::string line;
    getline(file, line); // header

    std::unordered_map<std::string, int> colIndex;
    auto headerFields = parseCsvLine(line);
    int index = 0;
    for (const auto& col : headerFields) {
        colIndex[col] = index++;
    }

    while (getline(file, line)) {
        std::vector<std::string> tokens = parseCsvLine(line);

        Track t;

        auto get = [&](const std::string& name) -> std::string {
            auto it = colIndex.find(name);
            if (it == colIndex.end()) {
                return "";
            }
            if (static_cast<size_t>(it->second) >= tokens.size()) {
                return "";
            }
            return tokens[colIndex[name]];
        };

        auto getFirstAvailable = [&](const std::vector<std::string>& names) -> std::string {
            for (const auto& name : names) {
                const auto value = get(name);
                if (!value.empty()) {
                    return value;
                }
            }
            return "";
        };

        try {
            t.id = getFirstAvailable({"track_id", "id"});
            t.name = stripTrailingDashNumber(getFirstAvailable({"track_name", "name"}));
            t.artist = stripTrailingDashNumber(getFirstAvailable({"artist_name", "artists", "artist"}));

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

            if (t.name.empty() || t.artist.empty() || !containsAlpha(t.artist)) {
                continue;
            }

            tracks.push_back(t);
        } catch (...) {
            continue;
        }
    }

    return tracks;
}