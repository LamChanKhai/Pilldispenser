import express from "express";
import { spawn } from "child_process";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const router = express.Router();

router.post("/", (req, res) => {
    const inputData = req.body;

    // Sử dụng "python" hoặc "python3" (tùy hệ thống)
    // Windows thường dùng "python", Linux/Mac dùng "python3"
    const pythonCmd = process.platform === "win32" ? "python" : "python3";
    const scriptPath = path.join(__dirname, "..", "ml", "predict_ann.py");
    
    // Chuẩn hóa JSON string để tránh lỗi escape trên Windows
    const jsonString = JSON.stringify(inputData);
    
    const py = spawn(
        pythonCmd,
        [scriptPath, jsonString],
        { 
            cwd: path.join(__dirname, ".."),
            shell: false // Không dùng shell để tránh escape issues
        }
    );

    let output = "";

    py.stdout.on("data", (data) => {
        output += data.toString();
    });

    py.stderr.on("data", (data) => {
        console.warn("Python warning:", data.toString());
    });

    py.on("close", (code) => {
        try {
            const result = JSON.parse(output);
            res.json(result);
        } catch (parseError) {
            res.status(500).json({
                error: "Prediction failed",
                raw: output,
                exitCode: code
            });
        }
    });
});

export default router;
