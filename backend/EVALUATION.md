# Offline recommendation evaluation

The evaluation executable calls the same C++ content-based, user-based, and
hybrid recommenders used by the backend. It also compares a popularity-ranked
baseline. Since the catalog has no user interaction history, this is a
**synthetic evaluation**, not evidence of real-world recommendation quality.

## Run

From the repository root:

```bash
cmake -S backend -B backend/build -DCMAKE_BUILD_TYPE=Release
cmake --build backend/build --target evaluate -j2
./backend/build/bin/evaluate backend/data/dataset.csv 3
```

The final argument is the number of timed requests per profile (default: 3).
The executable prints a Markdown results table.

## Synthetic test design

- Duplicate track IDs are removed, leaving 89,152 unique catalog tracks in the
  run below.
- Tracks are divided into 12 cohorts using the same energy, danceability, and
  valence thresholds as the dummy interaction generator.
- For each cohort, a fixed-seed shuffle selects 10 tracks as the profile's
  training likes and 20 other tracks as held-out positives. All other catalog
  tracks are candidates but are not labeled relevant.
- Ten synthetic peer users share the profile's 10 likes. Each peer also likes
  half of the 20 held-out positives, so every held-out positive occurs in five
  peer histories. This provides explicit interactions for the user-based
  recommender without exposing held-out tracks as the test profile's likes.
- Precision, recall, and binary NDCG are averaged over the 12 profiles.
  Latency is measured around one recommendation call, including recommender
  construction and ranking. Average and nearest-rank p95 use 36 sequential
  calls per strategy. Throughput is the reciprocal of average latency; it is
  single-process recommendation-call throughput, not HTTP/server throughput.
  Dataset loading and profile construction are excluded.

## Measured results

Run on the included catalog with a Release CMake build on Linux:

| Strategy | Precision@10 | Recall@10 | NDCG@10 | Avg latency (ms) | p95 latency (ms) | Throughput (req/s) |
|---|---:|---:|---:|---:|---:|---:|
| Popularity | 0.0000 | 0.0000 | 0.0000 | 18.889 | 19.869 | 52.9 |
| Content-based | 0.0000 | 0.0000 | 0.0000 | 46.560 | 48.349 | 21.5 |
| User-based | 1.0000 | 0.5000 | 1.0000 | 10.418 | 11.318 | 96.0 |
| Hybrid | 0.0000 | 0.0000 | 0.0000 | 41.136 | 44.321 | 24.3 |

This synthetic setup favors user-based filtering by construction: the peers
share the seed likes and contain all held-out positives. The high user-based
scores therefore verify that the implementation can retrieve known peer
interactions, but they do **not** establish that it will outperform other
strategies for real listeners. Likewise, the zero scores for the other
strategies describe this synthetic relevance definition only. A held-out
interaction dataset is needed for a meaningful real-world comparison.
