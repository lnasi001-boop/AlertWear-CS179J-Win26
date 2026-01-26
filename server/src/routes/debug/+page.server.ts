import { messages } from '$lib/server/mqtt';

export function load() {
    return {
        messages: messages.slice(0, 50) // Return last 50 messages
    };
}