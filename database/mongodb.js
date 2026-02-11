import mongoose from "mongoose";
import { DB_URI , NODE_ENV } from "../config/env.js";

/**
 * Serverless-friendly MongoDB connection helper.
 * - Caches the connection/promise across invocations (Vercel).
 * - Ensures queries don't "buffer timeout" because connect wasn't called.
 */

const globalForMongoose = globalThis;

const cached =
  globalForMongoose.__mongooseCached ||
  (globalForMongoose.__mongooseCached = { conn: null, promise: null });

const connectToDatabase = async () => {
  if (!DB_URI) {
    throw new Error("DB_URI is not defined in environment variables");
  }

  if (cached.conn) return cached.conn;

  if (!cached.promise) {
    cached.promise = mongoose
      .connect(DB_URI, {
        // Fail faster in serverless so requests don't hang too long
        serverSelectionTimeoutMS: 10_000
      })
      .then((m) => m);
  }

  cached.conn = await cached.promise;
  console.log(`Connected to MongoDB in ${NODE_ENV} mode`);
  return cached.conn;
};

export default connectToDatabase;