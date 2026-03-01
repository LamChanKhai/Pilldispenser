import { config } from "dotenv";

// Load dotenv: local dev hoặc production trên VPS (không load trên Vercel)
if (!process.env.VERCEL) {
  const envFile = `.env.${process.env.NODE_ENV || 'development'}.local`;
  config({ path: envFile });
}

export const PORT = process.env.PORT || 3000;
export const MQTT_BROKER_URL = process.env.MQTT_BROKER_URL;
export const DB_URI = process.env.DB_URI;
export const NODE_ENV = process.env.NODE_ENV || 'development';

export const DEFAULT_USER_ID = process.env.DEFAULT_USER_ID || null;

export const GEMINI_API_KEY = process.env.GEMINI_API_KEY;
export const GEMINI_MODEL = process.env.GEMINI_MODEL;

export const JWT_SECRET = process.env.JWT_SECRET;

/** Origin cho Socket.IO CORS - dùng khi deploy (VD: http://your-domain.com hoặc http://IP:3000) */
export const CORS_ORIGIN = process.env.CORS_ORIGIN || "http://localhost:3000";

// Debug log để kiểm tra trên Vercel (xem trong log)
if (!DB_URI) {
  console.error("❌ DB_URI is missing in environment variables!");
} else {
  console.log("✅ DB_URI loaded successfully");
}
