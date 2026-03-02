<script lang="ts">
    import { onMount } from 'svelte';
    import { invalidateAll } from '$app/navigation';
    import Map from '$lib/components/Map.svelte';
    import WorkerCard from '$lib/components/WorkerCard.svelte';
    import AlertLog from '$lib/components/AlertLog.svelte';

    interface Worker {
        tagId: number;
        fullName: string;
        empId: string;
        x: number;
        y: number;
        gasResistance: number;
        iaq: number;
        iaqLabel: string;
        temperature: number | null;
        humidity: number | null;
        pressure: number | null;
        status: 'ok' | 'warning' | 'alert';
    }

    interface Anchor {
        anchorId: string;
        name: string;
        x: number;
        y: number;
        online: boolean;
    }

    export let data: {
        workers: Worker[];
        anchors: Anchor[];
        alerts: any[];
    };

    onMount(() => {
        const interval = setInterval(() => {
            invalidateAll();
        }, 1000);

        return () => clearInterval(interval);
    });

    $: hasAlert = data.workers.some((w) => w.status === 'alert');
    $: onlineAnchors = data.anchors.filter((a) => a.online).length;

    // Alert sound
    let alertAudio: HTMLAudioElement;
    let alertPlaying = false;

    onMount(() => {
        alertAudio = new Audio('https://actions.google.com/sounds/v1/alarms/alarm_clock.ogg');
        alertAudio.loop = true;
    });

    $: if (hasAlert && !alertPlaying && alertAudio) {
        alertAudio.play();
        alertPlaying = true;
    } else if (!hasAlert && alertPlaying && alertAudio) {
        alertAudio.pause();
        alertAudio.currentTime = 0;
        alertPlaying = false;
    }
</script>

<svelte:head>
    <title>Dashboard - VERTEX</title>
</svelte:head>

{#if hasAlert}
    <div class="alert-banner">
        <span>⚠️ GAS ALERT DETECTED - Check worker status immediately!</span>
        <button class="mute-btn" on:click={() => { alertAudio.pause(); alertAudio.currentTime = 0; alertPlaying = false; }}>
            🔇 Mute
        </button>
    </div>
{/if}

<div class="dashboard">
    <div class="status-bar">
        <span class="dot green"></span>
        Live | {data.workers.length} Workers | {onlineAnchors}/{data.anchors.length} Anchors Online
    </div>

    <div class="content">
        <div class="map-section">
            <h2>Facility Map</h2>
            <Map workers={data.workers} anchors={data.anchors} />
        </div>

        <div class="workers-section">
            <h2>Workers ({data.workers.length})</h2>
            <div class="workers-list">
                {#if data.workers.length === 0}
                    <p class="no-data">Waiting for worker data...</p>
                {:else}
                    {#each data.workers as worker (worker.tagId)}
                        <WorkerCard {worker} />
                    {/each}
                {/if}
            </div>
        </div>
    </div>

    <div class="alert-section">
        <AlertLog alerts={data.alerts} />
    </div>
</div>

<style>
    .alert-banner {
        background: #ef4444;
        color: white;
        padding: 12px;
        font-weight: bold;
        animation: flash 1s infinite;
        display: flex;
        justify-content: center;
        align-items: center;
        gap: 16px;
    }

    .mute-btn {
        background: rgba(255, 255, 255, 0.2);
        color: white;
        border: 1px solid rgba(255, 255, 255, 0.4);
        padding: 6px 14px;
        border-radius: 6px;
        cursor: pointer;
        font-size: 13px;
        font-weight: 600;
    }

    .mute-btn:hover {
        background: rgba(255, 255, 255, 0.3);
    }

    @keyframes flash {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.7; }
    }

    .dashboard {
        min-height: calc(100vh - 60px);
        background: var(--bg-primary);
    }

    .status-bar {
        display: flex;
        align-items: center;
        gap: 8px;
        padding: 12px 24px;
        background: var(--bg-status);
        font-size: 14px;
        color: var(--text-secondary);
    }

    .dot {
        width: 10px;
        height: 10px;
        border-radius: 50%;
    }

    .dot.green {
        background: #22c55e;
        animation: pulse-dot 2s infinite;
    }

    @keyframes pulse-dot {
        0%, 100% { opacity: 1; box-shadow: 0 0 0 0 rgba(34, 197, 94, 0.4); }
        50% { opacity: 0.8; box-shadow: 0 0 0 6px rgba(34, 197, 94, 0); }
    }

    .content {
        display: flex;
        gap: 24px;
        padding: 24px;
        max-width: 1400px;
        margin: 0 auto;
    }

    .map-section {
        flex: 1;
        background: var(--bg-card);
        border-radius: 12px;
        padding: 20px;
        box-shadow: var(--shadow);
    }

    .workers-section {
        width: 320px;
        background: var(--bg-card);
        border-radius: 12px;
        padding: 20px;
        box-shadow: var(--shadow);
        max-height: calc(100vh - 180px);
        overflow-y: auto;
    }

    h2 {
        margin: 0 0 16px 0;
        font-size: 18px;
        color: var(--text-primary);
    }

    .workers-list {
        display: flex;
        flex-direction: column;
        gap: 12px;
    }

    .no-data {
        color: var(--text-muted);
        font-style: italic;
        text-align: center;
        padding: 20px;
    }

    .alert-section {
        padding: 0 24px 24px;
        max-width: 1400px;
        margin: 0 auto;
    }
</style>