import { createServer } from "http";
import { Server } from "socket.io";
import app from "./app.js";
import { PORT, CORS_ORIGIN } from "./config/env.js";
import connectToDatabase from "./database/mongodb.js";
import { setIO } from "./realtime/socket.js";
import { startMqtt } from "./controllers/mqtt.controller.js";

const server = createServer(app);

const io = new Server(server, {
  cors: {
    origin: CORS_ORIGIN,
    methods: ["GET", "POST"]
  }
});

setIO(io);

io.on("connection", () => {
  console.log("A user connected");
});

server.listen(PORT, async () => {
  console.log(`🚀 Server running at http://localhost:${PORT}`);
  await connectToDatabase();
  startMqtt();
});

