<script lang="ts">
    import { onMount, afterUpdate } from 'svelte';

    export let workers: Array<{
        tagId: number;
        fullName: string;
        x: number;
        y: number;
        status: 'ok' | 'warning' | 'alert';
    }> = [];

    export let anchors: Array<{
        anchorId: string;
        name: string;
        x: number;
        y: number;
        online: boolean;
    }> = [];

    let canvas: HTMLCanvasElement;
    let ctx: CanvasRenderingContext2D;

    const CANVAS_SIZE = 500;
    const GRID_SIZE = 10; // 10m x 10m
    const SCALE = CANVAS_SIZE / GRID_SIZE;
    const PADDING = 40;

    const statusColors = {
        ok: '#22c55e',
        warning: '#f59e0b',
        alert: '#ef4444'
    };

    function draw() {
        if (!ctx) return;

        // Clear canvas
        ctx.fillStyle = '#1e293b';
        ctx.fillRect(0, 0, CANVAS_SIZE + PADDING * 2, CANVAS_SIZE + PADDING * 2);

        // Draw grid
        ctx.strokeStyle = '#334155';
        ctx.lineWidth = 1;
        for (let i = 0; i <= GRID_SIZE; i++) {
            const pos = i * SCALE + PADDING;
            // Vertical lines
            ctx.beginPath();
            ctx.moveTo(pos, PADDING);
            ctx.lineTo(pos, CANVAS_SIZE + PADDING);
            ctx.stroke();
            // Horizontal lines
            ctx.beginPath();
            ctx.moveTo(PADDING, pos);
            ctx.lineTo(CANVAS_SIZE + PADDING, pos);
            ctx.stroke();
        }

        // Draw axis labels
        ctx.fillStyle = '#64748b';
        ctx.font = '12px system-ui';
        ctx.textAlign = 'center';
        for (let i = 0; i <= GRID_SIZE; i += 2) {
            ctx.fillText(`${i}m`, i * SCALE + PADDING, CANVAS_SIZE + PADDING + 20);
        }
        ctx.textAlign = 'right';
        for (let i = 0; i <= GRID_SIZE; i += 2) {
            ctx.fillText(`${i}m`, PADDING - 8, CANVAS_SIZE + PADDING - i * SCALE + 4);
        }

        // Draw anchors (blue squares)
        anchors.forEach((anchor) => {
            const x = anchor.x * SCALE + PADDING;
            const y = CANVAS_SIZE + PADDING - anchor.y * SCALE; // Flip Y axis

            ctx.fillStyle = anchor.online ? '#3b82f6' : '#64748b';
            ctx.fillRect(x - 12, y - 12, 24, 24);

            ctx.fillStyle = '#ffffff';
            ctx.font = 'bold 10px system-ui';
            ctx.textAlign = 'center';
            ctx.fillText(anchor.anchorId, x, y + 4);
        });

        // Draw workers (colored circles)
        workers.forEach((worker) => {
            const x = worker.x * SCALE + PADDING;
            const y = CANVAS_SIZE + PADDING - worker.y * SCALE; // Flip Y axis

            // Outer circle (status color)
            ctx.beginPath();
            ctx.arc(x, y, 18, 0, Math.PI * 2);
            ctx.fillStyle = statusColors[worker.status];
            ctx.fill();

            // Inner circle
            ctx.beginPath();
            ctx.arc(x, y, 14, 0, Math.PI * 2);
            ctx.fillStyle = '#1e293b';
            ctx.fill();

            // Label
            ctx.fillStyle = '#ffffff';
            ctx.font = 'bold 10px system-ui';
            ctx.textAlign = 'center';
            ctx.fillText(`W${worker.tagId}`, x, y + 4);
        });
    }

    onMount(() => {
        ctx = canvas.getContext('2d')!;
        draw();
    });

    // Redraw whenever data changes
    afterUpdate(() => {
        if (ctx) draw();
    });
</script>

<div class="map-container">
    <canvas
        bind:this={canvas}
        width={CANVAS_SIZE + PADDING * 2}
        height={CANVAS_SIZE + PADDING * 2}
    />
    <div class="legend">
        <div class="legend-item">
            <div class="anchor-icon"></div>
            <span>Anchor</span>
        </div>
        <div class="legend-item">
            <div class="worker-icon ok"></div>
            <span>OK</span>
        </div>
        <div class="legend-item">
            <div class="worker-icon warning"></div>
            <span>Warning</span>
        </div>
        <div class="legend-item">
            <div class="worker-icon alert"></div>
            <span>Alert</span>
        </div>
    </div>
</div>

<style>
    .map-container {
        display: flex;
        flex-direction: column;
        gap: 16px;
    }

    canvas {
        border-radius: 8px;
    }

    .legend {
        display: flex;
        gap: 24px;
        justify-content: center;
    }

    .legend-item {
        display: flex;
        align-items: center;
        gap: 8px;
        font-size: 14px;
        color: #94a3b8;
    }

    .anchor-icon {
        width: 16px;
        height: 16px;
        background: #3b82f6;
        border-radius: 2px;
    }

    .worker-icon {
        width: 16px;
        height: 16px;
        border-radius: 50%;
    }

    .worker-icon.ok {
        background: #22c55e;
    }

    .worker-icon.warning {
        background: #f59e0b;
    }

    .worker-icon.alert {
        background: #ef4444;
    }
</style>