import Measurement from "../model/measurement.model.js";
// 2 task
// 1. Lưu dữ liệu đo lường từ MQTT vào MongoDB
// 2. Lấy dữ liệu đo lường từ MongoDB và trả về cho client

// Lưu dữ liệu đo lường từ MQTT vào MongoDB
export const saveMeasurement = async (data) => {
    try {
        const measurement = new Measurement(data);
        await measurement.save();
    } catch (error) {
        console.error("Error saving measurement:", error);
    }
};
// Lấy dữ liệu đo lường từ MongoDB và trả về cho client
export const getMeasurements = async (req, res) => {
    try {
        const { userId } = req.params;
        const measurements = await Measurement.find({ userId });
        res.json(measurements);
    } catch (error) {
        console.error("Error getting measurements:", error);
        res.status(500).json({ error: "Internal server error" });
    }
};