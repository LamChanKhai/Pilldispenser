import {Router} from "express";
import { saveBp, saveSpo2, getBp, getSpo2 } from "../controllers/measurement.controller.js";

const measurementRouter = Router();

measurementRouter.post("/bp", saveBp);
measurementRouter.post("/spo2", saveSpo2);
measurementRouter.get("/bp/:userId", getBp);
measurementRouter.get("/spo2/:userId", getSpo2);
export default measurementRouter;