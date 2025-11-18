import express from "express";
import { spawn } from "child_process";

const router = express.Router();

// POST /api/predict
router.post("/", (req, res) => {
    const inputData = req.body;

    const py = spawn("python", ["ml/predict_from_json.py", JSON.stringify(inputData)]);

    let dataToSend = "";
    py.stdout.on("data", (data) => {
        dataToSend += data.toString();
    });

    py.stderr.on("data", (data) => {
        console.error(data.toString());
    });

    py.on("close", () => {
        try {
            res.json(JSON.parse(dataToSend));
        } catch (err) {
            res.status(500).send({ error: "Prediction failed" });
        }
    });
});

// ⚡ Đây là export default
export default router;
