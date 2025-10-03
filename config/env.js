import { config } from "dotenv";

// Chỉ load dotenv khi không phải production
if (process.env.NODE_ENV !== "production") {
  config({ path: `.env.${process.env.NODE_ENV || 'development'}.local` });
}

// Lấy biến môi trường
export const PORT = process.env.PORT || 3000;

export const MQTT_BROKER_URL = process.env.MQTT_BROKER_URL;
