<script lang="ts">
    import { onMount } from 'svelte';

    interface Anchor {
        anchorId: string;
        name: string;
        x: number;
        y: number;
        online?: boolean;
        lastSeen?: string;
    }

    let anchors: Anchor[] = [];
    let loading = true;
    let showModal = false;
    let editingAnchor: Anchor | null = null;

    // Form fields
    let formAnchorId = '';
    let formName = '';
    let formX = 0;
    let formY = 0;

    onMount(async () => {
        await loadAnchors();
        // Refresh status every 2 seconds
        const interval = setInterval(async () => {
            await loadAnchors();
        }, 2000);
        return () => clearInterval(interval);
    });

    async function loadAnchors() {
        const wasLoading = loading;
        if (wasLoading) loading = true;
        
        const res = await fetch('/api/anchors');
        const data = await res.json();
        
        // Try to get live status
        try {
            const statusRes = await fetch('/api/anchors/status');
            if (statusRes.ok) {
                const statusData = await statusRes.json();
                const statusMap = new Map(statusData.map((a: Anchor) => [a.anchorId, a]));
                anchors = data.map((a: Anchor) => ({
                    ...a,
                    online: statusMap.get(a.anchorId)?.online ?? false,
                    lastSeen: statusMap.get(a.anchorId)?.lastSeen
                }));
            } else {
                anchors = data;
            }
        } catch {
            anchors = data;
        }
        
        loading = false;
    }

    function getAnchorStatus(anchor: Anchor): { status: string; color: string } {
        if (anchor.online) {
            return { status: 'Online', color: 'green' };
        }
        return { status: 'Offline', color: 'gray' };
    }

    function openAddModal() {
        editingAnchor = null;
        formAnchorId = `A${anchors.length + 1}`;
        formName = '';
        formX = 0;
        formY = 0;
        showModal = true;
    }

    function openEditModal(anchor: Anchor) {
        editingAnchor = anchor;
        formAnchorId = anchor.anchorId;
        formName = anchor.name;
        formX = anchor.x;
        formY = anchor.y;
        showModal = true;
    }

    function closeModal() {
        showModal = false;
        editingAnchor = null;
    }

    async function saveAnchor() {
        const anchorData = {
            anchorId: formAnchorId,
            name: formName,
            x: formX,
            y: formY
        };

        if (editingAnchor) {
            await fetch('/api/anchors', {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(anchorData)
            });
        } else {
            await fetch('/api/anchors', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(anchorData)
            });
        }

        closeModal();
        await loadAnchors();
    }

    async function deleteAnchor(anchorId: string) {
        if (!confirm('Are you sure you want to delete this anchor?')) return;

        await fetch('/api/anchors', {
            method: 'DELETE',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ anchorId })
        });

        await loadAnchors();
    }
</script>

<svelte:head>
    <title>Anchors - Admin - VERTEX</title>
</svelte:head>

<div class="page">
    <div class="header">
        <div>
            <h1>Anchor Management</h1>
            <p>Configure anchor positions in the facility</p>
        </div>
        <button class="btn-primary" on:click={openAddModal}>
            + Add Anchor
        </button>
    </div>

    {#if loading}
        <div class="loading">Loading...</div>
    {:else}
        <div class="table-container">
            <table>
                <thead>
                    <tr>
                        <th>Anchor ID</th>
                        <th>Name</th>
                        <th>X Position (m)</th>
                        <th>Y Position (m)</th>
                        <th>Status</th>
                        <th>Actions</th>
                    </tr>
                </thead>
                <tbody>
                    {#each anchors as anchor (anchor.anchorId)}
                        {@const status = getAnchorStatus(anchor)}
                        <tr>
                            <td><span class="anchor-badge">{anchor.anchorId}</span></td>
                            <td>{anchor.name}</td>
                            <td>{anchor.x} m</td>
                            <td>{anchor.y} m</td>
                            <td>
                                <span class="status-badge {status.color}">
                                    <span class="status-dot"></span>
                                    {status.status}
                                </span>
                            </td>
                            <td class="actions">
                                <button class="btn-edit" on:click={() => openEditModal(anchor)}>
                                    Edit
                                </button>
                                <button class="btn-delete" on:click={() => deleteAnchor(anchor.anchorId)}>
                                    Delete
                                </button>
                            </td>
                        </tr>
                    {:else}
                        <tr>
                            <td colspan="6" class="empty">No anchors configured</td>
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
            <h2>{editingAnchor ? 'Edit Anchor' : 'Add Anchor'}</h2>
            
            <div class="form-group">
                <label for="anchorId">Anchor ID</label>
                <input 
                    type="text" 
                    id="anchorId" 
                    bind:value={formAnchorId}
                    disabled={editingAnchor !== null}
                    placeholder="A1"
                />
            </div>

            <div class="form-group">
                <label for="name">Name</label>
                <input 
                    type="text" 
                    id="name" 
                    bind:value={formName}
                    placeholder="North Corner"
                />
            </div>

            <div class="form-row">
                <div class="form-group">
                    <label for="x">X Position (m)</label>
                    <input 
                        type="number" 
                        id="x" 
                        bind:value={formX}
                        min="0"
                        max="100"
                        step="0.1"
                    />
                </div>

                <div class="form-group">
                    <label for="y">Y Position (m)</label>
                    <input 
                        type="number" 
                        id="y" 
                        bind:value={formY}
                        min="0"
                        max="100"
                        step="0.1"
                    />
                </div>
            </div>

            <div class="modal-actions">
                <button class="btn-secondary" on:click={closeModal}>Cancel</button>
                <button class="btn-primary" on:click={saveAnchor}>
                    {editingAnchor ? 'Update' : 'Add'} Anchor
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

    .anchor-badge {
        background: #3b82f6;
        color: white;
        padding: 6px 16px;
        border-radius: 4px;
        font-size: 14px;
        font-weight: 600;
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

    .status-badge.gray {
        background: #f1f5f9;
        color: #64748b;
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

    .form-row {
        display: flex;
        gap: 16px;
    }

    .form-row .form-group {
        flex: 1;
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