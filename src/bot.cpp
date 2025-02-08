#include "bot.h"

bot::bot(const std::string& modelPath) : modelPath(modelPath)
{
    animateText("\033[1;34m🔥 Booting up the AI... Loading the Matrix... \033[0m", 30);

    llama_log_set([](enum ggml_log_level level, const char * text, void * /* user_data */) {
        if (level >= GGML_LOG_LEVEL_ERROR) {
            fprintf(stderr, "%s", text);
        }
    }, nullptr);

    ggml_backend_load_all();

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 21;
    this->model = llama_model_load_from_file(modelPath.c_str(), model_params);

    if (model == nullptr)
    {
        animateText("\033[1;31m❌ AI Error: Model loading failed! \033[0m", 40);
        std::exit(1);
    }

    this->vocab = llama_model_get_vocab(this->model);

    contextParams = llama_context_default_params();
    contextParams.n_ctx = 4096;
    contextParams.n_batch = 2048;
    contextParams.n_threads = 7;
    contextParams.n_threads_batch = contextParams.n_threads;

    this->context = llama_init_from_model(this->model, contextParams);

    if (context == nullptr)
    {
        animateText("\033[1;31m💀 AI Crash: Failed to create context! \033[0m", 40);
        std::exit(1);
    }

    this->sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(this->sampler, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_top_p(0.95f, 0));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    animateText("\033[1;34m✅ AI Loaded & Ready to Drop Some Wisdom! 🚀\033[0m", 30);
}

bot::~bot()
{
    if (model != nullptr)
    {
        llama_model_free(model);
    }

    if (context != nullptr)
    {
        llama_free(context);
    }

    if (sampler != nullptr)
    {
        llama_sampler_free(sampler);
    }
}

void bot::startChat()
{
    std::vector<llama_chat_message> chat_history;
    std::vector<char> formatted_input(llama_n_ctx(context));

    std::string system_prompt = "[INST] Yo, wassup? You chattin' with the realest AI out here. \n"
                                "I keep it 💯 and talk street. No corny, formal talk—just real convos. \n"
                                "I got bars, I got wisdom, and I got jokes. Let's chop it up! [/INST]";
    
    chat_history.push_back({"system", strdup(system_prompt.c_str())});

    int prev_len = 0;

    animateText("\033[1;32m✨ Yo! AI at your service. Type 'exit' to bounce. 🚀\033[0m", 40);

    while(true)
    {
        printUser("😎 You : ");
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit")
        {
            animateText("\033[1;36m👋 Aight, I'm out! Stay cool. 😎\033[0m", 40);
            break;
        }

        const char * tmpl = llama_model_chat_template(model, nullptr);
        chat_history.push_back({"user", strdup(input.c_str())});

        int new_len = llama_chat_apply_template(tmpl, chat_history.data(), chat_history.size(), true, formatted_input.data(), formatted_input.size());
        
        if (new_len > (int)formatted_input.size()) 
        {
            formatted_input.resize(new_len);
            new_len = llama_chat_apply_template(tmpl, chat_history.data(), chat_history.size(), true, formatted_input.data(), formatted_input.size());
        }
        
        if (new_len < 0) 
        {
            std::cerr << "Failed to apply the chat template" << std::endl;
            continue;
        }
        
        std::string prompt(formatted_input.begin() + prev_len, formatted_input.begin() + new_len);
        animateText("🤖 Neura : " + getResponse(prompt), 35);
        
        chat_history.push_back({"assistant", strdup(prompt.c_str())});
        prev_len = llama_chat_apply_template(tmpl, chat_history.data(), chat_history.size(), false, nullptr, 0);
        
        if (prev_len < 0) 
        {
            std::cerr << "Failed to apply the chat template" << std::endl;
        }
    }
}

std::string bot::getResponse(const std::string& input) 
{
    bool is_first = llama_get_kv_cache_used_cells(context) == 0;

    const int32_t num_tokens = -llama_tokenize(vocab, input.c_str(), input.size(), nullptr, 0, is_first, true);
    
    if (num_tokens < 0) 
    {
        std::cerr << "Tokenization failed!" << std::endl;
        return "Error: Tokenization failed.";
    }

    std::vector<llama_token> tokens(num_tokens);
    if (llama_tokenize(vocab, input.c_str(), input.size(), tokens.data(), tokens.size(), is_first, true) < 0) 
    {
        std::cerr << "Failed to tokenize input text!" << std::endl;
        return "Error: Could not process input.";
    }


    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());

    std::string response;
    llama_token new_token_id;

    while (true)
    {
        int n_ctx = llama_n_ctx(context);
        int n_ctx_used = llama_get_kv_cache_used_cells(context);

        if (n_ctx_used + batch.n_tokens >= n_ctx)
        {
            std::cerr << "Warning: Context full! Shifting memory.\n";
            return "Error: Context is full.";
        }

        if(llama_decode(context, batch))
        {
            std::cerr << "Failed to decode!" << std::endl;
            return "Error: Failed to decode.";
        }

        new_token_id = llama_sampler_sample(sampler, context, -1);

        if (llama_vocab_is_eog(vocab, new_token_id))
        {
            break;
        }

        char buf[1024];
        int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);

        std::string text(buf, n);
        response += text;

        batch = llama_batch_get_one(&new_token_id, 1);
    }

    return response;
}


void bot::printUser(const std::string& text)
{
    std::cout << "\033[1;36m" << text << "\033[0m";
}

void bot::printBot(const std::string& text)
{
    std::cout << "\033[1;33m" << text << "\033[0m\n";
}

void bot::animateText(const std::string& text, int delay)
{
    for (char c : text)
    {
        std::cout << c << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    std::cout << std::endl;
}
