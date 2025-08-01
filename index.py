from google import genai
import os

# 색상 출력을 위한 colorama 추가
from colorama import Fore, Style, init
init(autoreset=True)

rules = """
1. 너의 대답의 출력은 PWM값으로 모터에다 전달할 값이기 때문에 반드시 어떤 상황에서도 숫자로만 출력해.
2. PWM값은 0부터 255까지의 범위로 제한되어 있어. 따라서 너의 출력은 항상 이 범위 내에 있어야 해.
3. 너는 그동안 했던 질문들과 그에 따른 너의 출력을 기억하지 못하기 때문에 질문들은 inputHistory로, 너의 출력은 outputHistory로 정리해줄게.
4. 질문 형식은 다음과 같아. rules->inputHistory->outputHistory->question->너의 대답
5.
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
