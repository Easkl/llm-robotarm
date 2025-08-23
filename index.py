import openai
import os
import serial
import time

# 색상 출력을 위한 colorama 추가
from colorama import Fore, Style, init
init(autoreset=True)

# ser = serial.Serial(port ='COM3', #장치관리자에서 포트는 매번 수정해줄 것.
#                     baudrate=115200, 
#                     timeout=1) 

rules = """
단위는 무조건 밀리미터(mm)로 고정한다. 원점은 (0,0)이며 이는 캔버스의 왼쪽 아래에 해당한다. 캔버스 크기는 297×210(mm)으로 고정한다. 좌표계는 절대좌표계만 사용한다.
펜 업은 G0 Z5로 정의하며, 펜 다운은 G1 Z0으로 정의한다. 이동은 G0 X.. Y.. 형태로 하며 이는 펜이 올라간 상태에서만 사용한다. 그리기는 G1 X.. Y.. 형태로 하며 이는 펜이 내려간 상태에서만 사용한다. 원호는 선택적으로 사용할 수 있으며, 시계 방향은 G2 X.. Y.. I.. J.., 반시계 방향은 G3 X.. Y.. I.. J..로 정의한다. 여기서 I, J는 현재 위치에서 원 중심까지의 오프셋을 의미한다. 원호의 끝점 좌표가 시작점과 같을 경우 완전한 원으로 해석한다.
한 줄에는 반드시 하나의 명령만 작성하며, 파라미터(X, Y, I, J, Z 등)는 같은 줄에 쓴다. 각 줄은 시리얼 통신을 통해 전송되며, 장비가 “ok”라는 응답을 반환한 뒤에만 다음 줄을 전송한다. 좌표는 항상 캔버스 범위(0 ≤ X ≤ 297, 0 ≤ Y ≤ 210) 안에서만 사용한다. 이 범위를 벗어나는 좌표는 오류로 처리하거나 클램핑한다.
피드 속도(F), 공구 선택(T), 압출(E), 스핀들(M3/M5), 히터(M104/M140) 등은 사용하지 않는다. ARC 명령어에서 I=J=0은 금지한다.
가장 중요하게, 출력 형식은 오직 G-code 명령어 줄만 나열하고, 어떤 경우에도 주석이나 설명은 포함하지 않는 규칙으로 고정
"""
#그동안 했던 질문 기억용 리스트
inputHistory = []
#질문에 따른 출력값 history
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
    outputHistory.append(answer)
    print(f"{Fore.YELLOW}{answer}{Style.RESET_ALL}")
    # lines = answer.strip().split("\n")
    # for line in lines:
    #     ser.write((line + "\n").encode('utf-8'))
    #     while True:
    #         recv = ser.readline().decode('utf-8').strip()
    #         if recv == "ok":
    #             break