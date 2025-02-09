#include "bot.h"

int main()
{
    std::ios_base::sync_with_stdio(false);

    {
        std::string modelPath = "../model/mistral-7b-instruct-v0.2.Q5_K_M.gguf";
        bot chatbot(modelPath);
        chatbot.startChat();
    }

    return 0;
}