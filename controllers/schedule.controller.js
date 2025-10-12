import mqtt from 'mqtt';
import {  MQTT_BROKER_URL } from '../config/env.js';
import { io } from '../app.js';
// Kết nối MQTT broker
const client = mqtt.connect(MQTT_BROKER_URL); 
// 👉 sau này bạn thay localhost bằng VPS_PUBLIC_IP
client.subscribe('pill/data/log');
client.subscribe('pill/data/status');
client.on('connect', () => {
  console.log('✅ Connected to MQTT broker');
});

client.on('message', (topic, message) => {
  console.log(`📨 Received from ${topic}: ${message.toString()}`);
  // TODO: bạn có thể emit qua WebSocket cho frontend, hoặc lưu DB
  if(topic === 'pill/data/status') {
    const status = message.toString();
    // send status to frontend
    io.emit('pill/data/status', status);
    console.log('📨 Sent to frontend:', status);
  }
});


export const setSchedule = (req, res) => {
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
    client.publish('pill/command/schedule', (mode + "," + sang + "," + toi));
    console.log(`⏰ Schedule set for ${sang} and ${toi}`);
    res.status(200).json({ message: `Schedule set for ${sang} and ${toi}` });
  } else if(mode === 'custom') {
    const { thoiGian } = req.body;
    console.log('⏰ Time received:', thoiGian);
    if(!thoiGian) {
      return res.status(400).json({ message: 'Thoi gian is required' });
    }
    //client.publish('pill/command/schedule', (mode + "," + JSON.stringify(thoiGian)));
    console.log(`⏰ Schedule set for ${JSON.stringify(thoiGian)}`);
    //{
    //  mode: 'custom',
    //  thoiGian: {
    //    '1': { gio: '11:11', ngay: '1111-11-11' },
    //    '2': { gio: '22:22', ngay: '2222-02-22' }
    //  }
    //}
    for (let i = 0; i < thoiGian.length - 1; i++) {
      if(thoiGian[i].ngay > thoiGian[i+1].ngay) {
        return res.status(400).json({ message: 'Ngày uống phải tăng dần' });
      }
      if(thoiGian[i].gio > thoiGian[i+1].gio && thoiGian[i].ngay === thoiGian[i+1].ngay) {
        return res.status(400).json({ message: 'Giờ uống phải tăng dần' });
      }
    }
    client.publish('pill/command/schedule', (JSON.stringify({mode:'custom',thoiGian})));
    res.status(200).json({ message: `Schedule set for ${thoiGian.length}` });
    }
  };

