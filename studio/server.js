const express = require('express');
const { exec, spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const cors = require('cors');
const multer = require('multer');

const app = express();
const port = 3000;

app.use(cors());
app.use(express.json());
app.use(express.static(__dirname));

const upload = multer({ dest: 'assets/uploads/' });

let gameProcess = null;
let logClients = [];

// Base paths
const rootDir = path.join(__dirname, '..');
const gamesDir = path.join(rootDir, 'game');
let currentProject = null; // e.g. 'tetris' -> game/tetris/main.cpp
let scenePath = path.join(gamesDir, 'scene.json');
let gameDir = gamesDir;

// Ensure directories exist
if (!fs.existsSync(gamesDir)) fs.mkdirSync(gamesDir, { recursive: true });

// --- Project Management API ---

app.get('/projects', (req, res) => {
    const projects = [];
    const items = fs.readdirSync(gamesDir);
    items.forEach(item => {
        const itemPath = path.join(gamesDir, item);
        if (fs.statSync(itemPath).isDirectory()) {
            if (fs.existsSync(path.join(itemPath, 'main.cpp'))) {
                projects.push(item);
            }
        }
    });
    res.json({ projects, current: currentProject });
});

app.post('/projects/open', (req, res) => {
    const { name } = req.body;
    const projectPath = path.join(gamesDir, name);
    if (fs.existsSync(path.join(projectPath, 'main.cpp'))) {
        currentProject = name;
        gameDir = projectPath;
        scenePath = path.join(projectPath, 'scene.json');
        broadcastLog(`Opened project: ${name}`);
        res.json({ status: 'ok', project: name });
    } else {
        res.status(404).json({ error: 'Project not found' });
    }
});

app.post('/projects/create', (req, res) => {
    const { name } = req.body;
    const projectPath = path.join(gamesDir, name);
    if (fs.existsSync(projectPath)) {
        return res.status(400).json({ error: 'Project already exists' });
    }
    fs.mkdirSync(projectPath, { recursive: true });
    // Create default main.cpp
    const defaultMain = `#include "zenu.hpp"
#include "scene_generated.hpp"

using namespace zenu;

void game_init() {}
void game_update() {}
void game_draw() {
    gfx_clear(Color::from_rgba(20, 20, 40));
    gfx_draw_text(50, 60, "${name.toUpperCase()}", Color::from_rgba(255, 255, 0));
}
`;
    fs.writeFileSync(path.join(projectPath, 'main.cpp'), defaultMain);
    // Create default scene
    const defaultScene = { objects: [{ id: "player", name: "Player", x: 80, y: 72, w: 16, h: 16, opacity: 255, type: "rect", color: "#ffff00" }] };
    fs.writeFileSync(path.join(projectPath, 'scene.json'), JSON.stringify(defaultScene, null, 4));
    // Create scene_generated.hpp
    fs.writeFileSync(path.join(projectPath, 'scene_generated.hpp'), `#ifndef SCENE_GENERATED_HPP
#define SCENE_GENERATED_HPP
namespace scene {
    const int player_x = 80;
    const int player_y = 72;
    const int player_w = 16;
    const int player_h = 16;
    const int player_opacity = 255;
}
#endif
`);
    currentProject = name;
    gameDir = projectPath;
    scenePath = path.join(projectPath, 'scene.json');
    broadcastLog(`Created new project: ${name}`);
    res.json({ status: 'created', project: name });
});

// --- API ---

app.get('/files', (req, res) => {
    function readDirRecursive(dir) {
        let results = [];
        if (!fs.existsSync(dir)) return [];
        const list = fs.readdirSync(dir);
        list.forEach(file => {
            if (file === 'node_modules' || file === '.git' || file === 'build') return;
            const fullPath = path.join(dir, file);
            const stat = fs.statSync(fullPath);
            if (stat.isDirectory()) {
                results.push({ name: file, type: 'dir', children: readDirRecursive(fullPath) });
            } else {
                results.push({ name: file, type: 'file', path: path.relative(rootDir, fullPath) });
            }
        });
        return results;
    }
    res.json(readDirRecursive(rootDir));
});

app.get('/file/content', (req, res) => {
    const filePath = path.join(rootDir, req.query.path);
    if (fs.existsSync(filePath)) {
        res.send(fs.readFileSync(filePath, 'utf8'));
    } else {
        res.status(404).send('File not found');
    }
});

app.post('/save', (req, res) => {
    const { code, path: filePath } = req.body;
    if (!filePath) return res.status(400).send('No path provided');
    fs.writeFileSync(path.join(rootDir, filePath), code);
    res.send({ status: 'saved' });
});

app.get('/scene', (req, res) => {
    if (!fs.existsSync(scenePath)) {
        const defaultScene = { objects: [{ id: "player", name: "Player", x: 80, y: 72, w: 16, h: 16, opacity: 255, type: "rect", color: "#ffff00" }] };
        fs.writeFileSync(scenePath, JSON.stringify(defaultScene, null, 4));
    }
    res.json(JSON.parse(fs.readFileSync(scenePath)));
});

app.post('/scene', (req, res) => {
    const scene = req.body;
    fs.writeFileSync(scenePath, JSON.stringify(scene, null, 4));

    // Generate C++ header
    let header = `#ifndef SCENE_GENERATED_HPP\n#define SCENE_GENERATED_HPP\n\n`;
    header += `namespace scene {\n`;
    scene.objects.forEach(obj => {
        const safeName = obj.name.replace(/\s+/g, '_').toLowerCase();
        header += `    // Transform for ${obj.name}\n`;
        header += `    const int ${safeName}_x = ${Math.round(obj.x)};\n`;
        header += `    const int ${safeName}_y = ${Math.round(obj.y)};\n`;
        header += `    const int ${safeName}_w = ${Math.round(obj.w || 16)};\n`;
        header += `    const int ${safeName}_h = ${Math.round(obj.h || 16)};\n`;
        header += `    const int ${safeName}_opacity = ${obj.opacity};\n\n`;
    });
    header += `}\n\n#endif\n`;

    fs.writeFileSync(path.join(gameDir, 'scene_generated.hpp'), header);
    broadcastLog('Scene sync: transforms (XYWH) updated.');
    res.send({ status: 'ok' });
});

// Logs SSE
app.get('/logs', (req, res) => {
    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
    logClients.push(res);
    req.on('close', () => logClients = logClients.filter(client => client !== res));
});

function broadcastLog(msg) {
    const data = JSON.stringify({ time: new Date().toLocaleTimeString(), msg });
    logClients.forEach(client => client.write(`data: ${data}\n\n`));
}

function runCommand(command, args, cwd, onExit) {
    const child = spawn(command, args, { cwd });
    child.stdout.on('data', (data) => broadcastLog(data.toString().trim()));
    child.stderr.on('data', (data) => broadcastLog(`Error: ${data.toString().trim()}`));
    child.on('close', (code) => {
        if (code === 0) broadcastLog(`${command} finished successfully.`);
        else broadcastLog(`${command} failed with code ${code}.`);
        if (onExit) onExit(code);
    });
    return child;
}

app.post('/run', (req, res) => {
    if (gameProcess) {
        broadcastLog('Stopping previous game process...');
        gameProcess.kill();
    }
    const gameSrc = currentProject ? `game/${currentProject}/main.cpp` : 'game/main.cpp';
    broadcastLog(`--- Starting PC Build & Run (${gameSrc}) ---`);
    runCommand('make', ['-f', 'Makefile.engine', 'PLATFORM=PC', `GAME_SRC=${gameSrc}`], rootDir, (code) => {
        if (code === 0) {
            broadcastLog('Launching game...');
            gameProcess = spawn('./build/game', [], { cwd: rootDir });
            gameProcess.stdout.on('data', (data) => broadcastLog(`[Game] ${data.toString().trim()}`));
            gameProcess.stderr.on('data', (data) => broadcastLog(`[Game Error] ${data.toString().trim()}`));
            res.send({ status: 'running' });
        } else {
            res.status(500).send('Build failed');
        }
    });
});

app.post('/build', (req, res) => {
    const gameSrc = currentProject ? `game/${currentProject}/main.cpp` : 'game/main.cpp';
    broadcastLog(`--- Starting RISC-V Zenu Build (${gameSrc}) ---`);
    runCommand('make', ['-f', 'Makefile.engine', 'PLATFORM=ZENU', `GAME_SRC=${gameSrc}`], rootDir, (code) => {
        if (code === 0) {
            broadcastLog('ROM Build complete: build/game.boc');
            res.send({ status: 'built' });
        } else {
            res.status(500).send('Zenu build failed');
        }
    });
});

app.post('/stop', (req, res) => {
    if (gameProcess) {
        broadcastLog('Process stopped by user.');
        gameProcess.kill();
    }
    gameProcess = null;
    res.send({ status: 'stopped' });
});

app.listen(port, () => console.log(`Zenu Studio V2 running at http://localhost:${port}`));
