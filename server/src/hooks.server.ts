import { redirect } from '@sveltejs/kit';
import { connectMQTT } from '$lib/server/mqtt';

// Connect to MQTT when server starts
connectMQTT();
console.log('🚀 Server hooks initialized');

// Set to false to disable login requirement (for development)
const REQUIRE_LOGIN = true;

// Protected routes that require authentication
const protectedRoutes = ['/', '/admin', '/debug'];

export async function handle({ event, resolve }) {
    // Skip auth check if login is disabled
    if (!REQUIRE_LOGIN) {
        return resolve(event);
    }

    const session = event.cookies.get('session');
    const isLoggedIn = session === 'authenticated';
    const path = event.url.pathname;

    // Check if this is a protected route
    const isProtectedRoute = protectedRoutes.some(route => 
        path === route || path.startsWith(route + '/')
    );

    // If trying to access protected route without login, redirect to login
    if (isProtectedRoute && !isLoggedIn) {
        throw redirect(303, '/login');
    }

    // If already logged in and trying to access login page, redirect to dashboard
    if (path === '/login' && isLoggedIn) {
        throw redirect(303, '/');
    }

    return resolve(event);
}