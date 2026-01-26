<script lang="ts">
    import { page } from '$app/stores';
    
    $: isLoginPage = $page.url.pathname === '/login';
</script>

{#if isLoginPage}
    <slot />
{:else}
    <div class="app">
        <nav class="navbar">
            <div class="nav-brand">
                <span class="logo">🔒</span>
                <span class="brand-name">AlertWear</span>
            </div>
            
            <div class="nav-links">
                <a href="/" class:active={$page.url.pathname === '/'}>
                    Dashboard
                </a>
                <a href="/admin/workers" class:active={$page.url.pathname.startsWith('/admin')}>
                    Admin
                </a>
                <a href="/debug" class:active={$page.url.pathname === '/debug'}>
                    Debug
                </a>
            </div>

            <div class="nav-user">
                <a href="/login" class="logout-btn">Logout</a>
            </div>
        </nav>

        <main>
            <slot />
        </main>
    </div>
{/if}

<style>
    :global(*) {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
    }

    :global(body) {
        font-family: system-ui, -apple-system, sans-serif;
        background: #f1f5f9;
    }

    .app {
        min-height: 100vh;
        display: flex;
        flex-direction: column;
    }

    .navbar {
        background: #1e293b;
        color: white;
        padding: 0 24px;
        height: 60px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        position: sticky;
        top: 0;
        z-index: 100;
    }

    .nav-brand {
        display: flex;
        align-items: center;
        gap: 10px;
    }

    .logo {
        font-size: 24px;
    }

    .brand-name {
        font-size: 20px;
        font-weight: bold;
    }

    .nav-links {
        display: flex;
        gap: 8px;
    }

    .nav-links a {
        color: #94a3b8;
        text-decoration: none;
        padding: 8px 16px;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 500;
        transition: all 0.2s;
    }

    .nav-links a:hover {
        color: white;
        background: #334155;
    }

    .nav-links a.active {
        color: white;
        background: #3b82f6;
    }

    .nav-user {
        display: flex;
        align-items: center;
    }

    .logout-btn {
        background: #334155;
        padding: 8px 16px;
        border-radius: 6px;
        font-size: 13px;
        color: #94a3b8;
        text-decoration: none;
        transition: all 0.2s;
    }

    .logout-btn:hover {
        background: #475569;
        color: white;
    }

    main {
        flex: 1;
    }
</style>