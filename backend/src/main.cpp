#include "../libs/Crow/include/crow.h"
#include "../include/DataLoader.h"
#include "../include/UserProfiler.h"
#include "../include/Recommendation.h"
#include "../include/ContentBased.h"
#include "../include/UserBased.h"
#include "../include/Hybrid.h"
#include "../include/InteractionStore.h"
#include <memory>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool hasField(const crow::json::rvalue& body, const std::string& field) {
    return body.has(field);
}

bool isNumeric(const crow::json::rvalue& value) {
    return value.t() == crow::json::type::Number;
}

bool parseBoundedFloat(
    const crow::json::rvalue& body,
    const std::string& field,
    float minAllowed,
    float maxAllowed,
    float& outValue
) {
    if (!hasField(body, field)) {
        return true;
    }
    if (!isNumeric(body[field])) {
        return false;
    }
    outValue = static_cast<float>(body[field].d());
    if (outValue < minAllowed || outValue > maxAllowed) {
        return false;
    }
    return true;
}

std::vector<std::string> parseLikedQueries(const crow::json::rvalue& body) {
    std::vector<std::string> likedQueries;
    if (!hasField(body, "likedQueries")) {
        return likedQueries;
    }
    const auto& likedValue = body["likedQueries"];
    if (likedValue.t() != crow::json::type::List) {
        return likedQueries;
    }

    for (const auto& item : likedValue) {
        if (item.t() == crow::json::type::String) {
            std::string query = item.s();
            if (!query.empty()) {
                likedQueries.push_back(query);
            }
        }
    }
    return likedQueries;
}

std::vector<Track> resolveLikedTracks(
    const std::vector<Track>& tracks,
    const std::vector<std::string>& likedQueries
) {
    std::vector<Track> matched;
    std::unordered_set<std::string> seenTrackIds;

    for (const auto& rawQuery : likedQueries) {
        std::string query = toLower(rawQuery);
        for (const auto& track : tracks) {
            std::string haystack = toLower(track.name + " " + track.artist);
            if (haystack.find(query) != std::string::npos && seenTrackIds.insert(track.id).second) {
                matched.push_back(track);
            }
        }
    }

    return matched;
}

bool passesPreferenceFilters(
    const Recommendation& recommendation,
    float energyMin,
    float energyMax,
    float danceabilityMin,
    float danceabilityMax,
    int popularityMin
) {
    const Track& track = recommendation.track;
    return track.energy >= energyMin &&
           track.energy <= energyMax &&
           track.danceability >= danceabilityMin &&
           track.danceability <= danceabilityMax &&
           track.popularity >= popularityMin;
}

std::string recommendationKey(const Recommendation& recommendation) {
    return toLower(recommendation.track.name + "|" + recommendation.track.artist);
}
} // namespace

int main() {
    crow::SimpleApp app;

    // Find dataset - try multiple locations
    std::string datasetPath = "./data/dataset.csv";
    if (!std::filesystem::exists(datasetPath)) {
        datasetPath = "../data/dataset.csv";
    }
    if (!std::filesystem::exists(datasetPath)) {
        datasetPath = "/home/hsxd/Documents/CODE/Medley++/backend/data/dataset.csv";
    }

    std::cout << "Looking for dataset at: " << datasetPath << std::endl;

    // Load dataset ONCE (important)
    auto tracks = DataLoader::loadTracks(datasetPath);
    std::cout << "Loaded " << tracks.size() << " tracks" << std::endl;
    InteractionStore interactionStore;
    interactionStore.buildFromTracks(tracks);
    std::cout << "Initialized " << interactionStore.users().size() << " dummy users for collaborative filtering" << std::endl;

    // Health check endpoint
    CROW_ROUTE(app, "/health")
    ([](){
        auto res = crow::response(200, "OK");
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    // OPTIONS endpoint for CORS preflight
    CROW_ROUTE(app, "/recommend").methods("OPTIONS"_method)
    ([](){
        auto res = crow::response(200);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    CROW_ROUTE(app, "/recommend").methods("POST"_method)
    ([&tracks, &interactionStore](const crow::request& req){

        auto body = crow::json::load(req.body);

        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        if (!hasField(body, "strategy") || body["strategy"].t() != crow::json::type::String) {
            return crow::response(400, "Missing or invalid strategy");
        }
        std::string strategy = body["strategy"].s();

        float energyMin = 0.0f;
        float energyMax = 1.0f;
        float danceabilityMin = 0.0f;
        float danceabilityMax = 1.0f;
        int popularityMin = 0;
        int limit = 5;

        if (!parseBoundedFloat(body, "energyMin", 0.0f, 1.0f, energyMin) ||
            !parseBoundedFloat(body, "energyMax", 0.0f, 1.0f, energyMax) ||
            !parseBoundedFloat(body, "danceabilityMin", 0.0f, 1.0f, danceabilityMin) ||
            !parseBoundedFloat(body, "danceabilityMax", 0.0f, 1.0f, danceabilityMax)) {
            return crow::response(400, "Invalid preference bounds. Expected numeric values in [0, 1].");
        }

        if (energyMin > energyMax || danceabilityMin > danceabilityMax) {
            return crow::response(400, "Minimum preference bounds cannot be greater than maximum bounds.");
        }

        if (hasField(body, "popularityMin")) {
            if (!isNumeric(body["popularityMin"])) {
                return crow::response(400, "popularityMin must be a number in [0, 100].");
            }
            popularityMin = static_cast<int>(body["popularityMin"].d());
            if (popularityMin < 0 || popularityMin > 100) {
                return crow::response(400, "popularityMin must be in [0, 100].");
            }
        }

        if (hasField(body, "limit")) {
            if (!isNumeric(body["limit"])) {
                return crow::response(400, "limit must be a number in [1, 20].");
            }
            limit = static_cast<int>(body["limit"].d());
            if (limit < 1 || limit > 20) {
                return crow::response(400, "limit must be in [1, 20].");
            }
        }

        std::vector<std::string> likedQueries = parseLikedQueries(body);
        if (likedQueries.empty()) {
            return crow::response(400, "likedQueries must contain at least one track or artist query.");
        }

        std::vector<Track> likedTracks = resolveLikedTracks(tracks, likedQueries);
        if (likedTracks.empty()) {
            return crow::response(400, "No tracks matched likedQueries. Try broader track or artist names.");
        }

        User user;
        user.likedTracks = likedTracks;
        
        // Build user profile
        UserProfiler::buildProfile(user);

        // Select and execute strategy
        std::unique_ptr<Recommender> recommender;
        std::vector<Recommendation> recommendations;
        const size_t requestedLimit = static_cast<size_t>(limit);
        const size_t candidateLimit = std::min(
            std::max<size_t>(requestedLimit * 20, 100),
            tracks.size()
        );

        if (strategy == "content") {
            recommender = std::make_unique<ContentBased>();
            recommendations = recommender->recommend(tracks, user, candidateLimit);
        } 
        else if (strategy == "user") {
            recommender = std::make_unique<UserBased>(&interactionStore);
            recommendations = recommender->recommend(tracks, user, candidateLimit);
        }
        else if (strategy == "hybrid") {
            recommender = std::make_unique<Hybrid>();
            recommendations = recommender->recommend(tracks, user, candidateLimit);
        }
        else {
            return crow::response(400, "Unknown strategy: " + strategy);
        }

        std::vector<Recommendation> filtered;
        std::unordered_set<std::string> seenRecommendationKeys;
        for (const auto& recommendation : recommendations) {
            if (passesPreferenceFilters(
                recommendation,
                energyMin,
                energyMax,
                danceabilityMin,
                danceabilityMax,
                popularityMin
            ) && seenRecommendationKeys.insert(recommendationKey(recommendation)).second) {
                filtered.push_back(recommendation);
            }
        }

        if (filtered.empty()) {
            return crow::response(400, "No recommendations matched the selected preference filters.");
        }

        // Build response with explainability
        crow::json::wvalue response;

        size_t responseLimit = std::min(requestedLimit, filtered.size());
        for (size_t i = 0; i < responseLimit; i++) {
            response[i]["track"] = filtered[i].track.name;
            response[i]["artist"] = filtered[i].track.artist;
            response[i]["score"] = filtered[i].score;
            response[i]["reason"] = filtered[i].reason;
            response[i]["popularity"] = filtered[i].track.popularity;
            response[i]["energy"] = filtered[i].track.energy;
            response[i]["danceability"] = filtered[i].track.danceability;
            response[i]["instrumentalness"] = filtered[i].track.instrumentalness;
            response[i]["loudness"] = filtered[i].track.loudness;
            response[i]["acousticness"] = filtered[i].track.acousticness;
        }

        auto res = crow::response(response);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    std::cout << "Starting Medley++ backend on http://localhost:8080" << std::endl;
    app.port(8080).multithreaded().run();
}
