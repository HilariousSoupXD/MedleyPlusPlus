"use client"

import { useEffect, useMemo, useRef, useState } from "react"
import { Music, Heart, X } from "lucide-react"

interface OnboardingData {
  type: "onboarding"
  songName: string
}

interface RecommendationData {
  type: "recommendation"
  trackName: string
  score: number
  reason: string
  analytics: {
    energy: number
    danceability: number
    instrumentalness: number
    loudness: number
    acousticness: number
    popularity: number
  }
}

type CardData = OnboardingData | RecommendationData

interface SwipeCardProps {
  data: CardData
  onSwipeLeft: () => void
  onSwipeRight: () => void
  isAnimating: boolean
  swipeDirection: "left" | "right" | null
}

export function SwipeCard({
  data,
  onSwipeLeft,
  onSwipeRight,
  isAnimating,
  swipeDirection,
}: SwipeCardProps) {
  const [isFlipped, setIsFlipped] = useState(false)
  const [dragX, setDragX] = useState(0)
  const isDraggingRef = useRef(false)
  const startXRef = useRef(0)
  const dragXRef = useRef(0)
  const rafRef = useRef<number | null>(null)
  const pointerIdRef = useRef<number | null>(null)

  const supportsHover = useMemo(() => {
    if (typeof window === "undefined") return false
    return window.matchMedia?.("(hover: hover)").matches ?? false
  }, [])

  useEffect(() => {
    return () => {
      if (rafRef.current) {
        cancelAnimationFrame(rafRef.current)
        rafRef.current = null
      }
    }
  }, [])

  const updateDragX = (value: number) => {
    dragXRef.current = value
    if (rafRef.current) return
    rafRef.current = requestAnimationFrame(() => {
      rafRef.current = null
      setDragX(dragXRef.current)
    })
  }

  const handlePointerDown = (e: React.PointerEvent<HTMLDivElement>) => {
    pointerIdRef.current = e.pointerId
    e.currentTarget.setPointerCapture(e.pointerId)
    isDraggingRef.current = true
    startXRef.current = e.clientX
    updateDragX(0)
  }

  const handlePointerMove = (e: React.PointerEvent<HTMLDivElement>) => {
    if (!isDraggingRef.current) return
    if (pointerIdRef.current !== e.pointerId) return
    const diff = e.clientX - startXRef.current
    updateDragX(diff)
  }

  const handlePointerUp = (e: React.PointerEvent<HTMLDivElement>) => {
    if (pointerIdRef.current !== e.pointerId) return
    isDraggingRef.current = false
    pointerIdRef.current = null

    const current = dragXRef.current
    if (current > 100) {
      onSwipeRight()
    } else if (current < -100) {
      onSwipeLeft()
    }
    updateDragX(0)
  }

  const getTransform = () => {
    if (isAnimating && swipeDirection) {
      return swipeDirection === "right"
        ? "translateX(150%) rotate(20deg)"
        : "translateX(-150%) rotate(-20deg)"
    }
    if (isDraggingRef.current) {
      const rotation = dragX / 20
      return `translateX(${dragX}px) rotate(${rotation}deg)`
    }
    return "translateX(0) rotate(0)"
  }

  const getOpacity = () => {
    if (isAnimating) return 0
    return 1
  }

  const isRecommendation = data.type === "recommendation"
  const isOnboarding = data.type === "onboarding"

  const onboardingTags = isOnboarding
    ? (() => {
        const name = data.songName.toLowerCase()
        if (name.includes("weeknd")) {
          return ["Synth-pop", "R&B"]
        }
        if (name.includes("ed sheeran")) {
          return ["Pop", "Singer-songwriter"]
        }
        if (name.includes("queen")) {
          return ["Rock", "Classic"]
        }
        if (name.includes("billie eilish")) {
          return ["Alt-pop", "Electropop"]
        }
        if (name.includes("bruno mars")) {
          return ["Funk-pop", "Dance-pop"]
        }
        if (name.includes("eminem")) {
          return ["Hip-hop", "Rap"]
        }
        if (name.includes("adele")) {
          return ["Soul-pop", "Ballad"]
        }
        if (name.includes("eagles")) {
          return ["Classic rock", "Soft rock"]
        }
        if (name.includes("nirvana")) {
          return ["Grunge", "Alternative rock"]
        }
        if (name.includes("guns n'")) {
          return ["Hard rock", "Classic rock"]
        }
        return ["Pop", "Alternative"]
      })()
    : []

  return (
    <div
      className="relative w-full max-w-sm mx-auto"
      style={{ perspective: "1000px" }}
    >
      <div
        className="relative w-full cursor-grab active:cursor-grabbing"
        style={{
          transform: getTransform(),
          opacity: getOpacity(),
          transition: isAnimating || !isDraggingRef.current ? "transform 0.25s ease-out, opacity 0.25s ease-out" : "none",
        }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onPointerCancel={handlePointerUp}
      >
        {/* Swipe indicators */}
        <div
          className="absolute -left-4 top-1/2 -translate-y-1/2 z-10 transition-opacity duration-200 pointer-events-none"
          style={{ opacity: Math.max(0, -dragX / 100) }}
        >
          <div className="bg-destructive rounded-full p-3">
            <X className="w-6 h-6 text-destructive-foreground" />
          </div>
        </div>
        <div
          className="absolute -right-4 top-1/2 -translate-y-1/2 z-10 transition-opacity duration-200 pointer-events-none"
          style={{ opacity: Math.max(0, dragX / 100) }}
        >
          <div className="bg-primary rounded-full p-3">
            <Heart className="w-6 h-6 text-primary-foreground" />
          </div>
        </div>

        {/* Card container with flip effect */}
        <div
          className="relative w-full h-[420px]"
          style={{
            transformStyle: "preserve-3d",
            transform: isRecommendation && isFlipped ? "rotateY(180deg)" : "rotateY(0)",
            transition: "transform 0.6s cubic-bezier(0.4, 0, 0.2, 1)",
          }}
          onMouseEnter={() => supportsHover && isRecommendation && setIsFlipped(true)}
          onMouseLeave={() => setIsFlipped(false)}
        >
          {/* Front of card */}
          <div
            className="absolute inset-0 w-full h-full bg-card rounded-2xl p-8 border border-border shadow-2xl"
            style={{
              backfaceVisibility: "hidden",
              WebkitBackfaceVisibility: "hidden",
            }}
          >
            <div className="flex flex-col items-center gap-6 h-full justify-center">
              <div className="w-full max-w-[260px] aspect-square rounded-2xl border border-border bg-secondary/40 overflow-hidden shadow-inner">
                <div className="h-full w-full flex items-center justify-center bg-gradient-to-br from-primary/20 via-secondary/30 to-secondary">
                  <Music className="w-20 h-20 text-primary" />
                </div>
              </div>

              <div className="text-center">
                <h2 className="text-2xl font-bold text-foreground mb-2 text-balance">
                  {data.type === "onboarding" ? data.songName : data.trackName}
                </h2>

                {isOnboarding && onboardingTags.length > 0 && (
                  <div className="mt-3 flex items-center justify-center gap-2 flex-wrap">
                    {onboardingTags.map((tag) => (
                      <span
                        key={tag}
                        className="rounded-full bg-secondary px-3 py-1 text-xs font-semibold text-muted-foreground"
                      >
                        {tag}
                      </span>
                    ))}
                  </div>
                )}

                {isRecommendation && (
                  <>
                    <div className="mt-4 w-full">
                      <div className="flex justify-between text-sm text-muted-foreground mb-1">
                        <span>Match Score</span>
                        <span>{Math.round(data.score * 100)}%</span>
                      </div>
                      <div className="w-full bg-secondary rounded-full h-2">
                        <div
                          className="bg-primary h-2 rounded-full"
                          style={{ width: `${data.score * 100}%` }}
                        />
                      </div>
                    </div>

                    <p className="mt-4 text-sm text-muted-foreground italic">
                      {`"${data.reason}"`}
                    </p>

                    <p className="mt-4 text-xs text-muted-foreground">
                      Hover to see analytics
                    </p>
                  </>
                )}
              </div>
            </div>
          </div>

          {/* Back of card (analytics) - only for recommendations */}
          {isRecommendation && (
            <div
              className="absolute inset-0 w-full h-full bg-card rounded-2xl p-8 border border-border shadow-2xl"
              style={{
                backfaceVisibility: "hidden",
                WebkitBackfaceVisibility: "hidden",
                transform: "rotateY(180deg)",
              }}
            >
              <div className="flex flex-col gap-4 h-full justify-center">
                <h3 className="text-lg font-bold text-foreground text-center mb-2">
                  Track Analytics
                </h3>

                {Object.entries(data.analytics).map(([key, value]) => (
                  <div key={key}>
                    <div className="flex justify-between text-sm text-muted-foreground mb-1">
                      <span className="capitalize">{key}</span>
                      <span>{Math.round(value * 100)}%</span>
                    </div>
                    <div className="w-full bg-secondary rounded-full h-2">
                      <div
                        className="bg-primary h-2 rounded-full"
                        style={{ width: `${value * 100}%` }}
                      />
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}
        </div>
      </div>

      {/* Action buttons + helper text */}
      <div className="mt-8 w-full flex flex-col items-center gap-4">
        <div className="grid grid-cols-2 gap-8">
          <div className="flex flex-col items-center gap-2">
            <button
              onClick={onSwipeLeft}
              className="w-16 h-16 rounded-full bg-secondary border-2 border-destructive flex items-center justify-center transition-all hover:scale-110 hover:bg-destructive/20"
              aria-label="Skip song"
            >
              <X className="w-8 h-8 text-destructive" />
            </button>
            <span className="text-xs text-muted-foreground font-medium">Skip</span>
          </div>
          <div className="flex flex-col items-center gap-2">
            <button
              onClick={onSwipeRight}
              className="w-16 h-16 rounded-full bg-secondary border-2 border-primary flex items-center justify-center transition-all hover:scale-110 hover:bg-primary/20"
              aria-label="Like song"
            >
              <Heart className="w-8 h-8 text-primary" />
            </button>
            <span className="text-xs text-muted-foreground font-medium">Like</span>
          </div>
        </div>
      </div>
    </div>
  )
}
