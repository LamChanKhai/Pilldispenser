#include <Arduino.h>
#include <ArduinoJson.h>
#include "schedule.h"
#include "audio.h"

// ======= STRUCT LỊCH =======
struct ScheduleEntry {
    char time[6];   // "HH:MM"
    bool active;
};

ScheduleEntry scheduleList[14];
int scheduleCount = 0;
int currentIndex  = 0;
int lastTriggeredIndex = -1;  // Ô vừa kích hoạt (để gửi MQTT khi bấm nút đúng giờ)

// Các ô đã kích hoạt (alarm đã báo) từ lần bấm nút trước → ô không bấm = skipped
int triggeredIndices[14];
int triggeredCount = 0;

// Lưu lại mốc thời gian (HH:MM) đã bắn lần gần nhất
// để trong cùng một phút không bắn lại nữa.
char lastTriggeredTime[6] = {0};
bool hasLastTriggered = false;

// =======================================================
// TIỆN ÍCH: KIỂM TRA GIỜ ĐÃ TỒN TẠI CHƯA
// (tránh trùng mốc → 1 phút bắn nhiều lần)
// =======================================================
bool isTimeAlreadyScheduled(const char* timeStr) {
    for (int i = 0; i < scheduleCount; i++) {
        if (scheduleList[i].active && strcmp(scheduleList[i].time, timeStr) == 0) {
            return true;
        }
    }
    return false;
}

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
    lastTriggeredIndex = -1;
    triggeredCount = 0;
    hasLastTriggered = false;
    memset(lastTriggeredTime, 0, sizeof(lastTriggeredTime));
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
            String gioStr = item["gio"].as<String>();
            gioStr.trim();

            char tmp[6];
            gioStr.toCharArray(tmp, 6);

            // Nếu giờ này đã có trong scheduleList thì bỏ qua,
            // tránh 1 mốc giờ trùng xuất hiện nhiều entry khác nhau
            if (isTimeAlreadyScheduled(tmp)) {
                Serial.printf("⚠ Duplicate time %s skipped (custom)\n", tmp);
                continue;
            }

            addSchedule(gioStr);
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
    beepOnce();
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

    // Nếu trong cùng phút này đã từng bắn rồi
    // thì bỏ qua, đợi sang phút mới.
    if (hasLastTriggered && strcmp(now, lastTriggeredTime) == 0) {
        return;
    }

    //Serial.printf("🕒 NOW = %s - Time to check: %s\n",now,scheduleList[currentIndex].time);
    if(scheduleList[currentIndex].active &&
       strcmp(scheduleList[currentIndex].time,now)==0){

        lastTriggeredIndex = currentIndex;  // Ô vừa kích hoạt (để gửi MQTT khi bấm đúng giờ)
        if (triggeredCount < 14) triggeredIndices[triggeredCount++] = currentIndex;  // Lưu để biết ô nào bị skip nếu user không bấm
        Serial.printf("⏰ TIME MATCH → %s → DISPENSE!\n",now);
        dispenseFunc();

        // Ghi lại phút vừa bắn để không bắn lại nữa
        strcpy(lastTriggeredTime, now);
        hasLastTriggered = true;

        currentIndex++;

        if(currentIndex >= scheduleCount)
            currentIndex = 0; // vòng lại
    }
}

int getLastTriggeredIndex() {
    return lastTriggeredIndex;
}

int getTriggeredCount() {
    return triggeredCount;
}

int getTriggeredIndexAt(int i) {
    if (i < 0 || i >= triggeredCount) return -1;
    return triggeredIndices[i];
}

void clearTriggeredList() {
    triggeredCount = 0;
}
