import express from "express";
import { spawn } from "child_process";

const router = express.Router();

router.post("/", (req, res) => {
    const inputData = req.body;

    const py = spawn(
        "ml/env_kltn/Scripts/python.exe",
        ["ml/predict_ann.py", JSON.stringify(inputData)]
    );

    let output = "";

    py.stdout.on("data", (data) => {
        output += data.toString();
    });

    py.stderr.on("data", (data) => {
        console.warn("Python warning:", data.toString());
    });

    py.on("close", () => {
        try {
            res.json(JSON.parse(output));
        } catch (err) {
            res.status(500).json({
                error: "Prediction failed",
                raw: output
            });
        }
    });
});

export default router;
