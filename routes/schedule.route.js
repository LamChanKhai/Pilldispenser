import {Router} from "express";
import { setSchedule, refill } from "../controllers/schedule.controller.js";

const scheduleRouter = Router();

scheduleRouter.post("/", setSchedule);
scheduleRouter.get("/refill", refill);

export default scheduleRouter;
