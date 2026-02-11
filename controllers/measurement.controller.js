import { bpModel, spo2Model } from "../model/measurement.model.js";
import { emit } from "../realtime/socket.js";
import { DEFAULT_USER_ID } from '../config/env.js';
import mongoose from 'mongoose';

// =======================================================
// MQTT MESSAGE HANDLERS
// =======================================================

/**
 * Xử lý message từ topic 'pill/data/status'
 */
export const handleStatusMessage = (status) => {
  // Gửi status đến frontend qua WebSocket
  emit("pill/data/status", status);
  console.log('📨 Sent status to frontend:', status);
};

/**
 * Xử lý message từ topic 'pill/data/measurement'
 */
export const handleMeasurementMessage = async (message) => {
  try {
    const measurementData = JSON.parse(message);
    console.log('🫀 Received measurement data:', measurementData);
    
    // Lưu vào database
    // Sử dụng userId từ measurementData hoặc userId mặc định từ config
    let userId = measurementData.userId || DEFAULT_USER_ID;
    
    if (!userId) {
      console.warn('⚠️ userId missing in measurement data and no DEFAULT_USER_ID configured, skipping save');
      // Vẫn gửi qua WebSocket để frontend có thể xử lý
      emit("pill/data/measurement", measurementData);
      return;
    }
    
    // Đảm bảo userId là ObjectId hợp lệ
    if (!mongoose.Types.ObjectId.isValid(userId)) {
      console.error('❌ Invalid userId format:', userId);
      return;
    }
    
    const dataToSave = {
      userId: new mongoose.Types.ObjectId(userId),
      heart_beat: measurementData.heart || measurementData.heart_beat,  // Hỗ trợ cả 'heart' và 'heart_beat'
      spo2: measurementData.spo2,
      temp: measurementData.temp || measurementData.temperature  // Hỗ trợ cả 'temp' và 'temperature'
    };
    
    await saveMeasurement(dataToSave);
    console.log('✅ Measurement saved to database');
    
    // Gửi qua WebSocket cho frontend
    emit("pill/data/measurement", measurementData);
    console.log('📨 Sent measurement to frontend');
  } catch (error) {
    console.error('❌ Error processing measurement:', error);
  }
};

// =======================================================
// DATABASE OPERATIONS
// =======================================================

/**
 * Lưu dữ liệu measurement (heart_beat, spo2, temp) vào MongoDB
 */
export const saveMeasurement = async (data) => {
  try {
    // Lưu vào spo2Model với spo2, heart_beat (optional), và temperature (optional)
    const measurementData = {
      userId: data.userId,
      spo2: data.spo2
    };
    
    // Chỉ thêm heart_beat nếu có giá trị
    if (data.heart_beat !== undefined && data.heart_beat !== null) {
      measurementData.heart_beat = data.heart_beat;
    }
    
    // Chỉ thêm temperature nếu có giá trị
    if (data.temp !== undefined && data.temp !== null) {
      measurementData.temperature = data.temp;
    }
    
    const measurement = new spo2Model(measurementData);
    await measurement.save();
    console.log('✅ Measurement (SPO2) saved');
  } catch (error) {
    console.error("Error saving measurement:", error);
    throw error;
  }
};

/**
 * Lưu dữ liệu bp từ MQTT vào MongoDB (Route handler)
 */
export const saveBp = async (req, res) => {
  try {
    const data = req.body;
    if (data.userId && !mongoose.Types.ObjectId.isValid(data.userId)) {
      return res.status(400).json({ error: "Invalid userId format" });
    }
    if (data.userId) {
      data.userId = new mongoose.Types.ObjectId(data.userId);
    }
    const bp = new bpModel(data);
    await bp.save();
    res.status(201).json(bp);
  } catch (error) {
    console.error("Error saving bp:", error);
    res.status(500).json({ error: "Internal server error" });
  }
};

/**
 * Lưu dữ liệu spo2 từ MQTT vào MongoDB (Route handler)
 */
export const saveSpo2 = async (req, res) => {
  try {
    const data = req.body;
    if (data.userId && !mongoose.Types.ObjectId.isValid(data.userId)) {
      return res.status(400).json({ error: "Invalid userId format" });
    }
    if (data.userId) {
      data.userId = new mongoose.Types.ObjectId(data.userId);
    }
    const spo2 = new spo2Model(data);
    await spo2.save();
    res.status(201).json(spo2);
  } catch (error) {
    console.error("Error saving spo2:", error);
    res.status(500).json({ error: "Internal server error" });
  }
};

/**
 * Lưu dữ liệu bp từ MQTT vào MongoDB (Internal function)
 */
export const saveBpInternal = async (data) => {
  try {
    const bp = new bpModel(data);
    await bp.save();
  } catch (error) {
    console.error("Error saving bp:", error);
  }
};

/**
 * Lưu dữ liệu spo2 từ MQTT vào MongoDB (Internal function)
 */
export const saveSpo2Internal = async (data) => {
  try {
    const spo2 = new spo2Model(data);
    await spo2.save();
  } catch (error) {
    console.error("Error saving spo2:", error);
  }
};

/**
 * Lấy dữ liệu bp từ MongoDB và trả về cho client
 */
export const getBp = async (req, res) => {
  try {
    const { userId } = req.params;
    if (!mongoose.Types.ObjectId.isValid(userId)) {
      return res.status(400).json({ error: "Invalid userId format" });
    }
    const bp = await bpModel.find({ userId: new mongoose.Types.ObjectId(userId) }).sort({ createdAt: 1 });
    res.json(bp);
  } catch (error) {
    console.error("Error getting bp:", error);
    res.status(500).json({ error: "Internal server error" });
  }
};

/**
 * Lấy dữ liệu spo2 từ MongoDB và trả về cho client
 */
export const getSpo2 = async (req, res) => {
  try {
    const { userId } = req.params;
    if (!mongoose.Types.ObjectId.isValid(userId)) {
      return res.status(400).json({ error: "Invalid userId format" });
    }
    const spo2 = await spo2Model.find({ userId: new mongoose.Types.ObjectId(userId) }).sort({ createdAt: 1 });
    res.json(spo2);
  } catch (error) {
    console.error("Error getting spo2:", error);
    res.status(500).json({ error: "Internal server error" });
  }
};