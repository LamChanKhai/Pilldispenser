import express from "express";
import bodyParser from "body-parser";
import path from "path";
import { fileURLToPath } from "url";
import { createServer } from "http";
import scheduleRouter from "./routes/schedule.route.js";
import measurementRouter from "./routes/measurement.route.js";
import userRouter from "./routes/user.route.js";
import { PORT } from "./config/env.js";
import { Server } from "socket.io";
import connecToDatabase from "./database/mongodb.js";
// Fix __dirname trong ES module
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const server = createServer(app);

export const io = new Server(server, {
  cors: {
    origin: "http://localhost:3000",
    methods: ["GET", "POST"]
  }
});

// Middleware
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, "public")));

io.on("connection", (socket) => {
  console.log("A user connected");

});


// Routes
app.use("/api/v1/schedule", scheduleRouter);
app.use("/api/v1/measurement", measurementRouter);
app.use("/api/v1/user", userRouter);

// Start server
server.listen(PORT, async () => {
  console.log(`🚀 Server running at http://localhost:${PORT}`);
  await connecToDatabase();
});

export default app;