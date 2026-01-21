# ===============================================
# File: train_hypertension_model.py
# Mô hình Random Forest dự đoán huyết áp & nguy cơ tăng huyết áp
# Tác giả: Lê Đình Khánh
# ===============================================

import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report, accuracy_score
import joblib

# ===============================================
# 1️⃣ Đọc dataset
# ===============================================
# Giả sử file dữ liệu nằm trong cùng thư mục
df = pd.read_csv("./dataset/Hypertension-risk-model-main.csv")

# Kiểm tra dữ liệu
print("✅ Dataset loaded. Shape:", df.shape)
print(df.head())

# ===============================================
# 2️⃣ Xử lý dữ liệu & tạo nhãn mức huyết áp
# ===============================================
def bp_category(sysBP, diaBP):
    if sysBP < 120 and diaBP < 80:
        return 0  # Bình thường
    elif 120 <= sysBP < 140 or 80 <= diaBP < 90:
        return 1  # Tiền tăng huyết áp
    elif 140 <= sysBP < 160 or 90 <= diaBP < 100:
        return 2  # Tăng huyết áp độ 1
    else:
        return 3  # Tăng huyết áp độ 2

df["BP_Level"] = df.apply(lambda x: bp_category(x["sysBP"], x["diaBP"]), axis=1)

# Loại bỏ các hàng thiếu dữ liệu
df.dropna(inplace=True)

# ===============================================
# 3️⃣ Chia dữ liệu
# ===============================================
X = df.drop(["Risk", "BP_Level"], axis=1)
y_bp = df["BP_Level"]   # Task 1
y_risk = df["Risk"]     # Task 2

X_train, X_test, y_bp_train, y_bp_test = train_test_split(X, y_bp, test_size=0.2, random_state=42)
_, _, y_risk_train, y_risk_test = train_test_split(X, y_risk, test_size=0.2, random_state=42)

# ===============================================
# 4️⃣ Huấn luyện mô hình Random Forest
# ===============================================
print("\n🔹 Đang huấn luyện mô hình phân loại mức huyết áp...")
rf_bp = RandomForestClassifier(n_estimators=200, random_state=42)
rf_bp.fit(X_train, y_bp_train)

y_bp_pred = rf_bp.predict(X_test)
print("\n🩸 Kết quả phân loại mức huyết áp:")
print("Độ chính xác:", accuracy_score(y_bp_test, y_bp_pred))
print(classification_report(y_bp_test, y_bp_pred))

# -----------------------------------------------
print("\n🔹 Đang huấn luyện mô hình dự đoán nguy cơ tăng huyết áp...")
rf_risk = RandomForestClassifier(n_estimators=200, random_state=42)
rf_risk.fit(X_train, y_risk_train)

y_risk_pred = rf_risk.predict(X_test)
print("\n⚠️ Kết quả dự đoán nguy cơ tăng huyết áp:")
print("Độ chính xác:", accuracy_score(y_risk_test, y_risk_pred))
print(classification_report(y_risk_test, y_risk_pred))

# ===============================================
# 5️⃣ Lưu mô hình
# ===============================================
joblib.dump(rf_bp, "model_bp_level.pkl")
joblib.dump(rf_risk, "model_hypertension_risk.pkl")
print("\n💾 Mô hình đã được lưu thành công (model_bp_level.pkl & model_hypertension_risk.pkl)")

# ===============================================
# 6️⃣ Ví dụ dự đoán thử
# ===============================================
example_data = np.array([[1, 39, 0, 0, 0, 0, 195, 106, 70, 26.97, 80, 77]])
bp_level_pred = rf_bp.predict(example_data)[0]
risk_prob = rf_risk.predict_proba(example_data)[0][1] * 100

bp_levels = ["Bình thường", "Tiền tăng huyết áp", "Tăng huyết áp độ 1", "Tăng huyết áp độ 2"]

print("\n================= DỰ ĐOÁN MẪU =================")
print("🩸 Mức huyết áp:", bp_levels[bp_level_pred])
print(f"⚠️ Xác suất tăng huyết áp: {risk_prob:.2f}%")
print("================================================")
