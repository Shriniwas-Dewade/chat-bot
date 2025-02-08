#ifndef BOT_H
#define BOT_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include "llama.h"

class bot
{
private:
    std::string modelPath;
    llama_model* model;
    llama_context_params contextParams;
    llama_context* context;
    llama_sampler* sampler;
    const llama_vocab* vocab;

public:
    bot(const std::string& modelPath);
    ~bot();

    void startChat();
    std::string getResponse(const std::string& input);

    static void printUser(const std::string& message);
    static void printBot(const std::string& message);
};

#endif // BOT_H
