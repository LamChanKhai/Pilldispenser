# ===============================
# TẮT LOG TENSORFLOW (BẮT BUỘC)
# ===============================
import os
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "3"
os.environ["TF_ENABLE_ONEDNN_OPTS"] = "0"

# ===============================
# TẮT WARNINGS
# ===============================
import warnings
warnings.filterwarnings("ignore", category=FutureWarning)
warnings.filterwarnings("ignore", message=".*np.object.*")

# ===============================
# IMPORT
# ===============================
from flask import Flask, request, jsonify
import joblib
import numpy as np
import pandas as pd
from tensorflow.keras.models import load_model

# ===============================
# KHỞI TẠO FLASK APP
# ===============================
app = Flask(__name__)

# ===============================
# LOAD MODEL & SCALER (MỘT LẦN KHI KHỞI ĐỘNG)
# ===============================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Global variables để lưu model, scaler và feature names
model = None
scaler = None
feature_names = None

def load_ml_models():
    """Load model, scaler và feature names một lần khi khởi động server"""
    global model, scaler, feature_names
    
    try:
        model_path_keras = os.path.join(BASE_DIR, "hypertension_model.keras")
        model_path_h5 = os.path.join(BASE_DIR, "hypertension_model.h5")
        
        loaded_model = None
        
        # Thử load .keras trước
        if os.path.exists(model_path_keras):
            try:
                # Thử load với safe_mode=False cho Keras 3.x
                try:
                    loaded_model = load_model(model_path_keras, safe_mode=False)
                except TypeError:
                    # Nếu safe_mode không được hỗ trợ (Keras 2.x), load bình thường
                    try:
                        loaded_model = load_model(model_path_keras)
                    except Exception:
                        # Nếu vẫn lỗi, thử load với compile=False
                        loaded_model = load_model(model_path_keras, compile=False)
            except Exception as e1:
                # Nếu .keras lỗi, thử .h5
                if os.path.exists(model_path_h5):
                    try:
                        loaded_model = load_model(model_path_h5)
                    except Exception as e2:
                        raise Exception(f"Both .keras and .h5 failed. .keras: {str(e1)}, .h5: {str(e2)}")
                else:
                    raise e1
        elif os.path.exists(model_path_h5):
            # Chỉ có .h5
            loaded_model = load_model(model_path_h5)
        else:
            raise Exception("No model file found (hypertension_model.keras or hypertension_model.h5)")
        
        # Gán vào biến global
        model = loaded_model
        scaler = joblib.load(os.path.join(BASE_DIR, "scaler.pkl"))
        feature_names = joblib.load(os.path.join(BASE_DIR, "feature_names.pkl"))
        
        print("✅ Model, scaler và feature names loaded successfully")
        return True
    except Exception as e:
        print(f"❌ Error loading models: {str(e)}")
        return False

# Load models khi khởi động
if not load_ml_models():
    print("⚠️  Server started but models failed to load. API will return errors.")

# ===============================
# FEATURE CONFIGURATION
# ===============================
NUM_COLS = ['age', 'cigsPerDay', 'sysBP', 'diaBP', 'BMI', 'heartRate']
CAT_COLS = ['male', 'currentSmoker', 'BPMeds', 'diabetes']

DEFAULT_VALUES = {
    "male": 0,
    "age": 50,
    "currentSmoker": 0,
    "cigsPerDay": 0,
    "BPMeds": 0,
    "diabetes": 0,
    "sysBP": 120,
    "diaBP": 80,
    "BMI": 25,
    "heartRate": 70
}

# ===============================
# HÀM TIỀN XỬ LÝ DỮ LIỆU
# ===============================
def preprocess_data(input_data):
    """
    Tiền xử lý dữ liệu đầu vào:
    - Bù giá trị thiếu
    - Scaling các biến số
    - Tạo dummy variables cho biến phân loại
    - Đảm bảo đủ các cột cần thiết
    
    Args:
        input_data (dict): Dictionary chứa dữ liệu đầu vào
        
    Returns:
        pandas.DataFrame: DataFrame đã được xử lý sẵn sàng cho prediction
    """
    # Tạo bản sao để không thay đổi input gốc
    data = input_data.copy()
    
    # Bù giá trị thiếu
    for k, v in DEFAULT_VALUES.items():
        if k not in data or data[k] in ["", None]:
            data[k] = v
    
    # Tạo DataFrame
    df = pd.DataFrame([data])
    
    # Scaling các biến số
    df[NUM_COLS] = scaler.transform(df[NUM_COLS])
    
    # Tạo dummy variables cho biến phân loại
    df = pd.get_dummies(df, columns=CAT_COLS)
    
    # Fix các cột thiếu (thêm cột = 0 nếu thiếu)
    for col in feature_names:
        if col not in df.columns:
            df[col] = 0
    
    # Đảm bảo thứ tự cột đúng với feature_names
    df = df[feature_names]
    
    return df

# ===============================
# HÀM DIỄN GIẢI KẾT QUẢ
# ===============================
def interpret_result(probability):
    """
    Diễn giải kết quả dự đoán thành mức độ nguy cơ
    
    Args:
        probability (float): Xác suất từ model (0-1)
        
    Returns:
        str: Mức độ nguy cơ
    """
    if probability < 0.3:
        return "Bình thường"
    elif probability < 0.6:
        return "Tiền tăng huyết áp"
    else:
        return "Nguy cơ tăng huyết áp cao"

# ===============================
# ROUTES
# ===============================
@app.route("/", methods=["GET"])
def health_check():
    """Health check endpoint"""
    if model is None or scaler is None or feature_names is None:
        return jsonify({
            "status": "error",
            "message": "Models not loaded"
        }), 500
    
    return jsonify({
        "status": "ok",
        "message": "Hypertension Prediction API is running"
    })

@app.route("/predict", methods=["POST"])
def predict():
    """
    Endpoint dự đoán nguy cơ tăng huyết áp
    
    Request body (JSON):
    {
        "age": 50,
        "male": 1,
        "currentSmoker": 0,
        "cigsPerDay": 0,
        "BPMeds": 0,
        "diabetes": 0,
        "sysBP": 120,
        "diaBP": 80,
        "BMI": 25,
        "heartRate": 70
    }
    
    Response (JSON):
    {
        "BP_Level": "Bình thường",
        "Hypertension_Risk": "25.50%"
    }
    """
    # Kiểm tra model đã load chưa
    if model is None or scaler is None or feature_names is None:
        return jsonify({
            "error": "Models not loaded. Please check server logs."
        }), 500
    
    # Kiểm tra request có JSON không
    if not request.is_json:
        return jsonify({
            "error": "Content-Type must be application/json"
        }), 400
    
    try:
        # Lấy dữ liệu từ request
        input_data = request.get_json()
        
        # Tiền xử lý dữ liệu
        processed_df = preprocess_data(input_data)
        
        # Dự đoán
        prob = float(model.predict(processed_df, verbose=0)[0][0])
        
        # Diễn giải kết quả
        bp_level = interpret_result(prob)
        
        # Trả về kết quả
        result = {
            "BP_Level": bp_level,
            "Hypertension_Risk": f"{prob * 100:.2f}%"
        }
        
        return jsonify(result), 200
        
    except KeyError as e:
        return jsonify({
            "error": f"Missing required field: {str(e)}"
        }), 400
    except Exception as e:
        return jsonify({
            "error": f"Prediction failed: {str(e)}"
        }), 500

# ===============================
# MAIN
# ===============================
if __name__ == "__main__":
    # Lấy port từ biến môi trường (mặc định 5000)
    # Render sẽ tự động set PORT environment variable
    port = int(os.environ.get("PORT", 5000))
    
    # Chạy server
    # Lưu ý: Trong production, sử dụng gunicorn:
    # gunicorn -w 4 -b 0.0.0.0:$PORT app:app
    app.run(host="0.0.0.0", port=port, debug=False)

