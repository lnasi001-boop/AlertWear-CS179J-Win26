<script lang="ts">
    export let worker: {
        tagId: number;
        fullName: string;
        empId: string;
        x: number;
        y: number;
        gas: number;
        status: 'ok' | 'warning' | 'alert';
    };

    const statusLabels = {
        ok: 'OK',
        warning: 'Warning',
        alert: 'GAS ALERT'
    };
</script>

<div class="card {worker.status}">
    <div class="header">
        <div class="avatar {worker.status}">W{worker.tagId}</div>
        <div class="info">
            <div class="name">{worker.fullName}</div>
            <div class="emp-id">{worker.empId}</div>
        </div>
    </div>
    <div class="details">
        <div class="detail">
            <span class="label">Position</span>
            <span class="value">({worker.x}, {worker.y})</span>
        </div>
        <div class="detail">
            <span class="label">Gas Level</span>
            <span class="value gas-{worker.status}">{worker.gas} ppm</span>
        </div>
        <div class="detail">
            <span class="label">Status</span>
            <span class="status-badge {worker.status}">{statusLabels[worker.status]}</span>
        </div>
    </div>
</div>

<style>
    .card {
        background: #ffffff;
        border-radius: 8px;
        padding: 16px;
        border-left: 4px solid #22c55e;
    }

    .card.warning {
        border-left-color: #f59e0b;
        background: #fffbeb;
    }

    .card.alert {
        border-left-color: #ef4444;
        background: #fef2f2;
        animation: pulse 1s infinite;
    }

    @keyframes pulse {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.8; }
    }

    .header {
        display: flex;
        align-items: center;
        gap: 12px;
        margin-bottom: 12px;
    }

    .avatar {
        width: 40px;
        height: 40px;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
        font-weight: bold;
        font-size: 14px;
        color: white;
        background: #22c55e;
    }

    .avatar.warning {
        background: #f59e0b;
    }

    .avatar.alert {
        background: #ef4444;
    }

    .info {
        flex: 1;
    }

    .name {
        font-weight: 600;
        color: #1e293b;
    }

    .emp-id {
        font-size: 12px;
        color: #64748b;
    }

    .details {
        display: flex;
        flex-direction: column;
        gap: 8px;
    }

    .detail {
        display: flex;
        justify-content: space-between;
        font-size: 13px;
    }

    .label {
        color: #64748b;
    }

    .value {
        color: #1e293b;
        font-weight: 500;
    }

    .gas-warning {
        color: #f59e0b;
    }

    .gas-alert {
        color: #ef4444;
        font-weight: bold;
    }

    .status-badge {
        padding: 2px 8px;
        border-radius: 12px;
        font-size: 11px;
        font-weight: 600;
    }

    .status-badge.ok {
        background: #dcfce7;
        color: #16a34a;
    }

    .status-badge.warning {
        background: #fef3c7;
        color: #d97706;
    }

    .status-badge.alert {
        background: #fee2e2;
        color: #dc2626;
    }
</style>