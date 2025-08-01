from google import genai
import os

client = genai.Client(api_key=os.getenv("GEMINI_API_KEY"))
response = client.models.generate_content(
    model="gemini-2.5-flash-lite", 
    contents="why is the sky blue?"
)

print(response.text)