import mqtt from 'mqtt';
import { MQTT_BROKER_URL } from '../config/env.js';
import { io } from '../app.js';
import { handleMeasurementMessage, handleStatusMessage } from './measurement.controller.js';

// Kết nối MQTT broker
const client = mqtt.connect(MQTT_BROKER_URL);

// Subscribe các topics
client.subscribe('pill/data/log');
client.subscribe('pill/data/status');
client.subscribe('pill/data/measurement');

// Event handlers
client.on('connect', () => {
  console.log('✅ Connected to MQTT broker');
});

client.on('message', async (topic, message) => {
  console.log(`📨 Received from ${topic}: ${message.toString()}`);
  
  // Route messages đến các handler tương ứng
  if (topic === 'pill/data/status') {
    handleStatusMessage(message.toString());
  } else if (topic === 'pill/data/measurement') {
    await handleMeasurementMessage(message.toString());
  }
  // pill/data/log có thể xử lý sau nếu cần
});

// Export MQTT client để các controller khác có thể publish
export { client as mqttClient };

