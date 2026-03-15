import {Router} from "express";
import { setSchedule, refill, getLatestScheduleByUser } from "../controllers/schedule.controller.js";

const scheduleRouter = Router();

scheduleRouter.post("/", setSchedule);
scheduleRouter.get("/refill", refill);
scheduleRouter.get("/latest", getLatestScheduleByUser);

export default scheduleRouter;
