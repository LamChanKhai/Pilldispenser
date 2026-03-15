import mqtt from 'mqtt';
import { MQTT_BROKER_URL } from '../config/env.js';
import { emit } from "../realtime/socket.js";
import { handleMeasurementMessage, handleStatusMessage } from './measurement.controller.js';
import scheduleModel from '../model/schedule.model.js';

let mqttClient = null;

const ALLOWED_STATUSES = ['completed', 'late', 'skipped'];

/**
 * Xử lý message từ ESP32:
 * - Bấm đúng giờ → completed; bấm trễ 5 phút → late; ô không bấm (đến giờ ô sau mới bấm) → skipped
 * Payload: { "userId": "...", "updates": [ { "index": 0, "status": "skipped" }, { "index": 1, "status": "completed" } ] }
 */
async function handleScheduleMessage(payload) {
  try {
    const data = JSON.parse(payload);
    const { userId, updates } = data;
    if (userId == null) {
      console.warn('⚠️ pill/data/schedule: invalid payload', data);
      return;
    }
    const list = Array.isArray(updates) && updates.length > 0
      ? updates
      : (data.index != null && data.status != null ? [{ index: data.index, status: data.status }] : null);
    if (!list) {
      console.warn('⚠️ pill/data/schedule: need updates[] or index+status', data);
      return;
    }
    const schedule = await scheduleModel
      .findOne({ userId })
      .sort({ createdAt: -1 })
      .lean();
    if (!schedule) {
      console.warn('⚠️ pill/data/schedule: no schedule for userId', userId);
      return;
    }
    const setFields = {};
    for (const u of list) {
      const idx = u.index;
      const status = u.status;
      if (idx == null || idx < 0 || idx > 13 || !ALLOWED_STATUSES.includes(status)) continue;
      setFields[`status_Array.${idx}`] = status;
    }
    if (Object.keys(setFields).length === 0) return;
    await scheduleModel.findOneAndUpdate(
      { _id: schedule._id },
      { $set: setFields }
    );
    console.log(`✅ Schedule updated: userId=${userId}`, Object.entries(setFields).map(([k, v]) => `${k}=${v}`).join(', '));
  } catch (err) {
    console.error('handleScheduleMessage error:', err);
  }
}

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
  mqttClient.subscribe("pill/data/schedule");

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
    } else if (topic === "pill/data/schedule") {
      await handleScheduleMessage(message.toString());
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

