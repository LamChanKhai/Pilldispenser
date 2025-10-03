import mqtt from 'mqtt';

// Kết nối MQTT broker
const client = mqtt.connect('mqtt://34.63.156.115:1883'); 
// 👉 sau này bạn thay localhost bằng VPS_PUBLIC_IP
client.subscribe('pill/data/log');
client.on('connect', () => {
  console.log('✅ Connected to MQTT broker');
});

client.on('message', (topic, message) => {
  console.log(`📨 Received from ${topic}: ${message.toString()}`);
  // TODO: bạn có thể emit qua WebSocket cho frontend, hoặc lưu DB
});


export const setSchedule = (req, res) => {
  console.log('⏰ Setting schedule with data:', req.body);
  const { mode, sang, toi } = req.body;
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
};

