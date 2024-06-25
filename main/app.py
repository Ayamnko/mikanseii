from flask import Flask, jsonify, request
from flask_cors import CORS
import json
import os
from datetime import date

app = Flask(__name__)
CORS(app)

# データ保存用のファイルパス
data_file = 'study_data.json'

# 初期データ
initial_study_data = [
    {'date': str(date.today()), 'subject': '数学', 'time_minutes': 120, 'color': '#FF6384'},
    {'date': str(date.today()), 'subject': '英語', 'time_minutes': 90, 'color': '#36A2EB'}
]

# データを読み込む関数
def load_data():
    global study_data
    try:
        with open(data_file, 'r', encoding='utf-8') as f:
            study_data = json.load(f)
    except FileNotFoundError:
        study_data = initial_study_data  # ファイルが存在しない場合は初期データを使用

# データを保存する関数
def save_data():
    with open(data_file, 'w', encoding='utf-8') as f:
        json.dump(study_data, f, ensure_ascii=False, indent=4)

# アプリケーション起動時にデータを読み込む
load_data()

# 科目ごとの勉強時間を取得するエンドポイント
@app.route('/api/study_data', methods=['GET'])
def get_study_data():
    return jsonify(study_data)

# 科目ごとの勉強時間を保存するエンドポイント
@app.route('/api/study_data', methods=['POST'])
def add_or_update_study_data():
    request_data = request.get_json()
    subject = request_data.get('subject')
    time_minutes = request_data.get('time_minutes')
    color = request_data.get('color', '#4BC0C0')  # デフォルトの色を設定
    today = str(date.today())  # 今日の日付を取得

    # 既存の科目があれば時間を更新、なければ新規追加
    for data in study_data:
        if data['date'] == today and data['subject'] == subject:
            data['time_minutes'] += time_minutes
            break
    else:
        study_data.append({'date': today, 'subject': subject, 'time_minutes': time_minutes, 'color': color})

    save_data()  # データを保存する

    return jsonify(study_data)

# 科目ごとの勉強時間を減らすエンドポイント
@app.route('/api/study_data/<subject>', methods=['DELETE'])
def delete_study_data(subject):
    global study_data
    request_data = request.get_json()
    time_minutes = request_data.get('time_minutes')
    today = str(date.today())  # 今日の日付を取得

    for data in study_data:
        if data['date'] == today and data['subject'] == subject:
            data['time_minutes'] -= time_minutes
            if data['time_minutes'] < 0:
                data['time_minutes'] = 0
            break

    study_data = [data for data in study_data if not (data['time_minutes'] == 0 and data['date'] == today)]  # 勉強時間が0以下のデータを削除

    save_data()  # データを保存する

    return jsonify(study_data)

if __name__ == '__main__':
    app.run(debug=True)
