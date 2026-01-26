<script lang="ts">
    import { page } from '$app/stores';
</script>

<div class="admin">
    <div class="admin-nav">
        <a href="/admin/workers" class:active={$page.url.pathname === '/admin/workers'}>
            Workers
        </a>
        <a href="/admin/anchors" class:active={$page.url.pathname === '/admin/anchors'}>
            Anchors
        </a>
    </div>

    <div class="admin-content">
        <slot />
    </div>
</div>

<style>
    .admin {
        min-height: calc(100vh - 60px);
        background: #f1f5f9;
    }

    .admin-nav {
        display: flex;
        gap: 4px;
        padding: 16px 24px;
        background: white;
        border-bottom: 1px solid #e2e8f0;
    }

    .admin-nav a {
        color: #64748b;
        text-decoration: none;
        padding: 8px 20px;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 500;
        transition: all 0.2s;
    }

    .admin-nav a:hover {
        color: #1e293b;
        background: #f1f5f9;
    }

    .admin-nav a.active {
        color: #3b82f6;
        background: #eff6ff;
    }

    .admin-content {
        padding: 24px;
        max-width: 1200px;
        margin: 0 auto;
    }
</style>