from google import genai
import os
import serial
import time

client = genai.Client(api_key=os.getenv("GEMINI_API_KEY"))
ser = serial.Serial(
    port='COM3',  # 장치관리자에서 확인해서 포트 번호는 수정할 것.
    baudrate=115200,
    timeout=1
)

while True:
    user_input = input("Enter your command: ")
    if user_input.lower() == "exit":
        break
    response = client.models.generate_content(
        model="gemini-2.5-flash-lite",
        contents=user_input
    )
    print(f"You entered: {user_input}")
    print(response.candidates[0].content.parts[0].text)
    ser.write(response.candidates[0].content.parts[0].text.encode('utf-8'))