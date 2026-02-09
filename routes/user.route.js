import { Router } from "express";
import {
    register,
    login,
    createUser,
    getUsers,
    getUserById,
    updateUser,
    deleteUser
} from "../controllers/user.controller.js";
import { authenticateToken } from "../middleware/auth.middleware.js";

const userRouter = Router();

// Public routes
userRouter.post("/register", register);
userRouter.post("/login", login);

// Protected routes
userRouter.post("/", authenticateToken, createUser);
userRouter.get("/", authenticateToken, getUsers);
userRouter.get("/:userId", authenticateToken, getUserById);
userRouter.put("/:userId", authenticateToken, updateUser);
userRouter.delete("/:userId", authenticateToken, deleteUser);

export default userRouter;

