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
    gasResistance: number;     // Raw gas resistance in kOhm
    iaq: number;               // Calculated IAQ index (0-500)
    iaqLabel: string;          // Human-readable label
    temperature: number | null;
    humidity: number | null;
    pressure: number | null;
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

import anchorsConfig from '$lib/config/anchors.json';

export const anchors: Anchor[] = anchorsConfig.map(a => ({
    ...a,
    online: false,
    lastSeen: null
}));

export interface AlertRecord {
    tagId: number;
    fullName: string;
    empId: string;
    type: 'gas' | 'panic';
    gasResistance: number;
    iaq: number;
    timestamp: string;
}

export const alertHistory: AlertRecord[] = [];

// Track latest range readings per tag (need 3+ anchors for trilateration)
const tagReadings: Map<number, Map<string, RangeReading>> = new Map();

// Track calculated positions
export const workerPositions: Map<number, WorkerPosition> = new Map();

// ============================================================
// IAQ Calculation from raw gas resistance (kOhm)
// ============================================================
// BME680 gas resistance: higher = cleaner air, lower = polluted
// IAQ scale: 0 = excellent, 500 = hazardous
//
// This is a simplified approximation. Bosch's BSEC library
// uses a more complex algorithm with humidity compensation
// and baseline tracking over time.
// ============================================================

function gasResistanceToIAQ(gasResistance: number): { iaq: number; label: string } {
    // Gas resistance in kOhm -> IAQ score (0-500)
    // Based on typical BME680 indoor readings:
    //   Fresh air:    100-500+ kOhm
    //   Normal room:  50-100 kOhm  
    //   Stuffy/VOCs:  20-50 kOhm
    //   Poor air:     10-20 kOhm
    //   Hazardous:    <10 kOhm

    let iaq: number;

    if (gasResistance >= 300) {
        // Excellent: 300+ kOhm -> IAQ 0-25
        iaq = Math.max(0, 25 - ((gasResistance - 300) / 200) * 25);
    } else if (gasResistance >= 150) {
        // Good: 150-300 kOhm -> IAQ 25-75
        iaq = 25 + ((300 - gasResistance) / 150) * 50;
    } else if (gasResistance >= 75) {
        // Moderate: 75-150 kOhm -> IAQ 75-150
        iaq = 75 + ((150 - gasResistance) / 75) * 75;
    } else if (gasResistance >= 30) {
        // Poor: 30-75 kOhm -> IAQ 150-250
        iaq = 150 + ((75 - gasResistance) / 45) * 100;
    } else if (gasResistance >= 10) {
        // Unhealthy: 10-30 kOhm -> IAQ 250-350
        iaq = 250 + ((30 - gasResistance) / 20) * 100;
    } else {
        // Hazardous: <10 kOhm -> IAQ 350-500
        iaq = 350 + ((10 - gasResistance) / 10) * 150;
    }

    iaq = Math.round(Math.max(0, Math.min(500, iaq)));

    let label: string;
    if (iaq <= 50) label = 'Excellent';
    else if (iaq <= 100) label = 'Good';
    else if (iaq <= 150) label = 'Moderate';
    else if (iaq <= 200) label = 'Poor';
    else if (iaq <= 300) label = 'Unhealthy';
    else label = 'Hazardous';

    return { iaq, label };
}

// Determine status based on IAQ index
function getStatus(iaq: number): 'ok' | 'warning' | 'alert' {
    if (iaq >= 200) return 'alert';     // Unhealthy or worse
    if (iaq >= 150) return 'warning';   // Poor
    return 'ok';                         // Moderate or better
}

// Trilateration: calculate position from 3+ anchor distances
function trilaterate(readings: Map<string, RangeReading>): { x: number; y: number } | null {
    if (readings.size < 3) return null;

    const points: { x: number; y: number; d: number }[] = [];
    
    readings.forEach((reading, anchorId) => {
        const anchor = anchors.find(a => a.anchorId === String(anchorId));
        if (anchor) {
            points.push({ x: anchor.x, y: anchor.y, d: reading.distance });
        }
    });

    if (points.length < 3) return null;

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

    return {
        x: Math.max(0, Math.min(7.5, x)),
        y: Math.max(0, Math.min(7.5, y))
    };
}

// Process incoming MQTT data from anchors
// Anchors now send both distance AND gas data (piggyback)
export function processReading(data: {
    tagId: number;
    distance: number;
    anchorId: string;
    timestamp: string;
    // Piggyback sensor fields (optional - included when anchor has received AT+RDATA)
    temperature?: number;
    humidity?: number;
    pressure?: number;
    gas?: number;
}) {
    const { tagId, distance, anchorId, timestamp } = data;

    // Check if this worker is registered
    const registeredWorker = workers.find(w => w.tagId === tagId);
    if (!registeredWorker) return;

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

    // Get existing data for this worker
    const existing = workerPositions.get(tagId);

    // Calculate IAQ from gas resistance if we have gas data
    let gasResistance = existing?.gasResistance ?? 0;
    let iaq = existing?.iaq ?? 0;
    let iaqLabel = existing?.iaqLabel ?? 'Unknown';
    let temperature = existing?.temperature ?? null;
    let humidity = existing?.humidity ?? null;
    let pressure = existing?.pressure ?? null;

    // Update sensor data if present in this message (piggyback)
    if (data.gas !== undefined) {
        gasResistance = data.gas;
        const iaqResult = gasResistanceToIAQ(gasResistance);
        iaq = iaqResult.iaq;
        iaqLabel = iaqResult.label;
    }
    if (data.temperature !== undefined) temperature = data.temperature;
    if (data.humidity !== undefined) humidity = data.humidity;
    if (data.pressure !== undefined) pressure = data.pressure;

    const status = getStatus(iaq);

    // Log alert if status is alert and we have new gas data
    if (status === 'alert' && data.gas !== undefined) {
        alertHistory.unshift({
            tagId,
            fullName: registeredWorker.fullName,
            empId: registeredWorker.empId,
            type: 'gas',
            gasResistance,
            iaq,
            timestamp: new Date().toISOString()
        });
        if (alertHistory.length > 200) alertHistory.pop();
    }

    if (position) {
        workerPositions.set(tagId, {
            tagId,
            fullName: registeredWorker.fullName,
            empId: registeredWorker.empId,
            x: parseFloat(position.x.toFixed(2)),
            y: parseFloat(position.y.toFixed(2)),
            gasResistance,
            iaq,
            iaqLabel,
            temperature,
            humidity,
            pressure,
            status,
            lastSeen: timestamp
        });
    } else if (existing) {
        // Update sensor data even without new position
        workerPositions.set(tagId, {
            ...existing,
            gasResistance,
            iaq,
            iaqLabel,
            temperature,
            humidity,
            pressure,
            status,
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

export function getAlertHistory(): AlertRecord[] {
    return alertHistory;
}