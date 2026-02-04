import workersConfig from '$lib/config/workers.json';
import anchorsConfig from '$lib/config/anchors.json';

import { readFileSync } from 'fs';
import { resolve } from 'path';

function loadWorkers(): Worker[] {
    try {
        const data = readFileSync(resolve('src/lib/config/workers.json'), 'utf-8');
        return JSON.parse(data);
    } catch {
        return [];
    }
}

// Types
export interface Worker {
    tagId: number;
    fullName: string;
    empId: string;
}

export interface Anchor {
    anchorId: string;
    name: string;
    x: number;
    y: number;
    online: boolean;
    lastSeen: string | null;
}

export interface WorkerPosition {
    tagId: number;
    fullName: string;
    empId: string;
    x: number;
    y: number;
    gas: number;
    status: 'ok' | 'warning' | 'alert';
    lastSeen: string;
}

export interface RangeReading {
    anchorId: string;
    distance: number;
    timestamp: string;
}

// Store
export let workers: Worker[] = loadWorkers();

// Reload workers periodically (every 5 seconds)
setInterval(() => {
    workers = loadWorkers();
    // Remove positions for workers that no longer exist
    for (const tagId of workerPositions.keys()) {
        if (!workers.find(w => w.tagId === tagId)) {
            workerPositions.delete(tagId);
        }
    }
}, 5000);
export const anchors: Anchor[] = anchorsConfig.map(a => ({
    ...a,
    online: false,
    lastSeen: null
}));

// Track latest range readings per tag (need 3+ anchors for trilateration)
const tagReadings: Map<number, Map<string, RangeReading>> = new Map();

// Track calculated positions
export const workerPositions: Map<number, WorkerPosition> = new Map();

// Trilateration: calculate position from 3+ anchor distances
function trilaterate(readings: Map<string, RangeReading>): { x: number; y: number } | null {
    if (readings.size < 3) return null;

    // Get anchor positions and distances
    const points: { x: number; y: number; d: number }[] = [];
    
    readings.forEach((reading, anchorId) => {
        const anchor = anchors.find(a => a.anchorId === String(anchorId));
        if (anchor) {
            points.push({ x: anchor.x, y: anchor.y, d: reading.distance });
        }
    });

    if (points.length < 3) return null;

    // Simple trilateration using first 3 points
    const [p1, p2, p3] = points;

    const A = 2 * (p2.x - p1.x);
    const B = 2 * (p2.y - p1.y);
    const C = p1.d * p1.d - p2.d * p2.d - p1.x * p1.x + p2.x * p2.x - p1.y * p1.y + p2.y * p2.y;
    const D = 2 * (p3.x - p2.x);
    const E = 2 * (p3.y - p2.y);
    const F = p2.d * p2.d - p3.d * p3.d - p2.x * p2.x + p3.x * p3.x - p2.y * p2.y + p3.y * p3.y;

    const denominator = A * E - B * D;
    if (Math.abs(denominator) < 0.0001) return null;

    const x = (C * E - F * B) / denominator;
    const y = (A * F - D * C) / denominator;

    // Clamp to valid range (0-10m)
    return {
        x: Math.max(0, Math.min(3, x)),
        y: Math.max(0, Math.min(3, y))
    };
}

// Determine status based on gas level
function getStatus(gas: number): 'ok' | 'warning' | 'alert' {
    if (gas >= 80) return 'alert';
    if (gas >= 50) return 'warning';
    return 'ok';
}

// Process incoming MQTT data
// Process incoming MQTT data
export function processReading(data: {
    tagId: number;
    fullName: string;
    empId: string;
    distance: number;
    gas: number;
    anchorId: string;
    timestamp: string;
}) {
    const { tagId, distance, gas, anchorId, timestamp } = data;

    // Check if this worker is registered
    const registeredWorker = workers.find(w => w.tagId === tagId);
    if (!registeredWorker) {
        // Ignore data from unregistered tags
        return;
    }

    // Update anchor status
   const anchor = anchors.find(a => a.anchorId === String(anchorId));
    if (anchor) {
        anchor.online = true;
        anchor.lastSeen = timestamp;
    }

    // Store range reading
    if (!tagReadings.has(tagId)) {
        tagReadings.set(tagId, new Map());
    }
    tagReadings.get(tagId)!.set(anchorId, { anchorId, distance, timestamp });

    // Try to calculate position
    const readings = tagReadings.get(tagId)!;
    const position = trilaterate(readings);

    if (position) {
        workerPositions.set(tagId, {
            tagId,
            fullName: registeredWorker.fullName,
            empId: registeredWorker.empId,
            x: parseFloat(position.x.toFixed(2)),
            y: parseFloat(position.y.toFixed(2)),
            gas,
            status: getStatus(gas),
            lastSeen: timestamp
        });
    }
}

// Get all current worker positions
export function getWorkerPositions(): WorkerPosition[] {
    return Array.from(workerPositions.values());
}

// Get all anchors with status
export function getAnchors(): Anchor[] {
    return anchors;
}