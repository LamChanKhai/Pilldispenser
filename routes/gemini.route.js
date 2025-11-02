// routes/gemini.route.js
import express from "express";
import { processGemini } from "../controllers/gemini.controller.js";

const router = express.Router();

router.post("/", processGemini);

export default router;
