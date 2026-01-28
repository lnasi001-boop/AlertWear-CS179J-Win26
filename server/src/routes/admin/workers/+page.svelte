<script lang="ts">
    import { onMount } from 'svelte';
    import { invalidateAll } from '$app/navigation';

    interface Worker {
        tagId: number;
        fullName: string;
        empId: string;
    }

    interface WorkerPosition {
        tagId: number;
        status: 'ok' | 'warning' | 'alert';
    }

    let workers: Worker[] = [];
    let activeWorkers: Map<number, WorkerPosition> = new Map();
    let loading = true;
    let showModal = false;
    let editingWorker: Worker | null = null;

    // Form fields
    let formTagId = 1;
    let formFullName = '';
    let formEmpId = '';

    onMount(async () => {
        await loadWorkers();
        // Refresh active status every 2 seconds
        const interval = setInterval(async () => {
            await loadActiveStatus();
        }, 2000);
        return () => clearInterval(interval);
    });

    async function loadWorkers() {
        loading = true;
        const res = await fetch('/api/workers');
        workers = await res.json();
        await loadActiveStatus();
        loading = false;
    }

    async function loadActiveStatus() {
        try {
            const res = await fetch('/api/workers/status');
            if (res.ok) {
                const positions = await res.json();
                activeWorkers = new Map(positions.map((p: WorkerPosition) => [p.tagId, p]));
            }
        } catch (e) {
            // Status endpoint might not exist yet
        }
    }

    function getWorkerStatus(tagId: number): { status: string; color: string } {
        const position = activeWorkers.get(tagId);
        if (!position) {
            return { status: 'Pending', color: 'orange' };
        }
        return { status: 'Active', color: 'green' };
    }

    function isUnassigned(worker: Worker): boolean {
        return !worker.fullName || worker.fullName === 'Unassigned';
    }

    function openAddModal() {
        editingWorker = null;
        formTagId = Math.max(...workers.map(w => w.tagId), 0) + 1;
        formFullName = '';
        formEmpId = '';
        showModal = true;
    }

    function openEditModal(worker: Worker) {
        editingWorker = worker;
        formTagId = worker.tagId;
        formFullName = worker.fullName;
        formEmpId = worker.empId;
        showModal = true;
    }

    function openAssignModal(worker: Worker) {
        editingWorker = worker;
        formTagId = worker.tagId;
        formFullName = '';
        formEmpId = '';
        showModal = true;
    }

    function closeModal() {
        showModal = false;
        editingWorker = null;
    }

    async function saveWorker() {
        const workerData = {
            tagId: formTagId,
            fullName: formFullName,
            empId: formEmpId
        };

        if (editingWorker) {
            await fetch('/api/workers', {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(workerData)
            });
        } else {
            await fetch('/api/workers', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(workerData)
            });
        }

        closeModal();
        await loadWorkers();
    }

    async function deleteWorker(tagId: number) {
        if (!confirm('Are you sure you want to delete this worker?')) return;

        await fetch('/api/workers', {
            method: 'DELETE',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ tagId })
        });

        await loadWorkers();
    }
</script>

<svelte:head>
    <title>Workers - Admin - VERTEX</title>
</svelte:head>

<div class="page">
    <div class="header">
        <div>
            <h1>Worker ↔ Tag Assignment</h1>
            <p>Assign hardcoded Tag IDs to workers</p>
        </div>
        <button class="btn-primary" on:click={openAddModal}>
            + Add Worker
        </button>
    </div>

    {#if loading}
        <div class="loading">Loading...</div>
    {:else}
        <div class="table-container">
            <table>
                <thead>
                    <tr>
                        <th>Tag ID</th>
                        <th>Full Name</th>
                        <th>Employee ID</th>
                        <th>Status</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody>
                    {#each workers as worker (worker.tagId)}
                        {@const status = getWorkerStatus(worker.tagId)}
                        {@const unassigned = isUnassigned(worker)}
                        <tr class:unassigned={unassigned}>
                            <td>
                                <span class="tag-badge" class:pending={unassigned}>{worker.tagId}</span>
                            </td>
                            <td class:unassigned-text={unassigned}>
                                {unassigned ? 'Unassigned' : worker.fullName}
                            </td>
                            <td>{unassigned ? '—' : worker.empId}</td>
                            <td>
                                <span class="status-badge {status.color}">
                                    <span class="status-dot"></span>
                                    {status.status}
                                </span>
                            </td>
                            <td class="actions">
                                {#if unassigned}
                                    <button class="btn-assign" on:click={() => openAssignModal(worker)}>
                                        Assign
                                    </button>
                                {:else}
                                    <button class="btn-edit" on:click={() => openEditModal(worker)}>
                                        Edit
                                    </button>
                                    <button class="btn-delete" on:click={() => deleteWorker(worker.tagId)}>
                                        Delete
                                    </button>
                                {/if}
                            </td>
                        </tr>
                    {:else}
                        <tr>
                            <td colspan="5" class="empty">No workers configured</td>
                        </tr>
                    {/each}
                </tbody>
            </table>
        </div>
    {/if}
</div>

{#if showModal}
    <div class="modal-overlay" on:click={closeModal}>
        <div class="modal" on:click|stopPropagation>
            <h2>{editingWorker ? (isUnassigned(editingWorker) ? 'Assign Worker' : 'Edit Worker') : 'Add Worker'}</h2>
            
            <div class="form-group">
                <label for="tagId">Tag ID</label>
                <input 
                    type="number" 
                    id="tagId" 
                    bind:value={formTagId}
                    disabled={editingWorker !== null}
                    min="1"
                />
            </div>

            <div class="form-group">
                <label for="fullName">Full Name</label>
                <input 
                    type="text" 
                    id="fullName" 
                    bind:value={formFullName}
                    placeholder="John Smith"
                />
            </div>

            <div class="form-group">
                <label for="empId">Employee ID</label>
                <input 
                    type="text" 
                    id="empId" 
                    bind:value={formEmpId}
                    placeholder="EMP-001"
                />
            </div>

            <div class="modal-actions">
                <button class="btn-secondary" on:click={closeModal}>Cancel</button>
                <button class="btn-primary" on:click={saveWorker}>
                    {editingWorker ? (isUnassigned(editingWorker) ? 'Assign' : 'Update') : 'Add'} Worker
                </button>
            </div>
        </div>
    </div>
{/if}

<style>
    .page {
        background: white;
        border-radius: 12px;
        padding: 24px;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
    }

    .header {
        display: flex;
        justify-content: space-between;
        align-items: flex-start;
        margin-bottom: 24px;
    }

    h1 {
        font-size: 24px;
        color: #1e293b;
        margin: 0 0 4px 0;
    }

    .header p {
        color: #64748b;
        font-size: 14px;
        margin: 0;
    }

    .btn-primary {
        background: #3b82f6;
        color: white;
        border: none;
        padding: 10px 20px;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 500;
        cursor: pointer;
        transition: background 0.2s;
    }

    .btn-primary:hover {
        background: #2563eb;
    }

    .btn-secondary {
        background: #e2e8f0;
        color: #475569;
        border: none;
        padding: 10px 20px;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 500;
        cursor: pointer;
    }

    .btn-secondary:hover {
        background: #cbd5e1;
    }

    .loading {
        text-align: center;
        padding: 40px;
        color: #64748b;
    }

    .table-container {
        overflow-x: auto;
    }

    table {
        width: 100%;
        border-collapse: collapse;
    }

    th, td {
        padding: 16px 20px;
        text-align: left;
        border-bottom: 1px solid #e2e8f0;
    }

    th {
        background: #f8fafc;
        font-weight: 600;
        color: #475569;
        font-size: 13px;
    }

    td {
        color: #1e293b;
        font-size: 15px;
    }

    tr.unassigned {
        background: #fefce8;
        border: 2px dashed #fbbf24;
    }

    .tag-badge {
        background: #3b82f6;
        color: white;
        padding: 6px 16px;
        border-radius: 20px;
        font-size: 14px;
        font-weight: 600;
        display: inline-block;
    }

    .tag-badge.pending {
        background: #fbbf24;
        color: #78350f;
    }

    .unassigned-text {
        color: #d97706;
        font-style: italic;
    }

    .status-badge {
        display: inline-flex;
        align-items: center;
        gap: 6px;
        padding: 4px 12px;
        border-radius: 20px;
        font-size: 13px;
        font-weight: 500;
    }

    .status-badge.green {
        background: #dcfce7;
        color: #16a34a;
    }

    .status-badge.orange {
        background: #fef3c7;
        color: #d97706;
    }

    .status-badge.red {
        background: #fee2e2;
        color: #dc2626;
    }

    .status-dot {
        width: 8px;
        height: 8px;
        border-radius: 50%;
        background: currentColor;
    }

    .actions {
        display: flex;
        gap: 8px;
    }

    .btn-edit {
        background: none;
        border: 1px solid #3b82f6;
        color: #3b82f6;
        padding: 6px 12px;
        border-radius: 4px;
        font-size: 13px;
        cursor: pointer;
    }

    .btn-edit:hover {
        background: #eff6ff;
    }

    .btn-delete {
        background: none;
        border: 1px solid #ef4444;
        color: #ef4444;
        padding: 6px 12px;
        border-radius: 4px;
        font-size: 13px;
        cursor: pointer;
    }

    .btn-delete:hover {
        background: #fef2f2;
    }

    .btn-assign {
        background: #3b82f6;
        border: none;
        color: white;
        padding: 6px 16px;
        border-radius: 4px;
        font-size: 13px;
        font-weight: 500;
        cursor: pointer;
    }

    .btn-assign:hover {
        background: #2563eb;
    }

    .empty {
        text-align: center;
        color: #64748b;
        font-style: italic;
    }

    /* Modal */
    .modal-overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background: rgba(0, 0, 0, 0.5);
        display: flex;
        align-items: center;
        justify-content: center;
        z-index: 200;
    }

    .modal {
        background: white;
        padding: 24px;
        border-radius: 12px;
        width: 100%;
        max-width: 400px;
    }

    .modal h2 {
        margin: 0 0 20px 0;
        font-size: 20px;
        color: #1e293b;
    }

    .form-group {
        margin-bottom: 16px;
    }

    .form-group label {
        display: block;
        margin-bottom: 6px;
        font-size: 14px;
        font-weight: 500;
        color: #475569;
    }

    .form-group input {
        width: 100%;
        padding: 10px 12px;
        border: 1px solid #d1d5db;
        border-radius: 6px;
        font-size: 14px;
    }

    .form-group input:focus {
        outline: none;
        border-color: #3b82f6;
        box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
    }

    .form-group input:disabled {
        background: #f1f5f9;
        color: #64748b;
    }

    .modal-actions {
        display: flex;
        justify-content: flex-end;
        gap: 12px;
        margin-top: 24px;
    }
</style>