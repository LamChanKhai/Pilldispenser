#include <Arduino.h>
#include <ArduinoJson.h>
#include "schedule.h"

// ======= STRUCT LỊCH =======
struct ScheduleEntry {
    char time[6];   // "HH:MM"
    bool active;
};

ScheduleEntry scheduleList[14];
int scheduleCount = 0;
int currentIndex  = 0;

// =======================================================
// XÓA LỊCH
// =======================================================
void clearSchedule() {
    for(int i=0;i<14;i++){
        scheduleList[i].active = false;
        memset(scheduleList[i].time,0,sizeof(scheduleList[i].time));
    }
    scheduleCount = 0;
    currentIndex  = 0;
    Serial.println("🗑 Schedule cleared");
}

// =======================================================
// THÊM GIỜ UỐNG THUỐC
// =======================================================
void addSchedule(String timeStr){
    if(scheduleCount>=14) return;
    timeStr.trim();
    timeStr.toCharArray(scheduleList[scheduleCount].time,6);
    scheduleList[scheduleCount].active = true;
    scheduleCount++;
    Serial.printf("⏳ Added schedule: %s\n", scheduleList[scheduleCount-1].time);
}

// =======================================================
// 🟢 PARSE JSON MODE = custom
// {
//   mode:"custom",
//   thoiGian:{
//      "1":{gio:"11:11",ngay:"..."},
//      "2":{gio:"22:22",ngay:"..."}
//   }
// }
// =======================================================
void parseCustomJSON(String json){
    DynamicJsonDocument doc(1024);

    if(deserializeJson(doc,json)){
        Serial.println("❌ JSON parse lỗi!");
        return;
    }

    if(!doc["thoiGian"]){
        Serial.println("⚠ JSON không có thoiGian");
        return;
    }

    JsonObject tg = doc["thoiGian"];
    for(JsonPair kv : tg){
        JsonObject item = kv.value();
        if(item.containsKey("gio")){
            addSchedule(item["gio"].as<String>());
        }
    }
    Serial.printf("📥 Loaded %d schedule items (custom)\n",scheduleCount);
}

// =======================================================
// 🟢 PARSE QUICK MODE
// "quick,11:11,22:22" → xen kẽ 14 mốc
// 11:11,22:22,11:11,22:22,... x 14
// =======================================================
void parseQuick(String csv){
    String times[3];  // Đủ để chứa "quick", "11:11", "22:22"
    int index=0,start=0,end;

    // Parse tất cả các phần tử được phân tách bởi dấu phẩy
    while((end=csv.indexOf(",",start))!=-1 && index<3){
        times[index++]=csv.substring(start,end);
        start=end+1;
    }
    // Lấy phần tử cuối cùng
    if(index < 3) {
        times[index++] = csv.substring(start);
    }

    // Kiểm tra format: phải có "quick" và ít nhất 2 thời gian
    if(times[0]!="quick" || index<3) {
        Serial.println("❌ Invalid quick format");
        return;
    }

    // Xen kẽ 2 thời gian thành 14 mốc
    for(int i=0;i<14;i++){
        addSchedule( (i%2==0) ? times[1] : times[2] );
    }
    Serial.println("⚡ QUICK schedule generated (14)");
}

// =======================================================
// GỌI GỬI LỊCH TỪ MQTT
// =======================================================
void setSchedule(String data){
    clearSchedule();
    Serial.println("🔍 Setting schedule with data: " + data);
    if(data.startsWith("{"))         parseCustomJSON(data);
    else if(data.startsWith("quick"))parseQuick(data);
    else Serial.println("❌ Unknown schedule format");

    Serial.printf("📅 TOTAL SCHEDULE LOADED = %d\n",scheduleCount);
}

// =======================================================
// CHECK TIME EVERY LOOP
// trùng giờ → gọi dispense()
// =======================================================
void checkSchedule(void (*dispenseFunc)()){
    if(scheduleCount==0){
        //Serial.printf("Khong co lich");
        return;
    } 

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        //Serial.printf("Khong co timeinfo");
        return;
    } 

    char now[6];
    sprintf(now,"%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min);
    //Serial.printf("🕒 NOW = %s - Time to check: %s\n",now,scheduleList[currentIndex].time);
    if(scheduleList[currentIndex].active &&
       strcmp(scheduleList[currentIndex].time,now)==0){

        Serial.printf("⏰ TIME MATCH → %s → DISPENSE!\n",now);
        dispenseFunc();
        currentIndex++;

        if(currentIndex >= scheduleCount)
            currentIndex = 0; // vòng lại
    }
}
