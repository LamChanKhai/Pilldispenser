import { Router } from "express";
import {
    createUser,
    getUsers,
    getUserById,
    updateUser,
    deleteUser
} from "../controllers/user.controller.js";

const userRouter = Router();

userRouter.post("/", createUser);
userRouter.get("/", getUsers);
userRouter.get("/:userId", getUserById);
userRouter.put("/:userId", updateUser);
userRouter.delete("/:userId", deleteUser);

export default userRouter;

