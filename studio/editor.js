// Zenu Studio V2: Professional Visual Editor

let editorInstance;
let currentScene = { objects: [] };
let selectedObjectId = null;
let currentFilePath = 'game/main.cpp';

// Viewport Camera State
let camX = 640; // Center of 1280
let camY = 360; // Center of 720
let zoom = 2.0;

const canvas = document.getElementById('engine-viewport');
const ctx = canvas.getContext('2d');

// --- Monaco Setup ---
require.config({ paths: { 'vs': 'https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.44.0/min/vs' } });
require(['vs/editor/editor.main'], () => {
    editorInstance = monaco.editor.create(document.getElementById('monaco-container'), {
        value: '// Select a file to edit',
        language: 'cpp', theme: 'vs-dark', automaticLayout: true, minimap: { enabled: false }
    });
    setTimeout(() => openFile('game/main.cpp'), 500);
});

// --- File System ---
async function loadFiles() {
    const res = await fetch('/files');
    const files = await res.json();
    renderFileTree(files, document.getElementById('file-system-panel'));
}

function renderFileTree(files, container) {
    container.innerHTML = '';
    const ul = document.createElement('ul');
    ul.className = 'tree-view';
    files.forEach(file => {
        const li = document.createElement('li');
        li.textContent = (file.type === 'dir' ? '📁 ' : '📄 ') + file.name;
        if (file.type === 'file') li.onclick = (e) => { e.stopPropagation(); openFile(file.path); };
        if (file.children) {
            const childDiv = document.createElement('div');
            childDiv.style.paddingLeft = '15px'; childDiv.style.display = 'none';
            li.onclick = (e) => { e.stopPropagation(); childDiv.style.display = childDiv.style.display === 'none' ? 'block' : 'none'; };
            renderFileTree(file.children, childDiv);
            li.appendChild(childDiv);
        }
        ul.appendChild(li);
    });
    container.appendChild(ul);
}

async function openFile(path) {
    currentFilePath = path;
    const res = await fetch(`/file/content?path=${encodeURIComponent(path)}`);
    const content = await res.text();
    editorInstance.setValue(content);
    const ext = path.split('.').pop();
    monaco.editor.setModelLanguage(editorInstance.getModel(), (ext === 'cpp' || ext === 'hpp') ? 'cpp' : 'javascript');
    document.querySelector('.tab.active').textContent = path.split('/').pop();
    log(`Opened: ${path}`);
}

async function saveCode() {
    if (!editorInstance) return;
    const code = editorInstance.getValue();
    if (code.includes('Select a file to edit')) return;
    await fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code, path: currentFilePath })
    });
}

async function saveScene() {
    await fetch('/scene', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(currentScene)
    });
}

// --- Console & Logs ---
function log(msg, color = '#8b949e') {
    const consoleOut = document.getElementById('console-output');
    const div = document.createElement('div');
    div.style.color = color;
    div.style.marginBottom = '2px';
    div.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
    consoleOut.appendChild(div);
    consoleOut.scrollTop = consoleOut.scrollHeight;
}

const eventSource = new EventSource('/logs');
eventSource.onmessage = (event) => {
    const data = JSON.parse(event.data);
    log(data.msg, data.msg.toLowerCase().includes('error') ? '#ff5555' : '#8b949e');
};

// --- Viewport Graphics & Interaction ---
function worldToScreen(x, y) {
    return {
        x: (x - camX) * zoom + canvas.width / 2,
        y: (y - camY) * zoom + canvas.height / 2
    };
}

function screenToWorld(sx, sy) {
    return {
        x: (sx - canvas.width / 2) / zoom + camX,
        y: (sy - canvas.height / 2) / zoom + camY
    };
}

function renderViewport() {
    ctx.fillStyle = '#0a0a0a';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.strokeStyle = '#1a1a1a';
    ctx.lineWidth = 1;
    for (let x = -1000; x < 1000; x += 50) {
        const p1 = worldToScreen(x, -1000); const p2 = worldToScreen(x, 1000);
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y); ctx.stroke();
    }
    for (let y = -1000; y < 1000; y += 50) {
        const p1 = worldToScreen(-1000, y); const p2 = worldToScreen(1000, y);
        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y); ctx.stroke();
    }

    const camTopLeft = worldToScreen(0, 0);
    const camSize = { w: 160 * zoom, h: 144 * zoom };
    ctx.strokeStyle = '#58a6ff'; ctx.setLineDash([5, 5]);
    ctx.strokeRect(camTopLeft.x, camTopLeft.y, camSize.w, camSize.h);
    ctx.setLineDash([]);
    ctx.fillStyle = '#58a6ff'; ctx.font = '10px Inter';
    ctx.fillText('CAMERA (160x144)', camTopLeft.x, camTopLeft.y - 5);

    currentScene.objects.forEach(obj => {
        const p = worldToScreen(obj.x, obj.y);
        const w = (obj.w || 16) * zoom; const h = (obj.h || 16) * zoom;
        ctx.globalAlpha = obj.opacity / 255;
        ctx.fillStyle = obj.color || '#ffff00';
        ctx.fillRect(p.x - w / 2, p.y - h / 2, w, h);
        if (selectedObjectId === obj.id) {
            ctx.globalAlpha = 1.0; ctx.strokeStyle = '#00e5ff'; ctx.lineWidth = 2;
            ctx.strokeRect(p.x - w / 2 - 2, p.y - h / 2 - 2, w + 4, h + 4);
            ctx.fillStyle = '#fff'; ctx.fillRect(p.x + w / 2 - 4, p.y + h / 2 - 4, 8, 8);
        }
    });
}

function getMousePos(e) {
    const rect = canvas.getBoundingClientRect();
    return { x: (e.clientX - rect.left) * (canvas.width / rect.width), y: (e.clientY - rect.top) * (canvas.height / rect.height) };
}

let isPanning = false, isDragging = false, isResizing = false, lastMouse = { x: 0, y: 0 };

canvas.onwheel = (e) => {
    if (e.shiftKey) {
        e.preventDefault();
        zoom *= (e.deltaY < 0 ? 1.1 : 0.9);
        zoom = Math.max(0.1, Math.min(10, zoom));
    }
};

canvas.onmousedown = (e) => {
    const m = getMousePos(e); const w = screenToWorld(m.x, m.y);
    if (e.button === 1) isPanning = true;
    else {
        const obj = currentScene.objects.find(o => Math.abs(o.x - w.x) < (o.w || 16) / 2 + 5 && Math.abs(o.y - w.y) < (o.h || 16) / 2 + 5);
        if (obj) {
            selectObject(obj.id);
            const p = worldToScreen(obj.x, obj.y); const ow = (obj.w || 16) * zoom, oh = (obj.h || 16) * zoom;
            if (Math.abs(m.x - (p.x + ow / 2)) < 15 && Math.abs(m.y - (p.y + oh / 2)) < 15) isResizing = true;
            else isDragging = true;
        }
    }
    lastMouse = m;
};

window.onmousemove = (e) => {
    if (!isPanning && !isDragging && !isResizing) return;
    const m = getMousePos(e); const dx = m.x - lastMouse.x, dy = m.y - lastMouse.y;
    if (isPanning) { camX -= dx / zoom; camY -= dy / zoom; }
    else if ((isDragging || isResizing) && selectedObjectId) {
        const obj = currentScene.objects.find(o => o.id === selectedObjectId);
        if (isDragging) { obj.x += dx / zoom; obj.y += dy / zoom; }
        else { obj.w = (obj.w || 16) + dx / zoom; obj.h = (obj.h || 16) + dy / zoom; }
        renderInspector();
    }
    lastMouse = m;
};

window.onmouseup = () => { if (isDragging || isResizing) saveScene(); isPanning = isDragging = isResizing = false; };

// --- Scene Management ---
async function loadScene() { const res = await fetch('/scene'); currentScene = await res.json(); renderOutliner(); }
function renderOutliner() {
    const list = document.getElementById('scene-outliner'); list.innerHTML = '';
    currentScene.objects.forEach(obj => {
        const li = document.createElement('li'); li.textContent = obj.name;
        if (selectedObjectId === obj.id) li.classList.add('selected');
        li.onclick = () => selectObject(obj.id); list.appendChild(li);
    });
}
function selectObject(id) { selectedObjectId = id; renderOutliner(); renderInspector(); }
function renderInspector() {
    const panel = document.getElementById('inspector-content'); const obj = currentScene.objects.find(o => o.id === selectedObjectId);
    if (!obj) { panel.innerHTML = '<p class="empty-msg">Select an object</p>'; return; }
    panel.innerHTML = `
        <label>Name</label><input type="text" value="${obj.name}" onchange="updateProp('name', this.value)">
        <label>X</label><input type="number" value="${Math.round(obj.x)}" onchange="updateProp('x', parseInt(this.value))">
        <label>Y</label><input type="number" value="${Math.round(obj.y)}" onchange="updateProp('y', parseInt(this.value))">
        <label>W</label><input type="number" value="${Math.round(obj.w || 16)}" onchange="updateProp('w', parseInt(this.value))">
        <label>H</label><input type="number" value="${Math.round(obj.h || 16)}" onchange="updateProp('h', parseInt(this.value))">
        <label>Opacity</label><input type="range" min="0" max="255" value="${obj.opacity}" oninput="updateProp('opacity', parseInt(this.value))">
        <label>Color</label><input type="color" value="${obj.color || '#ffff00'}" onchange="updateProp('color', this.value)">
    `;
}
function updateProp(key, val) { const obj = currentScene.objects.find(o => o.id === selectedObjectId); if (obj) { obj[key] = val; saveScene(); } }

// --- Buttons ---
document.getElementById('btn-play').onclick = async () => { await saveScene(); await saveCode(); fetch('/run', { method: 'POST' }); };
document.getElementById('btn-stop').onclick = () => fetch('/stop', { method: 'POST' });
document.getElementById('btn-build').onclick = async () => { await saveScene(); await saveCode(); fetch('/build', { method: 'POST' }); };
document.getElementById('add-obj').onclick = () => {
    const newObj = { id: Date.now().toString(), name: "GameObject", x: 80, y: 72, w: 16, h: 16, opacity: 255, type: "rect", color: "#00e5ff" };
    currentScene.objects.push(newObj); saveScene(); renderOutliner(); selectObject(newObj.id);
};

// --- Project Management ---
const modal = document.getElementById('project-modal');
const modalTitle = document.getElementById('modal-title');
const modalBody = document.getElementById('modal-body');
const modalClose = document.getElementById('modal-close');
const projectLabel = document.getElementById('current-project-label');

modalClose.onclick = () => modal.classList.add('hidden');

document.getElementById('btn-open-project').onclick = async () => {
    const res = await fetch('/projects');
    const data = await res.json();
    modalTitle.textContent = 'Open Project';
    modalBody.innerHTML = data.projects.length === 0 ? '<p style="color:#888">No projects found. Create one!</p>' : '';
    data.projects.forEach(name => {
        const btn = document.createElement('button');
        btn.className = 'btn-outline';
        btn.textContent = '📁 ' + name;
        btn.onclick = async () => {
            await fetch('/projects/open', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ name }) });
            projectLabel.textContent = name;
            currentFilePath = `game/${name}/main.cpp`;
            openFile(currentFilePath);
            loadScene();
            modal.classList.add('hidden');
        };
        modalBody.appendChild(btn);
    });
    modal.classList.remove('hidden');
};

document.getElementById('btn-new-project').onclick = () => {
    modalTitle.textContent = 'New Project';
    modalBody.innerHTML = '<input type="text" id="new-project-name" placeholder="Project name (e.g. pong)">';
    const createBtn = document.createElement('button');
    createBtn.className = 'btn-primary';
    createBtn.textContent = 'Create';
    createBtn.onclick = async () => {
        const name = document.getElementById('new-project-name').value.trim().toLowerCase().replace(/\s+/g, '_');
        if (!name) return;
        await fetch('/projects/create', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ name }) });
        projectLabel.textContent = name;
        currentFilePath = `game/${name}/main.cpp`;
        openFile(currentFilePath);
        loadScene();
        modal.classList.add('hidden');
    };
    modalBody.appendChild(createBtn);
    modal.classList.remove('hidden');
};

// --- Execution ---
loadFiles(); loadScene(); setInterval(renderViewport, 32); canvas.oncontextmenu = (e) => e.preventDefault();

// Check for current project on load
(async () => {
    const res = await fetch('/projects');
    const data = await res.json();
    if (data.current) {
        projectLabel.textContent = data.current;
        currentFilePath = `game/${data.current}/main.cpp`;
        openFile(currentFilePath);
    }
})();
