# Medley++ - Now Running! 🎵

## ✅ Status

Both services are successfully running:

- **Backend**: http://localhost:8080 (C++ with Crow framework)
  - Loaded 106,882 tracks
  - Running on 18 threads
  - Provides `/health` and `/recommend` endpoints

- **Frontend**: http://localhost:3000 (Next.js + React)
  - Connected to backend via REST API
  - Beautiful card-based UI with flip animations
  - Support for 3 recommendation strategies

## 🚀 Quick Start (Next Time)

### Terminal 1 - Backend
```bash
cd backend
g++ -std=c++17 -I./libs/Crow/include -I./include -pthread \
  src/main.cpp src/DataLoader.cpp src/UserProfiler.cpp \
  src/ContentBased.cpp src/UserBased.cpp src/Hybrid.cpp \
  -o app && ./app
```

### Terminal 2 - Frontend
```bash
cd frontend
npm run dev
```

Then open: **http://localhost:3000**

## 📋 Full Setup Instructions

See [SETUP.md](./SETUP.md) for detailed instructions and troubleshooting.

## 🎯 How It Works

1. **Frontend** (React/Next.js) sends recommendation requests to the backend with a strategy choice
2. **Backend** (C++) loads 106K+ tracks and applies the selected algorithm:
   - **Content-Based**: Recommends similar tracks based on audio features
   - **User-Based**: Uses collaborative filtering
   - **Hybrid**: Combines both approaches
3. **Result**: Beautiful UI displays recommendations with explanations and audio metrics

## 🔧 Compilation Notes

- **Backend**: Uses `g++` with C++17 support (no CMake needed, but optional)
- **Frontend**: Uses Next.js with Node.js
- **CORS**: Enabled on backend for cross-origin requests from frontend
- **Dataset**: 20MB CSV with 106,882 tracks loaded into memory

## 📦 Technologies

| Layer | Tech |
|-------|------|
| Frontend | Next.js 16, React 19, TypeScript, Tailwind CSS, Radix UI |
| Backend | C++17, Crow framework, header-only, embedded in app |
| Dataset | 106,882 tracks with audio features (danceability, energy, etc.) |

## 🎨 Features

- ✨ Multiple recommendation strategies
- 🎵 Real-time predictions from C++ engine  
- 💫 Smooth card animations and flip effects
- 📊 Audio metrics visualization
- 🌙 Dark mode support
- ⚡ Fast, responsive UI
- 🔄 CORS-enabled API

---

**Ready to use!** Open http://localhost:3000 and try out different recommendation strategies!
