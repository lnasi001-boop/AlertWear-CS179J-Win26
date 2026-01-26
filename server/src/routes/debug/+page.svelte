<script lang="ts">
    export let data;
    
    // Auto-refresh every 2 seconds
    import { onMount } from 'svelte';
    import { invalidateAll } from '$app/navigation';
    
    onMount(() => {
        const interval = setInterval(() => {
            invalidateAll();
        }, 2000);
        
        return () => clearInterval(interval);
    });
</script>

<svelte:head>
    <title>Debug Console - AlertWear</title>
</svelte:head>

<div class="container">
    <header>
        <h1>🔧 Debug Console</h1>
        <div class="status">
            <span class="dot green"></span>
            MQTT Connected | Broker: localhost:1883
        </div>
    </header>

    <div class="console">
        {#if data.messages.length === 0}
            <p class="empty">No messages yet. Waiting for MQTT data...</p>
        {:else}
            {#each data.messages as msg}
                <div class="message">
                    <span class="time">[{new Date(msg.timestamp).toLocaleTimeString()}]</span>
                    <span class="topic">{msg.topic}</span>
                    <span class="payload">{msg.payload}</span>
                </div>
            {/each}
        {/if}
    </div>
</div>

<style>
    .container {
        font-family: system-ui, -apple-system, sans-serif;
        max-width: 1200px;
        margin: 0 auto;
        padding: 20px;
    }

    header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 20px;
    }

    h1 {
        margin: 0;
        font-size: 24px;
    }

    .status {
        display: flex;
        align-items: center;
        gap: 8px;
        color: #666;
        font-size: 14px;
    }

    .dot {
        width: 10px;
        height: 10px;
        border-radius: 50%;
    }

    .dot.green {
        background: #22c55e;
    }

    .console {
        background: #0f172a;
        border-radius: 8px;
        padding: 16px;
        min-height: 500px;
        max-height: 70vh;
        overflow-y: auto;
    }

    .empty {
        color: #64748b;
        font-style: italic;
    }

    .message {
        font-family: 'Monaco', 'Menlo', monospace;
        font-size: 13px;
        padding: 4px 0;
        border-bottom: 1px solid #1e293b;
    }

    .time {
        color: #64748b;
        margin-right: 12px;
    }

    .topic {
        color: #22c55e;
        margin-right: 12px;
    }

    .payload {
        color: #f8fafc;
    }
</style>