import {Router} from "express";
import { setSchedule } from "../controllers/schedule.controller.js";

const scheduleRouter = Router();

scheduleRouter.post("/", setSchedule);


export default scheduleRouter;
