from ultralytics import YOLO
import os

if __name__ == '__main__':
    print("🚀 正在加载 YOLOv8 Nano 预训练模型...")
    # yolo会自动去下载几十兆的 yolov8n.pt 初始权重文件
    model = YOLO('yolov8n.pt') 

    print("🔥 炼丹炉点火！开始训练...")
    
    # 注意：把这里的 data 路径，替换成你解压出来的那个数据集文件夹里的 data.yaml 文件的绝对路径！
    # 例如: data=r"C:\Users\28097\Desktop\my_code\my_tools\ADB_test\UI-Monkey-1\data.yaml"
    yaml_path = r"C:\Users\28097\Desktop\my_code\my_code\my_tools\ADB_test\UI-Monkey.v1i.yolov8\data.yaml"
    
    # epochs=50 表示让 AI 把这批图片看 50 遍
    # imgsz=640 表示图片尺寸
    # batch=8 是一次处理几张图，如果你的电脑内存小，可以改成 4
    results = model.train(
        data=yaml_path, 
        epochs=50, 
        imgsz=640,
        batch=8,
        device='cpu' # 如果你有N卡并且配置了CUDA，这里可以删掉，它会自动用GPU起飞。如果没有，就用 CPU 慢慢跑。
    )
    print("✅ 炼丹完成！")