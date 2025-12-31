# Zenu Studio & Engine 🚀

Zenu Studio is a comprehensive, professional-grade game development environment and engine. It features a high-fidelity visual editor, a robust cross-platform engine, and an integrated emulator for a seamless development-to-execution workflow.

## ✨ Key Features

- **Professional Visual Editor (Zenu Studio V2)**:
  - Integrated **Monaco Editor** for high-quality coding experience.
  - Real-time **Scene Inspector** and **Outliner**.
  - Project Management: Create and switch between multiple projects (Tetris, Pong, Sand Game, etc.).
  - Live Console with streaming logs.
- **Zenu Engine Core**:
  - High-performance 2D/3D graphics API.
  - Advanced Audio Processing Unit (APU) with multi-wave support.
  - Universal Gamepad/Controller support.
  - Bitmap font rendering for game UI.
- **Cross-Platform Backends**:
  - **PC Backend**: Uses SDL2 for high compatibility and performance.
  - **Zenu Backend**: Low-level RISC-V implementation for custom hardware simulation (MMIO-based).
- **Integrated Development Workflow**:
  - Build and Run directly from the browser.
  - Live scene synchronization and generated headers.

## 🕹️ Included Games

- **Tetris GBC-Style**: High-fidelity recreation with Menu, Ghost piece, Hold/Next UI, and pro effects.
- **Pong Ultra**: Arcade-quality Pong with AI difficulty, glowing effects, and sound.
- **Falling Sand**: Deep particle simulation stress-testing thousands of "intelligent pixels".

## 🛠️ Project Structure

- `/studio`: Node.js server and Web-based IDE.
- `/engine`: Core engine headers and platform backends (PC/Zenu).
- `/emulator`: C++ emulator for running Zenu bytecode.
- `/game`: Project directory containing source code for all games.
- `/sdk`: Development headers for user projects.

## 🚀 Getting Started

### Prerequisites

- **Node.js**: To run the Zenu Studio.
- **SDL2**: Development libraries for PC backend compilation.
- **RISC-V Toolchain** (optional): For compiling to the Zenu platform.

### Running Zenu Studio

1. Navigate to the `studio` directory:
   ```bash
   cd studio
   ```
2. Install dependencies (if any) and start the server:
   ```bash
   node server.js
   ```
3. Open your browser at `http://localhost:3000`.

### Building Games

From the Zenu Studio UI:
1. Use the **Open** button to select a project (e.g., `tetris`).
2. Click **Play** to compile and run on your PC.
3. Click **Build ROM** to generate a Zenu-compatible `.boc` binary.

## 📜 License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---
Developed with ❤️ by the Zenu Team.
