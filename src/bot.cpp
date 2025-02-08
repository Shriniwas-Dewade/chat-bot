#include "bot.h"

bot::bot(const std::string& modelPath) : modelPath(modelPath)
{
    std::cout << "\033[1;34mLoading model: " << modelPath << "\033[0m\n";

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
        std::cerr << "\033[1;31mFailed to load model: " << modelPath << "\033[0m\n";
        std::exit(1);
    }

    this->vocab = llama_model_get_vocab(this->model);

    contextParams = llama_context_default_params();
    contextParams.n_ctx = 4096;
    contextParams.n_batch = 4096;
    contextParams.n_threads = 7;
    contextParams.n_threads_batch = contextParams.n_threads;

    std::cout << "Number of threads: " << contextParams.n_threads << std::endl;

    this->context = llama_init_from_model(this->model, contextParams);

    if (context == nullptr)
    {
        std::cerr << "\033[1;31mFailed to create context!\033[0m\n";
        std::exit(1);
    }

    this->sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(this->sampler, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(this->sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    std::cout << "\033[1;34mModel loaded successfully!\033[0m\n";
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
    std::string input;
    std::cout << "\033[1;32mChatbot Ready! Type 'exit' to quit.\033[0m\n";

    while (true)
    {
        printUser("You: ");
        std::getline(std::cin, input);

        if (input == "exit")
        {
            break;
        }

        std::string response = getResponse(input);
        printBot("Bot: " + response);
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
