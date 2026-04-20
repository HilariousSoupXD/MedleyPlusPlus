#include "../libs/Crow/include/crow.h"
#include "../include/DataLoader.h"
#include "../include/UserProfiler.h"
#include "../include/Recommendation.h"
#include "../include/ContentBased.h"
#include "../include/UserBased.h"
#include "../include/Hybrid.h"
#include <memory>
#include <iostream>

int main() {
    crow::SimpleApp app;

    // Load dataset ONCE (important)
    auto tracks = DataLoader::loadTracks("./data/dataset.csv");
    std::cout << "Loaded " << tracks.size() << " tracks" << std::endl;

    CROW_ROUTE(app, "/recommend").methods("POST"_method)
    ([&tracks](const crow::request& req){

        auto body = crow::json::load(req.body);

        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string strategy = body["strategy"].s();
        
        // TODO: In production, would parse user liked tracks from request
        // For now, simulate a user with first 10 tracks as liked
        User user;
        if (tracks.size() > 10) {
            user.likedTracks.assign(tracks.begin(), tracks.begin() + 10);
        } else {
            user.likedTracks = tracks;
        }
        
        // Build user profile
        UserProfiler::buildProfile(user);

        // Select and execute strategy
        std::unique_ptr<Recommender> recommender;
        std::vector<Recommendation> recommendations;

        if (strategy == "content") {
            recommender = std::make_unique<ContentBased>();
            recommendations = recommender->recommend(tracks, user);
        } 
        else if (strategy == "user") {
            recommender = std::make_unique<UserBased>();
            recommendations = recommender->recommend(tracks, user);
        }
        else if (strategy == "hybrid") {
            recommender = std::make_unique<Hybrid>();
            recommendations = recommender->recommend(tracks, user);
        }
        else {
            return crow::response(400, "Unknown strategy: " + strategy);
        }

        // Build response with explainability
        crow::json::wvalue response;
        
        for (size_t i = 0; i < recommendations.size(); i++) {
            response[i]["track"] = recommendations[i].track.name;
            response[i]["artist"] = recommendations[i].track.artist;
            response[i]["score"] = recommendations[i].score;
            response[i]["reason"] = recommendations[i].reason;
            response[i]["popularity"] = recommendations[i].track.popularity;
            response[i]["energy"] = recommendations[i].track.energy;
            response[i]["danceability"] = recommendations[i].track.danceability;
        }

        return crow::response(response);
    });

    app.port(8080).multithreaded().run();
}
