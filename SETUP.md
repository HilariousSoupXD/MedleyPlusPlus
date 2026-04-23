# Medley++ Setup Guide

## Backend Setup

The backend is a C++ application using the Crow web framework.

### Prerequisites
- C++ compiler (g++ or clang)
- CMake 3.15+

### Building & Running

```bash
cd backend
# Build the backend
cmake -B build
cmake --build build

# Run the backend (from backend directory)
./app
# OR if executable is in build folder
./build/app
```

The backend will start on `http://localhost:8080`

### API Endpoints

- **`GET /health`** - Check backend status
- **`POST /recommend`** - Get music recommendations
  - Request body: `{ "strategy": "content" | "user" | "hybrid" }`
  - Response: Array of recommended tracks with scores and reasons

## Frontend Setup

The frontend is a Next.js application.

### Prerequisites
- Node.js 18+
- pnpm (or npm/yarn)

### Installation & Running

```bash
cd frontend

# Install dependencies
pnpm install

# Run development server
pnpm dev
```

The frontend will start on `http://localhost:3000`

## Configuration

The frontend is configured to connect to the backend via the `NEXT_PUBLIC_API_URL` environment variable in `.env.local`:

```env
NEXT_PUBLIC_API_URL=http://localhost:8080
```

Update this if your backend is running on a different host/port.

## Running Both Together

1. Start the backend:
   ```bash
   cd backend
   cmake -B build && cmake --build build
   ./app
   ```

2. In another terminal, start the frontend:
   ```bash
   cd frontend
   pnpm install  # (only needed first time)
   pnpm dev
   ```

3. Open `http://localhost:3000` in your browser

## Features

- **Multiple Recommendation Strategies**: Content-based, User-based, and Hybrid recommendations
- **Real-time Backend Integration**: Frontend fetches recommendations from your C++ backend
- **Beautiful UI**: Card-based recommendation interface with flip animation for details
- **Error Handling**: Graceful fallback if backend is not available

## Troubleshooting

- **Backend connection failed**: Ensure the backend is running on port 8080
- **CORS errors**: The backend now has CORS enabled for all origins
- **Port conflicts**: You can change `NEXT_PUBLIC_API_URL` in `.env.local` to use a different port
