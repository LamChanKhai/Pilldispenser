import mqtt from 'mqtt';
import { MQTT_BROKER_URL } from '../config/env.js';
import { emit } from "../realtime/socket.js";
import { handleMeasurementMessage, handleStatusMessage } from './measurement.controller.js';

let mqttClient = null;

/**
 * Start MQTT connection (intended for long-running servers only).
 * Do NOT call this from serverless (Vercel) handlers.
 */
export function startMqtt() {
  if (mqttClient) return mqttClient;
  if (!MQTT_BROKER_URL) {
    console.warn("⚠️ MQTT_BROKER_URL is missing; MQTT will not start");
    return null;
  }

  // Kết nối MQTT broker
  mqttClient = mqtt.connect(MQTT_BROKER_URL);

  // Subscribe các topics
  mqttClient.subscribe("pill/data/log");
  mqttClient.subscribe("pill/data/status");
  mqttClient.subscribe("pill/data/measurement");

  // Event handlers
  mqttClient.on("connect", () => {
    console.log("✅ Connected to MQTT broker");
  });

  mqttClient.on("message", async (topic, message) => {
    console.log(`📨 Received from ${topic}: ${message.toString()}`);

    // Route messages đến các handler tương ứng
    if (topic === "pill/data/status") {
      handleStatusMessage(message.toString());
    } else if (topic === "pill/data/measurement") {
      await handleMeasurementMessage(message.toString());
    }
    // pill/data/log có thể xử lý sau nếu cần
  });

  // Example: emit mqtt status to frontend if socket exists
  mqttClient.on("error", (err) => {
    console.error("❌ MQTT error:", err);
    emit("mqtt/error", { message: err?.message || String(err) });
  });

  return mqttClient;
}

export { mqttClient };

