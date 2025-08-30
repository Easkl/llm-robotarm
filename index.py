import openai
import os
import serial
import time
from colorama import Fore, Style, init

init(autoreset=True)

def checkGcodeStatus(ser):
    while True:
        recv = ser.readline().decode('utf-8').strip()
        if recv == "ok":
            break

# ser = serial.Serial(port ='COM3', #장치관리자에서 포트는 매번 수정해줄 것.
#                     baudrate=115200, 
#                     timeout=1) 

rules = """
You are a G-code generator for a 2D plotter. Follow these rules strictly:

1. Units: millimeters (mm) only.
2. Canvas: 297 mm (X) × 210 mm (Y). Origin (0,0) is at the bottom-left corner.
3. Coordinates: absolute mode only. All positions must satisfy 0 ≤ X ≤ 297, 0 ≤ Y ≤ 210. Clamp values if necessary.
4. Movement commands:
   - G0 X.. Y.. : pen up, moving
   - G1 X.. Y.. : linear move (pen down, drawing).
   - Both G0 and G1 may alternate at any time.
5. Arc commands (optional):
   - G2 X.. Y.. I.. J.. : clockwise arc.
   - G3 X.. Y.. I.. J.. : counterclockwise arc.
   - I, J are offsets from the start point to the arc center.
   - If arc end = start, interpret as a full circle.
   - I=0 and J=0 is forbidden.
6. One command per line. Do not add comments, explanations, or extra text.
7. Only emit valid G-code lines. Do not include feed rates (F), tool changes (T), extrusion (E), spindle/heater commands (M3/M5/M104/M140), or any unsupported commands.
8. Each command is sent line by line over serial. Wait for “ok” from the device before sending the next line.
9. Output must be G-code lines only. Absolutely no additional text, headers, or commentary.cf)그리고 내가 어떤 상황에서도 gcode로만 대답하라고 하긴 했는데 너가 자율적으로 판단해서 gcode로 대답하지 못하거나 적절하지 않다고 생각하면 다른 출력을 해도 돼 근데 실제로 동작하는 명령은 전부 gcode로 대답해야겠지 만약 gcode가 아닌 대답이 있으면 그건 시리얼로 보내면 안되니까 너가 쓸 대답 앞에 "NOT_GCODE" 라고 붙여줘.
"""
inputHistory = []
outputHistory = []

client = openai.OpenAI(api_key=os.getenv("OPENAI_API_KEY"))

print("명령을 입력하세요. (종료하려면 exit 입력)")
while True:
    user_input = input(f"{Fore.CYAN}명령: {Style.RESET_ALL}")
    if user_input.lower() == "exit":
        break
    inputHistory.append(user_input)
    input_text = "\n".join(inputHistory)
    output_text = "\n".join(outputHistory)
    response = client.chat.completions.create(
        model="gpt-5",
        messages=[
            {"role": "system", "content": rules},
            {"role": "user", "content": f"지금까지 내린 명령:\n{input_text}\n명령에 대한 LLM의 출력:\n{output_text}\n이번 명령: {user_input}"}
        ]
    )
    answer = response.choices[0].message.content
    if answer.startswith("NOT_GCODE"):
        not_gcode_text = answer[len("NOT_GCODE"):].strip()
        print(f"{Fore.YELLOW}{not_gcode_text}{Style.RESET_ALL}")
        continue
    outputHistory.append(answer)
    print(f"{Fore.YELLOW}{answer}{Style.RESET_ALL}")
    # ser.write((answer + "\n").encode('utf-8'))  # 전체 명령을 한 번에 전송
    # checkGcodeStatus(ser)  # ok 받을 때까지 대기