// controllers/gemini.controller.js
import fetch from "node-fetch";
import { GEMINI_API_KEY, GEMINI_MODEL } from "../config/env.js";

export const processGemini = async (req, res) => {
    const userText = req.body.text?.trim() || "";

    if (!userText) {
        return res.status(400).json({ error: "Thiếu nội dung văn bản để xử lý." });
    }

    try {
        const response = await fetch(
            `https://generativelanguage.googleapis.com/v1beta/models/${GEMINI_MODEL}:generateContent?key=${GEMINI_API_KEY}`,
            {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    contents: [
                        {
                            role: "user",
                            parts: [
                                {
                                    text: `
Bạn là **trợ lý y tế thông minh** có nhiệm vụ hỗ trợ chia thuốc vào hộp thuốc có 14 ngăn.

--- TOA THUỐC (OCR) ---
${userText}
---

YÊU CẦU:
1️⃣ Chuẩn hóa tên thuốc, liều lượng, số lần uống/ngày và hướng dẫn.
2️⃣ Tự động chia thuốc vào **14 ngăn** (sáng, trưa, chiều).
3️⃣ Mỗi ngăn hiển thị: | Ngăn | Ngày | Buổi | Thời gian | Thuốc cần bỏ |
4️⃣ Gợi ý thời gian: Sáng 08:00, Trưa 12:00, Chiều 18:00
                  `,
                                },
                            ],
                        },
                    ],
                }),
            }
        );

        const data = await response.json();
        const result =
            data?.candidates?.[0]?.content?.parts?.[0]?.text ||
            "⚠️ Không có phản hồi từ Gemini.";

        res.json({ result });
    } catch (err) {
        console.error("❌ Lỗi khi gọi Gemini API:", err);
        res.status(500).json({ error: "Lỗi khi gọi Gemini API" });
    }
};
