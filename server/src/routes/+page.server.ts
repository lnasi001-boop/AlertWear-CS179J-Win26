import { getWorkerPositions, getAnchors } from '$lib/server/store';

export function load() {
    return {
        workers: getWorkerPositions(),
        anchors: getAnchors()
    };
}