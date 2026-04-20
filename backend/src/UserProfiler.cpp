#include "../include/UserProfiler.h"
#include <cmath>

void UserProfiler::buildProfile(User& user) {
    int n = user.likedTracks.size();
    if (n == 0) return;

    user.total_likes = n;

    // Mean
    for (const auto& t : user.likedTracks) {
        user.avg_danceability += t.danceability;
        user.avg_energy += t.energy;
        user.avg_loudness += t.loudness;
        user.avg_speechiness += t.speechiness;
        user.avg_acousticness += t.acousticness;
        user.avg_instrumentalness += t.instrumentalness;
        user.avg_liveness += t.liveness;
        user.avg_valence += t.valence;
        user.avg_tempo += t.tempo;
    }

    user.avg_danceability /= n;
    user.avg_energy /= n;
    user.avg_loudness /= n;
    user.avg_speechiness /= n;
    user.avg_acousticness /= n;
    user.avg_instrumentalness /= n;
    user.avg_liveness /= n;
    user.avg_valence /= n;
    user.avg_tempo /= n;

    // Variance
    for (const auto& t : user.likedTracks) {
        user.var_energy += pow(t.energy - user.avg_energy, 2);
        user.var_danceability += pow(t.danceability - user.avg_danceability, 2);
        user.var_valence += pow(t.valence - user.avg_valence, 2);
    }

    user.var_energy /= n;
    user.var_danceability /= n;
    user.var_valence /= n;
}