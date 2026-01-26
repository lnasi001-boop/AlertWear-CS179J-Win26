import { json } from '@sveltejs/kit';
import fs from 'fs';
import path from 'path';

const configPath = path.resolve('src/lib/config/anchors.json');

function readAnchors() {
    const data = fs.readFileSync(configPath, 'utf-8');
    return JSON.parse(data);
}

function writeAnchors(anchors: any[]) {
    fs.writeFileSync(configPath, JSON.stringify(anchors, null, 4));
}

// GET - List all anchors
export async function GET() {
    const anchors = readAnchors();
    return json(anchors);
}

// POST - Add new anchor
export async function POST({ request }) {
    const newAnchor = await request.json();
    const anchors = readAnchors();
    
    if (anchors.find((a: any) => a.anchorId === newAnchor.anchorId)) {
        return json({ error: 'Anchor ID already exists' }, { status: 400 });
    }
    
    anchors.push(newAnchor);
    writeAnchors(anchors);
    
    return json(newAnchor, { status: 201 });
}

// PUT - Update anchor
export async function PUT({ request }) {
    const updatedAnchor = await request.json();
    const anchors = readAnchors();
    
    const index = anchors.findIndex((a: any) => a.anchorId === updatedAnchor.anchorId);
    if (index === -1) {
        return json({ error: 'Anchor not found' }, { status: 404 });
    }
    
    anchors[index] = updatedAnchor;
    writeAnchors(anchors);
    
    return json(updatedAnchor);
}

// DELETE - Remove anchor
export async function DELETE({ request }) {
    const { anchorId } = await request.json();
    const anchors = readAnchors();
    
    const filtered = anchors.filter((a: any) => a.anchorId !== anchorId);
    if (filtered.length === anchors.length) {
        return json({ error: 'Anchor not found' }, { status: 404 });
    }
    
    writeAnchors(filtered);
    
    return json({ success: true });
}