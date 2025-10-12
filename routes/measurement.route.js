import {Router} from "express";
import { getMeasurements, saveMeasurement} from "../controllers/measurement.controller.js";

const measurementRouter = Router();

measurementRouter.get("/:userId", getMeasurements);
measurementRouter.post("/", saveMeasurement);

export default measurementRouter;