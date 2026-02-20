import mqtt from 'mqtt';
import { processReading } from './store';

// Store for debug messages
export const messages: { timestamp: string; topic: string; payload: string }[] = [];

// MQTT Client
let client: mqtt.MqttClient | null = null;

export function connectMQTT() {
    if (client) return client;

    client = mqtt.connect('mqtt://172.20.10.2:1883'); //changed localhost to IP (Mari)

    client.on('connect', () => {
        console.log('✅ Connected to MQTT broker');
        client!.subscribe('uwb/#', (err) => {
            if (err) {
                console.error('Subscribe error:', err);
            } else {
                console.log('✅ Subscribed to all topics (uwb/#)');
            }
        });
    });

    client.on('message', (topic, payload) => {
        const payloadStr = payload.toString();
        
        // Store for debug console
        const msg = {
            timestamp: new Date().toISOString(),
            topic,
            payload: payloadStr
        };
        messages.unshift(msg);
        if (messages.length > 100) {
            messages.pop();
        }

        // Process UWB data
        if (topic.startsWith('uwb/')) {
            try {
                const data = JSON.parse(payloadStr);
                processReading(data);
            } catch (e) {
                console.error('Failed to parse MQTT message:', e);
            }
        }
    });

    client.on('error', (err) => {
        console.error('MQTT error:', err);
    });

    return client;
}