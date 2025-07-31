from google import genai
import os

client = genai.Client(api_key=os.getenv("GEMINI_API_KEY"))

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