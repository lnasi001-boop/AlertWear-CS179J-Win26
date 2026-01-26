<script lang="ts">
    import { onMount } from 'svelte';
    import { invalidateAll } from '$app/navigation';
    import Map from '$lib/components/Map.svelte';
    import WorkerCard from '$lib/components/WorkerCard.svelte';

    interface Worker {
        tagId: number;
        fullName: string;
        empId: string;
        x: number;
        y: number;
        gas: number;
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
    };

    onMount(() => {
        const interval = setInterval(() => {
            invalidateAll();
        }, 1000);

        return () => clearInterval(interval);
    });

    $: hasAlert = data.workers.some((w) => w.status === 'alert');
    $: onlineAnchors = data.anchors.filter((a) => a.online).length;
</script>

<svelte:head>
    <title>Dashboard - AlertWear</title>
</svelte:head>

{#if hasAlert}
    <div class="alert-banner">
        ⚠️ GAS ALERT DETECTED - Check worker status immediately!
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
</div>

<style>
    .alert-banner {
        background: #ef4444;
        color: white;
        text-align: center;
        padding: 12px;
        font-weight: bold;
        animation: flash 1s infinite;
    }

    @keyframes flash {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.7; }
    }

    .dashboard {
        min-height: calc(100vh - 60px);
        background: #f1f5f9;
    }

    .status-bar {
        display: flex;
        align-items: center;
        gap: 8px;
        padding: 12px 24px;
        background: #e2e8f0;
        font-size: 14px;
        color: #475569;
    }

    .dot {
        width: 10px;
        height: 10px;
        border-radius: 50%;
    }

    .dot.green {
        background: #22c55e;
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
        background: white;
        border-radius: 12px;
        padding: 20px;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
    }

    .workers-section {
        width: 320px;
        background: white;
        border-radius: 12px;
        padding: 20px;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
        max-height: calc(100vh - 180px);
        overflow-y: auto;
    }

    h2 {
        margin: 0 0 16px 0;
        font-size: 18px;
        color: #1e293b;
    }

    .workers-list {
        display: flex;
        flex-direction: column;
        gap: 12px;
    }

    .no-data {
        color: #64748b;
        font-style: italic;
        text-align: center;
        padding: 20px;
    }
</style>