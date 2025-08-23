from google import genai
import os

# 색상 출력을 위한 colorama 추가
from colorama import Fore, Style, init
init(autoreset=True)

rules = """
출력은 항상 G-code만. 설명/주석/빈 줄/특수문자 금지. 한 줄당 한 명령.

입력 형식: rules → inputHistory → outputHistory → question. 상태 기억은 하지 않으므로 항상 inputHistory와 outputHistory로 문맥을 잇는다.

캔버스: A4 세로, 단위 mm, 범위 0 ≤ X ≤ 210, 0 ≤ Y ≤ 297. 원점 (0,0)은 좌하단. 범위 밖 좌표는 가장자리로 클리핑.

명령은 표준만 사용: G0, G1, G2, G3. Z축 사용 금지.

G0는 펜 업 이동, G1/G2/G3는 펜 다운 그리기.

F는 PWM duty로 사용(0~1000). 기본값: G0는 F1200, G1/G2/G3는 F800. 속도·진하기 요구가 있으면 F로만 조절.

곡선·원·원호는 직선 근사 금지. 반드시 G2/G3 사용.

전체 원은 시작점을 원 둘레의 임의 위치로 지정한 뒤 G2 또는 G3 한 바퀴로 출력. 컨트롤러가 한 바퀴 지원하지 않으면 동일 중심으로 반바퀴 두 번으로 분할.

중심 지정은 I,J 오프셋 또는 R 사용. 기본은 I,J.

닫힌 도형은 반드시 시작점으로 복귀하여 닫는다.

경로 계획 시 G0 이동을 최소화하고 연속 경로 우선.

좌표가 없는 추상 요구는 기본 크기 50mm, 캔버스 중앙 배치. 여러 개 요청 시 자동 배치로 겹침 방지.

출력은 헤더 이후 G코드만 나열한다. 불필요한 공백, 설명, 특수문자, 비표준 코드는 비용 절감을 위해 금지.
"""
#그동안 했던 질문 기억용 리스트
inputHistory = []
#질문에 따른 출력값 history
outputHistory = []

client = genai.Client(api_key=os.getenv("GEMINI_API_KEY"))

print("명령을 입력하세요. (종료하려면 exit 입력)")
while True:
    user_input = input(f"{Fore.CYAN}명령: {Style.RESET_ALL}")
    if user_input.lower() == "exit":
        break
    inputHistory.append(user_input)
    input_text = "\n".join(inputHistory)
    output_text = "\n".join(outputHistory)
    response = client.models.generate_content(
        model="gemini-2.5-flash-lite",
        contents=f"{rules}\n지금까지의 명령:\n{input_text}\n명령에 대한 결과값:\n{output_text}\n이번 명령: {user_input}"
    )
    outputHistory.append(response.candidates[0].content.parts[0].text)
    print(f"{Fore.YELLOW}{response.candidates[0].content.parts[0].text}{Style.RESET_ALL}")
