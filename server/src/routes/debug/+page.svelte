<script lang="ts">
    import { onMount } from 'svelte';
    import { invalidateAll } from '$app/navigation';

    export let data;

    // Filters
    let topicFilter = 'uwb/#';
    let showAnchorMessages = true;
    let showServerEvents = true;
    let showAlertsOnly = false;
    let tagFilter = 'All tags';

    // Stats
    let messagesPerSec = 0;
    let totalReceived = 0;
    let alertsTriggered = 0;
    let lastCount = 0;

    // Auto-refresh every 500ms
    onMount(() => {
        const interval = setInterval(() => {
            invalidateAll();
        }, 500);

        // Calculate messages per second
        const statsInterval = setInterval(() => {
            messagesPerSec = (data.messages.length - lastCount) * 2;
            lastCount = data.messages.length;
            totalReceived = data.messages.length;
            alertsTriggered = data.messages.filter((m: any) => 
                m.payload.includes('"gas":') && parseInt(m.payload.match(/"gas":(\d+)/)?.[1] || '0') >= 80
            ).length;
        }, 500);

        return () => {
            clearInterval(interval);
            clearInterval(statsInterval);
        };
    });

    function getMessageType(topic: string, payload: string): 'anchor' | 'server' | 'alert' {
        if (payload.includes('"gas":')) {
            const gasMatch = payload.match(/"gas":(\d+)/);
            if (gasMatch && parseInt(gasMatch[1]) >= 80) {
                return 'alert';
            }
        }
        if (topic.startsWith('uwb/anchor')) return 'anchor';
        if (topic.startsWith('server/')) return 'server';
        return 'anchor';
    }

    function formatPayload(topic: string, payload: string): string {
        try {
            const data = JSON.parse(payload);
            if (topic.startsWith('uwb/anchor')) {
                const gasWarning = data.gas >= 80 ? ' ⚠ HIGH GAS' : '';
                return `{"tagId":${data.tagId},"distance":${data.distance},"gas":${data.gas}}${gasWarning}`;
            }
            if (topic === 'server/trilaterate') {
                return `Calculated position: {x: ${data.x}, y: ${data.y}}`;
            }
            if (topic === 'server/alert') {
                return `GAS ALERT: Worker W${data.tagId} (${data.fullName}) - ${data.gas} ppm detected!`;
            }
            return payload;
        } catch {
            return payload;
        }
    }

    function filterMessages(messages: any[]) {
        return messages.filter((msg: any) => {
            const type = getMessageType(msg.topic, msg.payload);
            
            if (showAlertsOnly && type !== 'alert') return false;
            if (!showAnchorMessages && type === 'anchor') return false;
            if (!showServerEvents && type === 'server') return false;
            
            if (tagFilter !== 'All tags') {
                const tagMatch = msg.payload.match(/"tagId":(\d+)/);
                if (tagMatch && tagMatch[1] !== tagFilter) return false;
            }
            
            return true;
        });
    }

    function clearMessages() {
        // This would need a server endpoint to actually clear
        // For now, just refresh
        invalidateAll();
    }

    function formatTime(timestamp: string): string {
        const date = new Date(timestamp);
        return date.toLocaleTimeString('en-US', { 
            hour12: false, 
            hour: '2-digit', 
            minute: '2-digit', 
            second: '2-digit',
            fractionalSecondDigits: 3
        });
    }

    $: filteredMessages = filterMessages(data.messages);
</script>

<svelte:head>
    <title>Debug Console - VERTEX</title>
</svelte:head>

<div class="debug-page">
    <div class="main-content">
        <div class="console-container">
            <div class="console-header">
                <div class="status">
                    <span class="status-dot"></span>
                    <span class="status-text">MQTT Connected</span>
                </div>
                <div class="broker-info">
                    Broker: localhost:1883
                </div>
                <div class="subscribed">
                    Subscribed: uwb/#
                </div>
                <button class="clear-btn" on:click={clearMessages}>Clear</button>
            </div>

            <div class="console">
                {#if filteredMessages.length === 0}
                    <p class="empty">No messages yet. Waiting for MQTT data...</p>
                {:else}
                    {#each filteredMessages as msg}
                        {@const type = getMessageType(msg.topic, msg.payload)}
                        <div class="message {type}">
                            <span class="time">[{formatTime(msg.timestamp)}]</span>
                            <span class="topic">{msg.topic}</span>
                            <span class="payload">{formatPayload(msg.topic, msg.payload)}</span>
                        </div>
                    {/each}
                {/if}
            </div>
        </div>
    </div>

    <div class="sidebar">
        <div class="filter-section">
            <h3>Filters</h3>
            
            <div class="filter-group">
                <label for="topicFilter">Topic Filter</label>
                <input 
                    type="text" 
                    id="topicFilter" 
                    bind:value={topicFilter}
                    placeholder="uwb/#"
                />
            </div>

            <div class="filter-group">
                <label>Show Only</label>
                <div class="checkbox-group">
                    <label class="checkbox">
                        <input type="checkbox" bind:checked={showAnchorMessages} />
                        <span class="checkmark anchor"></span>
                        Anchor messages
                    </label>
                    <label class="checkbox">
                        <input type="checkbox" bind:checked={showServerEvents} />
                        <span class="checkmark server"></span>
                        Server events
                    </label>
                    <label class="checkbox">
                        <input type="checkbox" bind:checked={showAlertsOnly} />
                        <span class="checkmark alert"></span>
                        Alerts only
                    </label>
                </div>
            </div>

            <div class="filter-group">
                <label for="tagFilter">Tag Filter</label>
                <select id="tagFilter" bind:value={tagFilter}>
                    <option>All tags</option>
                    <option>1</option>
                    <option>2</option>
                    <option>3</option>
                    <option>4</option>
                </select>
            </div>
        </div>

        <div class="stats-section">
            <h3>Stats</h3>
            <div class="stat">
                <span class="stat-label">Messages/sec:</span>
                <span class="stat-value green">{messagesPerSec.toFixed(1)}</span>
            </div>
            <div class="stat">
                <span class="stat-label">Total received:</span>
                <span class="stat-value">{totalReceived.toLocaleString()}</span>
            </div>
            <div class="stat">
                <span class="stat-label">Alerts triggered:</span>
                <span class="stat-value red">{alertsTriggered}</span>
            </div>
        </div>
    </div>
</div>

<style>
    .debug-page {
        display: flex;
        gap: 24px;
        padding: 24px;
        min-height: calc(100vh - 60px);
        background: #f1f5f9;
    }

    .main-content {
        flex: 1;
    }

    .console-container {
        background: white;
        border-radius: 12px;
        overflow: hidden;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
    }

    .console-header {
        background: #1e293b;
        padding: 12px 20px;
        display: flex;
        align-items: center;
        gap: 24px;
    }

    .status {
        display: flex;
        align-items: center;
        gap: 8px;
    }

    .status-dot {
        width: 10px;
        height: 10px;
        background: #22c55e;
        border-radius: 50%;
        animation: pulse 2s infinite;
    }

    @keyframes pulse {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.5; }
    }

    .status-text {
        color: #22c55e;
        font-weight: 600;
        font-size: 14px;
    }

    .broker-info, .subscribed {
        color: #94a3b8;
        font-size: 14px;
    }

    .clear-btn {
        margin-left: auto;
        background: #475569;
        color: white;
        border: none;
        padding: 8px 16px;
        border-radius: 6px;
        font-size: 13px;
        cursor: pointer;
        transition: background 0.2s;
    }

    .clear-btn:hover {
        background: #64748b;
    }

    .console {
        background: #0f172a;
        min-height: 500px;
        max-height: calc(100vh - 220px);
        overflow-y: auto;
        padding: 16px;
        font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
        font-size: 13px;
    }

    .empty {
        color: #64748b;
        font-style: italic;
    }

    .message {
        padding: 6px 0;
        border-bottom: 1px solid #1e293b;
        display: flex;
        gap: 12px;
        flex-wrap: wrap;
    }

    .message.anchor .topic {
        color: #22c55e;
    }

    .message.server .topic {
        color: #3b82f6;
    }

    .message.alert {
        background: rgba(239, 68, 68, 0.1);
    }

    .message.alert .topic {
        color: #ef4444;
    }

    .message.alert .payload {
        color: #fca5a5;
    }

    .time {
        color: #64748b;
    }

    .topic {
        color: #22c55e;
        min-width: 150px;
    }

    .payload {
        color: #f8fafc;
    }

    /* Sidebar */
    .sidebar {
        width: 280px;
        display: flex;
        flex-direction: column;
        gap: 20px;
    }

    .filter-section, .stats-section {
        background: white;
        border-radius: 12px;
        padding: 20px;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
    }

    h3 {
        margin: 0 0 16px 0;
        font-size: 16px;
        color: #1e293b;
    }

    .filter-group {
        margin-bottom: 16px;
    }

    .filter-group > label {
        display: block;
        margin-bottom: 8px;
        font-size: 13px;
        font-weight: 500;
        color: #64748b;
    }

    .filter-group input[type="text"],
    .filter-group select {
        width: 100%;
        padding: 10px 12px;
        border: 1px solid #e2e8f0;
        border-radius: 6px;
        font-size: 14px;
        background: #f8fafc;
    }

    .filter-group input:focus,
    .filter-group select:focus {
        outline: none;
        border-color: #3b82f6;
    }

    .checkbox-group {
        display: flex;
        flex-direction: column;
        gap: 10px;
    }

    .checkbox {
        display: flex;
        align-items: center;
        gap: 10px;
        font-size: 14px;
        color: #475569;
        cursor: pointer;
    }

    .checkbox input {
        display: none;
    }

    .checkmark {
        width: 18px;
        height: 18px;
        border: 2px solid #d1d5db;
        border-radius: 4px;
        position: relative;
    }

    .checkbox input:checked + .checkmark {
        border-color: transparent;
    }

    .checkbox input:checked + .checkmark.anchor {
        background: #22c55e;
    }

    .checkbox input:checked + .checkmark.server {
        background: #3b82f6;
    }

    .checkbox input:checked + .checkmark.alert {
        background: #ef4444;
    }

    .checkbox input:checked + .checkmark::after {
        content: '✓';
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        color: white;
        font-size: 12px;
        font-weight: bold;
    }

    .stat {
        display: flex;
        justify-content: space-between;
        padding: 8px 0;
        border-bottom: 1px solid #f1f5f9;
    }

    .stat:last-child {
        border-bottom: none;
    }

    .stat-label {
        color: #64748b;
        font-size: 14px;
    }

    .stat-value {
        font-weight: 600;
        font-size: 14px;
        color: #1e293b;
    }

    .stat-value.green {
        color: #22c55e;
    }

    .stat-value.red {
        color: #ef4444;
    }
</style>