import express from "express";
import bodyParser from "body-parser";
import path from "path";
import { fileURLToPath } from "url";
import scheduleRouter from "./routes/schedule.route.js";
import measurementRouter from "./routes/measurement.route.js";
import userRouter from "./routes/user.route.js";
import geminiRouter from "./routes/gemini.route.js";
import connectToDatabase from "./database/mongodb.js";
import predictRoute from "./routes/predict.route.js";

// Fix __dirname trong ES module
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();

// Middleware
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, "public")));

// Ensure MongoDB is connected before hitting API routes (serverless-friendly)
app.use("/api", async (req, res, next) => {
  try {
    await connectToDatabase();
    next();
  } catch (error) {
    console.error("Failed to connect to MongoDB", error);
    res.status(500).json({ message: "Database connection failed" });
  }
});

// Routes
app.use("/api/v1/schedule", scheduleRouter);
app.use("/api/v1/measurement", measurementRouter);
app.use("/api/v1/user", userRouter);

app.use("/api/v1/gemini", geminiRouter);

app.use("/api/predict", predictRoute);

export default app;
