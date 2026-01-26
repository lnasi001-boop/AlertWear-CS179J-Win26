import { json } from '@sveltejs/kit';
import fs from 'fs';
import path from 'path';

const configPath = path.resolve('src/lib/config/workers.json');

function readWorkers() {
    const data = fs.readFileSync(configPath, 'utf-8');
    return JSON.parse(data);
}

function writeWorkers(workers: any[]) {
    fs.writeFileSync(configPath, JSON.stringify(workers, null, 4));
}

// GET - List all workers
export async function GET() {
    const workers = readWorkers();
    return json(workers);
}

// POST - Add new worker
export async function POST({ request }) {
    const newWorker = await request.json();
    const workers = readWorkers();
    
    // Check if tagId already exists
    if (workers.find((w: any) => w.tagId === newWorker.tagId)) {
        return json({ error: 'Tag ID already exists' }, { status: 400 });
    }
    
    workers.push(newWorker);
    writeWorkers(workers);
    
    return json(newWorker, { status: 201 });
}

// PUT - Update worker
export async function PUT({ request }) {
    const updatedWorker = await request.json();
    const workers = readWorkers();
    
    const index = workers.findIndex((w: any) => w.tagId === updatedWorker.tagId);
    if (index === -1) {
        return json({ error: 'Worker not found' }, { status: 404 });
    }
    
    workers[index] = updatedWorker;
    writeWorkers(workers);
    
    return json(updatedWorker);
}

// DELETE - Remove worker
export async function DELETE({ request }) {
    const { tagId } = await request.json();
    const workers = readWorkers();
    
    const filtered = workers.filter((w: any) => w.tagId !== tagId);
    if (filtered.length === workers.length) {
        return json({ error: 'Worker not found' }, { status: 404 });
    }
    
    writeWorkers(filtered);
    
    return json({ success: true });
}