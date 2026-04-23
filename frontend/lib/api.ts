const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || "/api"

export interface Track {
  track: string
  artist: string
  score: number
  reason: string
  popularity: number
  energy: number
  danceability: number
}

export type RecommendationStrategy = "content" | "user" | "hybrid"

export interface RecommendationPreferences {
  energyMin: number
  energyMax: number
  danceabilityMin: number
  danceabilityMax: number
  popularityMin?: number
}

export interface RecommendationRequest {
  strategy: RecommendationStrategy
  likedQueries: string[]
  preferences: RecommendationPreferences
  limit?: number
}

export async function getRecommendations(
  request: RecommendationRequest
): Promise<Track[]> {
  const endpoint = `${API_BASE_URL}/recommend`

  const response = await fetch(endpoint, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      strategy: request.strategy,
      likedQueries: request.likedQueries,
      energyMin: request.preferences.energyMin,
      energyMax: request.preferences.energyMax,
      danceabilityMin: request.preferences.danceabilityMin,
      danceabilityMax: request.preferences.danceabilityMax,
      popularityMin: request.preferences.popularityMin ?? 0,
      limit: request.limit ?? 5,
    }),
  })

  if (!response.ok) {
    const errorText = await response.text().catch(() => "Unknown error")
    throw new Error(`HTTP ${response.status}: ${response.statusText} - ${errorText}`)
  }

  return response.json()
}

export async function healthCheck(): Promise<boolean> {
  try {
    const response = await fetch(`${API_BASE_URL}/health`)
    return response.ok
  } catch {
    return false
  }
}
