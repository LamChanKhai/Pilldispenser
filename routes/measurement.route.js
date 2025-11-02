import {Router} from "express";
import { getMeasurement, getMeasurements, saveMeasurement} from "../controllers/measurement.controller.js";

const measurementRouter = Router();

measurementRouter.get("/:userId", getMeasurement);
measurementRouter.post("/", saveMeasurement);
measurementRouter.get("/all/:userId", getMeasurements);
export default measurementRouter;