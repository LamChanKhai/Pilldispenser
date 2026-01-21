import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import roc_curve, auc, confusion_matrix
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.callbacks import EarlyStopping

# 1. TIỀN XỬ LÝ DỮ LIỆU (Theo LaFreniere 2016)
df = pd.read_csv('dataset/processed_dataset.csv').fillna(0)
num_cols = ['age', 'cigsPerDay', 'sysBP', 'diaBP', 'BMI', 'heartRate']
cat_cols = ['male', 'currentSmoker', 'BPMeds', 'diabetes']

scaler = StandardScaler()
df[num_cols] = scaler.fit_transform(df[num_cols])
df = df[(np.abs(df[num_cols]) <= 5).all(axis=1)] 
df = pd.get_dummies(df, columns=cat_cols)

X = df.drop('Risk', axis=1)
y = df['Risk']
X_train, X_temp, y_train, y_temp = train_test_split(X, y, test_size=0.3, random_state=42)
X_val, X_test, y_val, y_test = train_test_split(X_temp, y_temp, test_size=0.5, random_state=42)

# 2. HUẤN LUYỆN MÔ HÌNH (11-7-2)
model = Sequential([
    Dense(7, input_dim=X_train.shape[1], activation='sigmoid'),
    Dense(1, activation='sigmoid')
])
model.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])
model.fit(X_train, y_train, validation_data=(X_val, y_val), epochs=100, batch_size=32, 
          callbacks=[EarlyStopping(monitor='val_loss', patience=6, restore_best_weights=True)], verbose=0)

# 3. VẼ FIG 6: 4 BIỂU ĐỒ ROC
def plot_fig6():
    fig, axes = plt.subplots(2, 2, figsize=(14, 12))
    sets = [('Training ROC', X_train, y_train), ('Validation ROC', X_val, y_val),
            ('Test ROC', X_test, y_test), ('All ROC', X, y)]

    for i, (title, dx, dy) in enumerate(sets):
        probs = model.predict(dx)
        fpr, tpr, _ = roc_curve(dy, probs)
        ax = axes[i//2, i%2]
        ax.plot(fpr, tpr, color='blue', lw=2, label=f'AUC = {auc(fpr, tpr):.2f}')
        ax.plot([0, 1], [0, 1], color='green', linestyle='--')
        ax.set_title(title, fontsize=14, pad=15)
        ax.set_xlabel('False Positive Rate (FPR)')
        ax.set_ylabel('True Positive Rate (TPR)')
        ax.legend(loc='lower right')
        ax.grid(alpha=0.2)
    
    plt.subplots_adjust(hspace=0.4, wspace=0.3)
    plt.show()

# 4. VẼ FIG 5: ALL CONFUSION MATRIX (9 Ô CHUẨN MÀU BÀI BÁO)
def plot_fig5():
    y_pred = (model.predict(X) > 0.5).astype(int)
    cm = confusion_matrix(y, y_pred)
    tn, fp, fn, tp = cm.ravel()
    total = np.sum(cm)

    # Tính toán các chỉ số cho nhãn
    labels = np.empty((3, 3), dtype=object)
    labels[0,0] = f"{tn}\n{tn/total:.1%}"; labels[0,1] = f"{fp}\n{fp/total:.1%}"
    labels[1,0] = f"{fn}\n{fn/total:.1%}"; labels[1,1] = f"{tp}\n{tp/total:.1%}"

    p0 = tn/(tn+fp); labels[0,2] = f"{p0:.1%} correct\n{1-p0:.1%} incorrect"
    p1 = tp/(tp+fn); labels[1,2] = f"{p1:.1%} correct\n{1-p1:.1%} incorrect"
    s0 = tn/(tn+fn); labels[2,0] = f"{s0:.1%} correct\n{1-s0:.1%} incorrect"
    s1 = tp/(tp+fp); labels[2,1] = f"{s1:.1%} correct\n{1-s1:.1%} incorrect"
    acc = (tn+tp)/total; labels[2,2] = f"{acc:.1%} correct\n{1-acc:.1%} incorrect"

    # Định nghĩa màu sắc theo bài báo 
    # TN(0,0)=Green, FP(0,1)=Red, Summary(0,2)=Grey
    # FN(1,0)=Red, TP(1,1)=Green, Summary(1,2)=Grey
    # Summary(2,0)=Grey, Summary(2,1)=Grey, Accuracy(2,2)=Black
    colors = [
        ['#228B22', '#D22B2B', '#A9A9A9'],  # Hàng 0: Xanh, Đỏ, Xám
        ['#D22B2B', '#228B22', '#A9A9A9'],  # Hàng 1: Đỏ, Xanh, Xám
        ['#A9A9A9', '#A9A9A9', '#000000']   # Hàng 2: Xám, Xám, Đen
    ]

    fig, ax = plt.subplots(figsize=(10, 8))
    
    # Vẽ từng ô thủ công để kiểm soát màu sắc và viền
    for i in range(3):
        for j in range(3):
            color = colors[i][j]
            rect = plt.Rectangle([j, 2-i], 1, 1, facecolor=color, edgecolor='black', linewidth=1.5)
            ax.add_patch(rect)
            
            # Chữ trắng cho nền đen, chữ đen cho các nền khác
            text_color = 'white' if color == '#000000' else 'black'
            ax.text(j+0.5, 2.5-i, labels[i, j], ha='center', va='center', 
                    fontsize=11, fontweight='bold', color=text_color)

    # Thiết lập trục
    ax.set_xlim(0, 3)
    ax.set_ylim(0, 3)
    ax.set_xticks([0.5, 1.5, 2.5])
    ax.set_xticklabels(['0', '1', '']) # Bỏ chữ Summary
    ax.set_yticks([0.5, 1.5, 2.5])
    ax.set_yticklabels(['', '1', '0']) # Bỏ chữ Summary
    
    ax.set_xlabel('Target Class', fontsize=12, fontweight='bold')
    ax.set_ylabel('Output Class', fontsize=12, fontweight='bold')

    # Thêm viền đen đậm bao quanh toàn bộ ma trận
    for spine in ax.spines.values():
        spine.set_visible(True)
        spine.set_linewidth(2)
        spine.set_color('black')

    plt.tight_layout()
    plt.show()

# Thực thi
plot_fig6()
plot_fig5()
model.save('hypertension_model.keras')