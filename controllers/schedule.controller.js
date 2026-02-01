import { mqttClient } from './mqtt.controller.js';

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
    mqttClient.publish('pill/command/schedule', (mode + "," + sang + "," + toi));
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
    //    ...
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
    mqttClient.publish('pill/command/schedule', (JSON.stringify({mode:'custom',thoiGian})));
    res.status(200).json({ message: `Schedule set for ${thoiGian.length}` });
  }
};

export const refill = (req, res) => {
  mqttClient.publish('pill/command/refill' );
  res.status(200).json({ message: `Refilling` });
};
