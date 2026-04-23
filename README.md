# Medley++ 🎵

A modern music recommendation system combining content-based, user-based, and hybrid recommendation strategies with a beautiful web interface.

## Architecture

**Backend**: C++ with Crow web framework
- RESTful API with recommendation engine
- Multiple recommendation strategies (Content, User-based, Hybrid)
- Explainable recommendations with reasoning

**Frontend**: Next.js + TypeScript + Tailwind CSS
- Beautiful card-based UI with flip animations
- Real-time recommendations from backend
- Responsive design with dark mode support

## Quick Start

See [SETUP.md](./SETUP.md) for detailed installation and running instructions.

### TL;DR
```bash
# Terminal 1 - Backend
cd backend
cmake -B build && cmake --build build
./app

# Terminal 2 - Frontend
cd frontend
pnpm install
pnpm dev
```

Then open http://localhost:3000

## Features

✨ **Multiple Recommendation Strategies**
- Content-Based: Recommends tracks similar to liked tracks
- User-Based: Recommends tracks liked by similar users
- Hybrid: Combines both approaches for better recommendations

🎨 **Beautiful UI**
- Interactive card interface
- Flip animation for track details
- Match score visualization
- Real-time energy & danceability metrics

🔄 **Seamless Integration**
- Frontend-Backend communication via REST API
- CORS enabled for cross-origin requests
- Environment-based configuration

## Project Structure

```
Medley++/
├── backend/
│   ├── src/           # C++ source code
│   ├── include/       # Header files
│   ├── data/          # Dataset
│   └── libs/          # Crow framework
├── frontend/
│   ├── app/           # Next.js pages
│   ├── components/    # UI components
│   ├── lib/           # Utilities (API client)
│   └── public/        # Static assets
├── SETUP.md           # Setup guide
└── README.md          # This file
```

## API Reference

### Health Check
```
GET /health
Response: 200 OK
```

### Get Recommendations
```
POST /recommend
Content-Type: application/json

{
  "strategy": "content" | "user" | "hybrid"
}

Response:
[
  {
    "track": "Song Name",
    "artist": "Artist Name",
    "score": 0.92,
    "reason": "Similar to your favorite tracks",
    "popularity": 85,
    "energy": 0.75,
    "danceability": 0.82
  },
  ...
]
```

## Technologies

### Backend
- C++17
- Crow (lightweight C++ web framework)
- CMake

### Frontend
- Next.js 16
- React 19
- TypeScript
- Tailwind CSS
- Radix UI components

## Development

The frontend automatically fetches recommendations from the backend. To modify API behavior, edit:
- Backend: `backend/src/main.cpp`
- Frontend API client: `frontend/lib/api.ts`

For UI changes: `frontend/app/page.tsx`

## Troubleshooting

**Backend won't compile**: Ensure you have C++ dev tools and CMake installed
**Frontend won't start**: Check that you have Node.js 18+ installed
**CORS errors**: Backend has CORS enabled - check `NEXT_PUBLIC_API_URL` in `.env.local`

See [SETUP.md](./SETUP.md) for more troubleshooting tips.

## License

MIT
