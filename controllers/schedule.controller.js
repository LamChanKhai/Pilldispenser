import { mqttClient } from './mqtt.controller.js';
import scheduleModel from '../model/schedule.model.js';
/** Kiểm tra MQTT có sẵn sàng (cần server chạy lâu dài, KHÔNG chạy trên Vercel serverless) */
function ensureMqtt(res) {
  if (!mqttClient || !mqttClient.connected) {
    res.status(503).json({
      message: 'MQTT không khả dụng. Ứng dụng cần chạy trên server có process lâu dài (Railway, Render, VPS...) thay vì Vercel.',
      code: 'MQTT_UNAVAILABLE'
    });
    return false;
  }
  return true;
}

export const setSchedule = async (req, res) => {
  if (!ensureMqtt(res)) return;
  console.log('⏰ Setting schedule with data:', req.body);
  const { mode } = req.body;
  if(mode === 'quick') {
    const { sang, toi } = req.body;
    console.log('⏰ Time received:', { sang, toi });
    if (!sang || !toi) {
      return res.status(400).json({ message: 'Both times are required' });
    }
    if(sang > toi) {
      return res.status(400).json({ message: 'Morning time must be before evening time' });
    }
    mqttClient.publish('pill/command/schedule', (mode + "," + sang + "," + toi));
    console.log(`⏰ Schedule set for ${sang} and ${toi}`);
    //res.status(200).json({ message: `Schedule set for ${sang} and ${toi}` });
    const timeArray = [];
    for (let i = 0; i < 14; i++) 
      timeArray[i] = (i%2==0) ? sang : toi;
    try {
    const newSchedule = await scheduleModel.create({
      userId: req.body.userId,
      timeArray: timeArray,
      status_Array: Array(14).fill('incomplete'),
      index: 0
    });
    res.status(200).json({ message: `Schedule created`, schedule: newSchedule });
    } catch (error) {
      console.error("Error creating schedule:", error);
      res.status(500).json({ message: "Lỗi máy chủ" });
    }
  } else if(mode === 'custom') {
    const { thoiGian } = req.body;
    console.log('⏰ Time received:', thoiGian);
    if(!thoiGian) {
      return res.status(400).json({ message: 'Thoi gian is required' });
    }
    const thoiGianArray = Object.values(thoiGian);
    for (let i = 0; i < thoiGianArray.length - 1; i++) {
      if(thoiGianArray[i].ngay > thoiGianArray[i+1].ngay) {
        return res.status(400).json({ message: 'Ngày uống phải tăng dần' });
      }
      if(thoiGianArray[i].gio > thoiGianArray[i+1].gio && thoiGianArray[i].ngay === thoiGianArray[i+1].ngay) {
        return res.status(400).json({ message: 'Giờ uống phải tăng dần' });
      }
    }
    mqttClient.publish('pill/command/schedule', (JSON.stringify({mode:'custom',thoiGian})));
    //res.status(200).json({ message: `Schedule set for ${thoiGian.length}` });
  
    const timeArray = [];
    for (let i = 0; i < thoiGianArray.length; i++) {
      timeArray[i] = thoiGianArray[i].gio;
    }
    try {
      const newSchedule = await scheduleModel.create({
      userId: req.body.userId,
      timeArray: timeArray,
      status_Array: Array(thoiGianArray.length).fill('incomplete').concat(Array(14-thoiGianArray.length).fill('empty')),
      index: 0
    });
    res.status(200).json({ message: `Schedule created`, schedule: newSchedule });
    } catch (error) {
      console.error("Error creating schedule:", error);
      res.status(500).json({ message: "Lỗi máy chủ" });
    }
  }
};

export const refill = (req, res) => {
  if (!ensureMqtt(res)) return;
  mqttClient.publish('pill/command/refill' );
  res.status(200).json({ message: `Refilling` });
};

/** Lấy Schedule mới nhất theo userId (query: userId) */
export const getLatestScheduleByUser = async (req, res) => {
  const { userId } = req.query;
  if (!userId) {
    return res.status(400).json({ message: 'userId is required' });
  }
  try {
    const schedule = await scheduleModel
      .findOne({ userId })
      .sort({ createdAt: -1 })
      .lean();
    if (!schedule) {
      return res.status(404).json({ message: 'No schedule found', schedule: null });
    }
    res.status(200).json(schedule);
  } catch (error) {
    console.error('Error fetching latest schedule:', error);
    res.status(500).json({ message: 'Lỗi máy chủ' });
  }
};
