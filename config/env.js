import { config } from "dotenv";

// Chỉ load dotenv khi đang chạy local (không phải production trên Vercel)
if (!process.env.VERCEL && process.env.NODE_ENV !== "production") {
  config({ path: `.env.${process.env.NODE_ENV || 'development'}.local` });
}

export const PORT = process.env.PORT || 3000;
export const MQTT_BROKER_URL = process.env.MQTT_BROKER_URL;
export const DB_URI = process.env.DB_URI;
export const NODE_ENV = process.env.NODE_ENV || 'development';
export const DEFAULT_USER_ID = process.env.DEFAULT_USER_ID || null;

// Debug log để kiểm tra trên Vercel (xem trong log)
if (!DB_URI) {
  console.error("❌ DB_URI is missing in environment variables!");
} else {
  console.log("✅ DB_URI loaded successfully");
}
