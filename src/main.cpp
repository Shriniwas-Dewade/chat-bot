#include "bot.h"

int main()
{
    {
        std::string modelPath = "../model/mistral-7b-instruct-v0.2-q4_k_m.gguf";
        bot chatbot(modelPath);
        chatbot.startChat();
    }

    return 0;
}