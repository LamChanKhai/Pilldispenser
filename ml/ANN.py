import pandas as pd
import numpy as np
import joblib

from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.callbacks import EarlyStopping

# ======================
# 1. LOAD & PREPROCESS
# ======================
df = pd.read_csv('dataset/processed_dataset.csv').fillna(0)

num_cols = ['age', 'cigsPerDay', 'sysBP', 'diaBP', 'BMI', 'heartRate']
cat_cols = ['male', 'currentSmoker', 'BPMeds', 'diabetes']

# Scale số
scaler = StandardScaler()
df[num_cols] = scaler.fit_transform(df[num_cols])

# Loại outlier (theo LaFreniere 2016)
df = df[(np.abs(df[num_cols]) <= 5).all(axis=1)]

# One-hot
df = pd.get_dummies(df, columns=cat_cols)

X = df.drop('Risk', axis=1)
y = df['Risk']

# Chia dữ liệu
X_train, X_temp, y_train, y_temp = train_test_split(
    X, y, test_size=0.3, random_state=42
)
X_val, X_test, y_val, y_test = train_test_split(
    X_temp, y_temp, test_size=0.5, random_state=42
)

# ======================
# 2. MODEL (11–7–1)
# ======================
model = Sequential([
    Dense(7, input_dim=X_train.shape[1], activation='sigmoid'),
    Dense(1, activation='sigmoid')
])

model.compile(
    optimizer='adam',
    loss='binary_crossentropy',
    metrics=['accuracy']
)

model.fit(
    X_train, y_train,
    validation_data=(X_val, y_val),
    epochs=100,
    batch_size=32,
    callbacks=[
        EarlyStopping(
            monitor='val_loss',
            patience=6,
            restore_best_weights=True
        )
    ],
    verbose=1
)

# ======================
# 3. SAVE FOR WEB
# ======================
model.save('hypertension_model.keras')
joblib.dump(scaler, 'scaler.pkl')
joblib.dump(X.columns.tolist(), 'feature_names.pkl')

print("✅ Model + scaler + feature names saved")
